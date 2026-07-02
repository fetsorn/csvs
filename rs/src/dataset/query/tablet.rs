use super::groups::{key_groups, KeyGroup};
use super::line::State;
use super::strategy::Tablet;
use crate::{Dataset, Entry, Grain, Result};
use async_stream::try_stream;
use futures_core::stream::Stream;
use regex::Regex;

fn make_state_initial(state: &State, tablet: &Tablet) -> State {
    // if tablet base differs from query base, sow previous entry into a new entry
    let is_same_base = tablet.base == state.query.base;

    let do_discard = state.entry.is_none() || is_same_base;

    let entry_initial = if do_discard {
        Entry::new(&tablet.base)
    } else {
        let prev = state.entry.as_ref().expect("checked is_some via do_discard");
        let e = Entry::new(&tablet.base);

        let g = Grain {
            base: prev.base.clone(),
            base_value: prev.base_value.clone(),
            leaf: prev.base.clone(),
            leaf_value: None,
        };

        e.sow(&g, &tablet.base, &prev.base)
    };

    let entry_base_changed = match &state.entry {
        None => true,
        Some(e) => e.base != entry_initial.base,
    };

    // if entry base changed, forget parent join key
    let thing_querying_initial = if entry_base_changed {
        None
    } else {
        state.thing_querying.clone()
    };

    State {
        entry: Some(entry_initial),
        query: state.query.clone(),
        thing_querying: thing_querying_initial,
    }
}

/// Match a key group against query grains.
/// Returns Some(State) if any value in the group matched, None otherwise.
fn match_group(
    group: &KeyGroup,
    tablet: &Tablet,
    grains: &[Grain],
    state_initial: &State,
) -> Option<State> {
    let mut group_entry = state_initial.entry.clone()?;
    let mut group_query = state_initial.query.clone();
    let mut matched = false;
    let mut group_thing_querying: Option<String> = None;

    for value in &group.values {
        let trait_ = if tablet.trait_is_first {
            group.key.clone()
        } else {
            value.clone()
        };

        let thing = if tablet.thing_is_first {
            group.key.clone()
        } else {
            value.clone()
        };

        let grain_new = if tablet.thing_is_first {
            if tablet.trait_is_first {
                Grain {
                    base: tablet.thing.clone(),
                    base_value: Some(group.key.clone()),
                    leaf: tablet.thing.clone(),
                    leaf_value: None,
                }
            } else {
                Grain {
                    base: tablet.thing.clone(),
                    base_value: Some(group.key.clone()),
                    leaf: tablet.trait_.clone(),
                    leaf_value: Some(value.clone()),
                }
            }
        } else if tablet.trait_is_first {
            Grain {
                base: tablet.trait_.clone(),
                base_value: Some(group.key.clone()),
                leaf: tablet.thing.clone(),
                leaf_value: Some(value.clone()),
            }
        } else {
            Grain {
                base: tablet.base.clone(),
                base_value: None,
                leaf: tablet.base.clone(),
                leaf_value: None,
            }
        };

        // all grains must match
        let is_match_grains = grains.iter().fold(None, |acc, grain| {
            let re_str = if tablet.trait_is_first {
                grain.base_value.as_deref().unwrap_or("")
            } else {
                grain.leaf_value.as_deref().unwrap_or("")
            };

            let is_match_grain = if tablet.trait_is_regex {
                Regex::new(re_str)
                    .ok()
                    .map_or(false, |re| re.is_match(&trait_))
            } else {
                re_str == trait_
            };

            Some(match acc {
                None => is_match_grain,
                Some(prev) => prev && is_match_grain,
            })
        });

        // when parent join key is set, also require it matches
        let is_match_querying = match &state_initial.thing_querying {
            Some(tq) => *tq == thing,
            None => true,
        };

        let is_match = is_match_grains.unwrap_or(false) && is_match_querying;

        if is_match {
            matched = true;
            group_thing_querying = Some(thing);
            group_entry = group_entry.sow(&grain_new, &tablet.trait_, &tablet.thing);
            group_query = group_query.sow(&grain_new, &tablet.trait_, &tablet.thing);
        }
    }

    if matched {
        Some(State {
            entry: Some(group_entry),
            query: group_query,
            thing_querying: group_thing_querying,
        })
    } else {
        None
    }
}

pub fn query_tablet_stream(
    dataset: Dataset,
    tablet: Tablet,
    state: State,
    is_first_tablet: bool,
) -> impl Stream<Item = Result<State>> {
    try_stream! {
        let filepath = dataset.dir.join(&tablet.filename);

        // an existing but empty (0-byte) tablet must behave like a missing
        // one, mirroring csvs-js `isEmpty` (stream.js), which treats size 0
        // the same as absent. guarding only on existence would let an empty
        // file fall through to the reader, yield no key groups, and silently
        // drop the accumulated query state on a non-first tablet.
        let is_empty = std::fs::metadata(&filepath)
            .map(|meta| meta.len() == 0)
            .unwrap_or(true);

        // first tablet needs lines — empty/missing file means no matches
        // later tablet — empty/missing file means match all
        if is_empty {
            if !is_first_tablet {
                yield state;
            }
            return;
        }

        let state_initial = make_state_initial(&state, &tablet);
        let grains = state_initial.query.mow(&tablet.trait_, &tablet.thing);

        let groups = key_groups(&filepath)?;

        for group in &groups {
            if let Some(matched_state) = match_group(group, &tablet, &grains, &state_initial) {
                yield matched_state;
            }
        }
    }
}
