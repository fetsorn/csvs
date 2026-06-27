use crate::Entry;
use serde_json::{json, Value};

impl From<Entry> for Value {
    fn from(entry: Entry) -> Value {
        let mut value: Value = json!({
            "_": entry.base,
        });

        match entry.base_value {
            None => (),
            Some(s) => value[entry.base] = s.into(),
        }

        match entry.leader_value {
            None => (),
            Some(s) => value["__"] = s.into(),
        }

        for (lang, text) in entry.prose.iter() {
            let key = match lang {
                None => "@".to_string(),
                Some(l) => format!("@{}", l),
            };
            value[key] = text.clone().into();
        }

        for (leaf, items) in entry.leaves.iter() {
            for child in items {
                // condense entry to a string if it has no leaves
                let leaf_value: Value = match child.leaves.is_empty() && child.prose.is_empty() {
                    true => match &child.base_value {
                        None => continue,
                        Some(s) => s.to_owned().into(),
                    },
                    false => child.clone().into(),
                };

                value[&leaf] = match value.get(leaf) {
                    None => leaf_value,
                    Some(i) => match i {
                        Value::String(s) => vec![s.to_owned().into(), leaf_value].into(),
                        Value::Object(o) => vec![o.clone().into(), leaf_value].into(),
                        Value::Array(vs) => [&vs[..], &[leaf_value]].concat().into(),
                        // skip unexpected value types rather than panicking
                        _ => continue,
                    },
                };
            }
        }

        value
    }
}
