use crate::dataset::query::groups::key_groups;
use crate::schema::is_well_formed_branch;
use crate::{Branch, Entry, Result, Schema, Trunks};
use regex::Regex;
use std::collections::{HashMap, HashSet, VecDeque};
use std::path::Path;

/// One `~` traversal: a closure over the tablet that stores the
/// relationship between the query base and the named field.
pub struct Traversal {
    /// Stop regexes: a value matching any of these is neither
    /// returned nor expanded.
    stops: Vec<Regex>,
    /// Two-way adjacency over the tablet lines. The direction in
    /// which the tablet stores the relationship does not affect
    /// the result.
    neighbors: HashMap<String, Vec<String>>,
    /// Values this traversal has already expanded.
    visited: HashSet<String>,
}

impl Traversal {
    /// Whether a value matches a stop value.
    pub fn is_stopped(&self, value: &str) -> bool {
        self.stops.iter().any(|re| re.is_match(value))
    }

    /// Breadth-first closure from a seed value over the two-way join.
    /// Returns newly reached values in discovery order, skipping stops
    /// and values this traversal has already expanded.
    pub fn closure(&mut self, seed: &str) -> Vec<String> {
        let mut reached = vec![];

        if !self.visited.insert(seed.to_owned()) {
            return reached;
        }

        let mut queue = VecDeque::from([seed.to_owned()]);

        while let Some(value) = queue.pop_front() {
            let nexts = match self.neighbors.get(&value) {
                Some(ns) => ns.clone(),
                None => continue,
            };

            for next in nexts {
                if self.visited.contains(&next) || self.is_stopped(&next) {
                    continue;
                }

                self.visited.insert(next.clone());

                reached.push(next.clone());

                queue.push_back(next);
            }
        }

        reached
    }
}

/// Parse `~` specs out of the query leaves, removing the `~` leaf.
/// Each spec names a field of the query base collection and optionally
/// carries stop regexes under `.`. A spec without a base value or with
/// a base other than `~` is discarded.
pub fn extract_traversals(
    dir: &Path,
    schema: &Schema,
    query: &mut Entry,
) -> Result<Vec<Traversal>> {
    let specs = match query.leaves.remove("~") {
        None => return Ok(vec![]),
        Some(specs) => specs,
    };

    let mut traversals = vec![];

    for spec in specs {
        if spec.base != "~" {
            continue;
        }

        // the base value of a `~` record is an exact field name
        let field = match &spec.base_value {
            Some(f) if !f.is_empty() => f.clone(),
            _ => continue,
        };

        // stop values are regexes; a malformed regex matches nothing
        let stops: Vec<Regex> = spec
            .leaves
            .get(".")
            .map(|vs| {
                vs.iter()
                    .filter_map(|v| v.base_value.as_deref())
                    .filter_map(|s| Regex::new(s).ok())
                    .collect()
            })
            .unwrap_or_default();

        let neighbors = load_neighbors(dir, schema, &query.base, &field)?;

        traversals.push(Traversal {
            stops,
            neighbors,
            visited: HashSet::new(),
        });
    }

    Ok(traversals)
}

/// Find the tablet that stores the relationship between `base` and
/// `field` and read it into a two-way adjacency map. An unknown
/// relationship or a missing tablet yields an empty map.
fn load_neighbors(
    dir: &Path,
    schema: &Schema,
    base: &str,
    field: &str,
) -> Result<HashMap<String, Vec<String>>> {
    // collection names are interpolated into a filename
    if !is_well_formed_branch(base) || !is_well_formed_branch(field) {
        return Ok(HashMap::new());
    }

    let has_trunk = |branch: &str, trunk: &str| match schema.0.get(branch) {
        Some(Branch {
            trunks: Trunks(ts), ..
        }) => ts.iter().any(|t| t == trunk),
        None => false,
    };

    let filename = if has_trunk(field, base) {
        format!("{}-{}.csv", base, field)
    } else if has_trunk(base, field) {
        format!("{}-{}.csv", field, base)
    } else {
        return Ok(HashMap::new());
    };

    let mut neighbors: HashMap<String, Vec<String>> = HashMap::new();

    for group in key_groups(&dir.join(filename))? {
        for value in group.values {
            neighbors
                .entry(group.key.clone())
                .or_default()
                .push(value.clone());

            neighbors.entry(value).or_default().push(group.key.clone());
        }
    }

    Ok(neighbors)
}
