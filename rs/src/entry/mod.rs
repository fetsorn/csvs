mod into_value;
pub mod mow;
pub mod sow;
mod try_from;
use crate::Grain;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::HashMap;
use std::fmt;

/// A SON record.
///
/// `prose` holds description blobs keyed by language tag.
/// `None` = untagged (`@`), `Some("en")` = `@en`, etc.
/// Populated only when the prose flag is set during build.
#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Entry {
    pub base: String,
    pub base_value: Option<String>,
    pub leader_value: Option<String>,
    pub leaves: HashMap<String, Vec<Entry>>,
    #[serde(default, skip_serializing_if = "HashMap::is_empty")]
    pub prose: HashMap<Option<String>, String>,
}

impl fmt::Display for Entry {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let value: Value = self.clone().into();
        write!(f, "{}", value)
    }
}

impl Entry {
    pub fn new(base: &str) -> Self {
        Entry {
            base: base.to_string(),
            base_value: None,
            leader_value: None,
            leaves: HashMap::new(),
            prose: HashMap::new(),
        }
    }

    pub fn mow(&self, trait_: &str, thing: &str) -> Vec<Grain> {
        mow::mow(self, trait_, thing)
    }

    pub fn sow(&self, grain: &Grain, trait_: &str, thing: &str) -> Entry {
        sow::sow(self, grain, trait_, thing)
    }
}
