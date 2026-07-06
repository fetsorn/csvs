use crate::{Result, Dataset, Entry};
use async_stream::try_stream;
use futures_core::stream::Stream;
use futures_util::pin_mut;
use futures_util::stream::StreamExt;
use std::collections::HashSet;
use std::pin::Pin;
mod recursion;

pub fn select_record_stream(
    dataset: Dataset,
    query: Vec<Entry>,
    light: bool,
) -> impl Stream<Item = Result<Entry>> {
    try_stream! {
        let mut seen = if query.len() > 1 {
            Some(HashSet::new())
        } else {
            None
        };

        for q in query {
            let is_schema = q.base == "_";

            if is_schema {
                let schema_record = dataset.select_schema().await?;

                yield schema_record;

                continue;
            }

            let is_version = q.base == ".";

            if is_version {
                let version_record = dataset.select_version().await?;

                yield version_record;

                continue;
            }

            // Extract prose filters from query
            let prose_filters: Vec<(Option<String>, String)> = q.prose.iter()
                .filter(|(_, v)| !v.is_empty())
                .map(|(lang, pattern)| (lang.clone(), pattern.clone()))
                .collect();

            let prose_allowed: Option<HashSet<String>> = if !prose_filters.is_empty() {
                let mut allowed: Option<HashSet<String>> = None;

                for (lang, pattern) in &prose_filters {
                    let matches = dataset.prose_address.search_prose(
                        &dataset.dir,
                        pattern,
                        lang.as_deref(),
                    )?;

                    let match_set: HashSet<String> = matches.into_iter().collect();

                    allowed = Some(match &allowed {
                        None => match_set,
                        Some(prev) => prev.intersection(&match_set).cloned().collect(),
                    });
                }

                allowed
            } else {
                None
            };

            // Strip prose keys from query before tablet dispatch
            let mut q_stripped = q.clone();
            q_stripped.prose.clear();

            // Extract `~` recursion traversals from the query
            let mut traversals = if q_stripped.leaves.contains_key("~") {
                let schema = dataset.get_schema().await?;

                recursion::extract_traversals(&dataset.dir, &schema, &mut q_stripped)?
            } else {
                vec![]
            };

            // Base values yielded by this query, for closure dedup
            let mut yielded: HashSet<String> = HashSet::new();

            let has_leaves = q_stripped.leaves.len() > 0;

            let mut stream: Pin<Box<dyn Stream<Item = Result<Entry>> + Send>> = if has_leaves {
                Box::pin(dataset.clone().query_record_stream(q_stripped))
            } else {
                Box::pin(dataset.clone().select_option_stream(q_stripped))
            };

            while let Some(entry) = stream.next().await {
                let entry = entry?;

                // Filter by prose matches
                if let Some(ref allowed) = prose_allowed {
                    if let Some(ref bv) = entry.base_value {
                        if !allowed.contains(bv) {
                            continue;
                        }
                    }
                }

                if let Some(ref mut set) = seen {
                    if let Some(ref bv) = entry.base_value {
                        if !set.insert(bv.clone()) {
                            continue;
                        }
                    }
                }

                let seed = match (traversals.is_empty(), &entry.base_value) {
                    // no recursion — yield the entry as before
                    (true, _) | (false, None) => {
                        if light {
                            yield entry;
                        } else {
                            yield dataset.clone().build_record(entry).await?;
                        }

                        continue;
                    }
                    (false, Some(bv)) => bv.clone(),
                };

                // a seed that matches a stop is neither returned nor expanded
                if traversals.iter().all(|t| t.is_stopped(&seed)) {
                    continue;
                }

                if !yielded.insert(seed.clone()) {
                    continue;
                }

                if light {
                    yield entry;
                } else {
                    yield dataset.clone().build_record(entry).await?;
                }

                // records reached by closure are returned regardless
                // of whether they match the query fields
                for traversal in traversals.iter_mut() {
                    if traversal.is_stopped(&seed) {
                        continue;
                    }

                    for value in traversal.closure(&seed) {
                        if !yielded.insert(value.clone()) {
                            continue;
                        }

                        if let Some(ref mut set) = seen {
                            if !set.insert(value.clone()) {
                                continue;
                            }
                        }

                        let mut reached = Entry::new(&q.base);

                        reached.base_value = Some(value);

                        if light {
                            yield reached;
                        } else {
                            yield dataset.clone().build_record(reached).await?;
                        }
                    }
                }
            }
        }
    }
}

pub async fn select_record(dataset: Dataset, query: Vec<Entry>, light: bool) -> Result<Vec<Entry>> {
    let mut entries = vec![];

    let s = dataset.select_record_stream(query, light);

    pin_mut!(s);

    while let Some(entry) = s.next().await {
        let entry = entry?;

        entries.push(entry);
    }

    Ok(entries)
}

pub async fn print_record(dataset: Dataset, query: Vec<Entry>) -> Result<()> {
    let s = dataset.select_record_stream(query, false);

    pin_mut!(s);

    while let Some(entry) = s.next().await {
        let entry = entry?;

        println!("{}", entry);
    }

    Ok(())
}
