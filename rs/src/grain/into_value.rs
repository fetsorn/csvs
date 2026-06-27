use super::Grain;
use serde_json::{json, Value};

impl From<Grain> for Value {
    fn from(grain: Grain) -> Value {
        match grain.leaf_value {
            Some(leaf_value) => json!({
                "_": grain.base,
                grain.base: grain.base_value,
                grain.leaf: leaf_value
            }),
            None => json!({
                "_": grain.base,
                grain.base: grain.base_value
            }),
        }
    }
}
