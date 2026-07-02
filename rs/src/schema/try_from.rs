use super::{Branch, Leaves, Schema, Trunks};
use crate::{Entry, Error, Result};
use serde_json::Value;
use std::collections::HashMap;
use std::convert::TryFrom;

impl TryFrom<Entry> for Schema {
    type Error = Error;

    fn try_from(entry: Entry) -> Result<Self> {
        if entry.base != "_" {
            return Err(Error::from_message("base is not _"));
        }

        // reject collection names that would be unsafe as tablet filenames
        // before any filename is built from them (path traversal guard)
        for (trunk, leaves) in entry.leaves.iter() {
            super::assert_well_formed_branch(trunk)?;

            for leaf in leaves.iter().filter_map(|e| e.base_value.as_ref()) {
                super::assert_well_formed_branch(leaf)?;
            }
        }

        let node_map: HashMap<String, Branch> =
            entry
                .leaves
                .iter()
                .fold(HashMap::new(), |with_trunk, (trunk, leaves)| {
                    leaves.iter().filter_map(|e| e.base_value.as_ref()).fold(
                        with_trunk,
                        |mut with_leaf, leaf| {
                            let trunk_branch = match with_leaf.get(trunk) {
                                None => Branch {
                                    trunks: Trunks(vec![]),
                                    leaves: Leaves(vec![]),
                                },
                                Some(vs) => vs.clone(),
                            };

                            let trunk_trunks = trunk_branch.trunks.clone();

                            let trunk_leaves =
                                Leaves([&trunk_branch.leaves.0[..], &[leaf.clone()]].concat());

                            with_leaf.insert(
                                trunk.to_owned(),
                                Branch {
                                    trunks: trunk_trunks,
                                    leaves: trunk_leaves,
                                },
                            );

                            let leaf_branch = match with_leaf.get(leaf) {
                                None => Branch {
                                    trunks: Trunks(vec![]),
                                    leaves: Leaves(vec![]),
                                },
                                Some(vs) => vs.clone(),
                            };

                            let leaf_trunks =
                                Trunks([&leaf_branch.trunks.0[..], &[trunk.to_owned()]].concat());

                            let leaf_leaves = leaf_branch.leaves;

                            with_leaf.insert(
                                leaf.to_owned(),
                                Branch {
                                    trunks: leaf_trunks,
                                    leaves: leaf_leaves,
                                },
                            );

                            with_leaf
                        },
                    )
                });

        Ok(Schema(node_map))
    }
}

impl TryFrom<Value> for Schema {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self> {
        let entry: Entry = value.try_into()?;

        entry.try_into()
    }
}

#[cfg(test)]
mod tests {
    use super::Schema;
    use serde_json::json;

    #[test]
    fn schema_rejects_traversal_branch() {
        // a malicious schema whose branch escapes the `event-` filename
        // prefix via an inner slash: `event-x/../../secret.csv`
        let value = json!({ "_": "_", "event": "x/../../secret" });

        let result: Result<Schema, _> = value.try_into();

        assert!(result.is_err(), "traversal branch must be rejected");
    }

    #[test]
    fn schema_rejects_traversal_trunk() {
        let value = json!({ "_": "_", "x/../../secret": "date" });

        let result: Result<Schema, _> = value.try_into();

        assert!(result.is_err(), "traversal trunk must be rejected");
    }

    #[test]
    fn schema_accepts_normal_names() {
        let value = json!({ "_": "_", "event": ["date", "actName"] });

        let result: Result<Schema, _> = value.try_into();

        assert!(result.is_ok(), "normal schema must be accepted");
    }
}
