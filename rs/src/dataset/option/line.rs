use super::strategy::Tablet;
use crate::{line::Line, Entry, Grain, Result};
use regex::Regex;
use std::collections::HashMap;

#[derive(Debug, Clone)]
pub struct State {
    pub query: Entry,
    pub entry: Entry,
    pub fst: Option<String>,
    pub is_match: bool,
    pub match_map: HashMap<String, bool>,
    pub last: Option<Entry>,
}

pub fn option_line(tablet: Tablet, state: State, line: Line) -> Result<State> {
    let mut state = state;

    let fst_is_new = state.fst.is_none() || state.fst.as_ref() != Some(&line.key);

    state.fst = Some(line.key.clone());

    let push_end_of_group = fst_is_new && state.is_match;

    if push_end_of_group {
        state.last = Some(state.entry.clone());

        state.entry = Entry::new(&tablet.base);

        state.is_match = false;
    }

    let base = if tablet.thing_is_first {
        line.key.to_owned()
    } else {
        line.value.to_owned()
    };

    let grain_new = Grain {
        base: tablet.base.to_owned(),
        base_value: Some(base.clone()),
        leaf: tablet.base.to_owned(),
        leaf_value: None,
    };

    let is_match = match state.query.clone().base_value {
        None => true,
        Some(base_value) => {
            let re_str = base_value;

            // a malformed pattern matches nothing instead of aborting the
            // stream, mirroring the query path (dataset/query/tablet.rs)
            Regex::new(&re_str)
                .ok()
                .map_or(false, |re| re.is_match(&base))
        }
    };

    let match_is_new = state.match_map.get(&base).is_none();

    state.is_match = if state.is_match {
        state.is_match
    } else {
        is_match && match_is_new
    };

    if is_match && match_is_new {
        state.match_map.insert(base, true);

        state.entry = state
            .entry
            .clone()
            .sow(&grain_new, &tablet.base, &tablet.base);
    }

    Ok(state)
}
