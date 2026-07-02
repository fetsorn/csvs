use super::{Branch, Schema, Trunks};
use std::collections::HashSet;

/// Nesting level = distance from a leaf node in the schema graph.
/// Leaves (no trunks) are level 0, their trunks are level 1, etc.
/// E.g. for datum -> filepath -> moddate: moddate=0, filepath=1, datum=2.
pub fn get_nesting_level(schema: &Schema, branch: &str) -> i32 {
    get_nesting_level_pathed(schema, branch, &HashSet::new())
}

/// The schema graph may contain cycles (e.g. `event -> date -> event`),
/// for which distance-from-a-leaf is undefined. `path` holds the branches
/// on the current descent; revisiting one closes a cycle and stops the
/// descent so the recursion terminates instead of overflowing the stack.
/// A fresh copy is passed to each trunk so that a branch reachable through
/// two distinct paths (a diamond, not a cycle) is still counted fully.
fn get_nesting_level_pathed(schema: &Schema, branch: &str, path: &HashSet<String>) -> i32 {
    let trunks = match schema.0.get(branch) {
        None => return 0,
        Some(Branch {
            trunks: Trunks(ts), ..
        }) => ts,
    };

    if path.contains(branch) {
        return 0;
    }

    let mut next_path = path.clone();
    next_path.insert(branch.to_string());

    let trunk_levels: Vec<i32> = trunks
        .iter()
        .map(|trunk| get_nesting_level_pathed(schema, trunk, &next_path))
        .collect();

    let level: i32 = *trunk_levels.iter().max().unwrap_or(&-1);

    level + 1
}

#[cfg(test)]
mod tests {
    use crate::schema::Schema;
    use serde_json::json;
    use std::convert::TryFrom;

    // self-recursive and mutually-recursive schemas must terminate
    #[test]
    fn recursive_schema_terminates() {
        let selfrec = Schema::try_from(json!({ "_": "_", "landmark": "landmark" })).unwrap();
        assert_eq!(selfrec.get_nesting_level("landmark"), 1);

        let mutual =
            Schema::try_from(json!({ "_": "_", "event": "date", "date": "event" })).unwrap();
        // just needs to terminate with a finite value
        let _ = mutual.get_nesting_level("event");
        let _ = mutual.get_nesting_level("date");
    }

    // a diamond (two paths to the same branch, no cycle) is still counted
    // at full depth: the cycle guard must not collapse the second path.
    #[test]
    fn diamond_counts_full_depth() {
        let schema = Schema::try_from(json!({
            "_": "_",
            "datum": ["a", "b"],
            "a": "leaf",
            "b": "leaf"
        }))
        .unwrap();

        assert_eq!(schema.get_nesting_level("datum"), 0);
        assert_eq!(schema.get_nesting_level("a"), 1);
        assert_eq!(schema.get_nesting_level("b"), 1);
        assert_eq!(schema.get_nesting_level("leaf"), 2);
    }
}
