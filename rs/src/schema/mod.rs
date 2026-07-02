mod count_leaves;
mod find_crown;
mod get_nesting_level;
mod is_connected;
mod sort_nesting_ascending;
mod sort_nesting_descending;
mod try_from;
use crate::{Error, Result};
use serde::{Deserialize, Serialize};
use std::cmp::Ordering;
use std::collections::HashMap;

/// Whether `name` is a well-formed collection (branch) name: a non-empty
/// string that is neither the reserved schema base `_` nor contains a
/// character the spec forbids in collection names.
///
/// Branch names are interpolated into tablet filenames as
/// `{trunk}-{branch}.csv`, so an unconstrained name — e.g. one containing
/// a slash and `..` — escapes the dataset directory and allows arbitrary
/// file reads and writes (path traversal). This mirrors the prose
/// language-tag guard (`is_well_formed_lang`), which closes the same
/// vector for prose filenames.
///
/// The forbidden set covers filesystem separators and traversal
/// (`/ \ .`), the tablet key separator (`-`), and the other characters the
/// spec reserves because collection names are used as filenames.
pub fn is_well_formed_branch(name: &str) -> bool {
    !name.is_empty()
        && name != "_"
        && !name.chars().any(|c| {
            matches!(
                c,
                '/' | '\\'
                    | '<'
                    | '>'
                    | '\''
                    | ':'
                    | '"'
                    | '`'
                    | '|'
                    | '?'
                    | '*'
                    | '.'
                    | ','
                    | '['
                    | ']'
                    | ';'
                    | '{'
                    | '}'
                    | '$'
                    | '&'
                    | '-'
                    | '\n'
                    | '\r'
            )
        })
}

/// Return an error if a collection name would be unsafe as a tablet filename.
pub fn assert_well_formed_branch(name: &str) -> Result<()> {
    if !is_well_formed_branch(name) {
        return Err(Error::from_message(format!(
            "csvs: collection name is not a well-formed branch: {name:?}"
        )));
    }

    Ok(())
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Leaves(pub Vec<String>);

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Trunks(pub Vec<String>);

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Branch {
    pub trunks: Trunks,
    pub leaves: Leaves,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Schema(pub HashMap<String, Branch>);

impl Schema {
    pub fn is_connected(&self, base: &str, branch: &str) -> bool {
        is_connected::is_connected(self, base, branch)
    }

    pub fn find_crown(&self, base: &str) -> Vec<String> {
        find_crown::find_crown(self, base)
    }

    pub fn count_leaves(&self, branch: &str) -> usize {
        count_leaves::count_leaves(self, branch)
    }

    pub fn get_nesting_level(&self, branch: &str) -> i32 {
        get_nesting_level::get_nesting_level(self, branch)
    }

    pub fn sort_nesting_descending(self) -> impl FnMut(&String, &String) -> Ordering {
        sort_nesting_descending::sort_nesting_descending(self)
    }

    pub fn sort_nesting_ascending(self) -> impl FnMut(&String, &String) -> Ordering {
        sort_nesting_ascending::sort_nesting_ascending(self)
    }
}

#[cfg(test)]
mod tests {
    use super::is_well_formed_branch;

    #[test]
    fn accepts_normal_names() {
        for name in ["event", "date", "actName", "tag%40", "a b", "日本語"] {
            assert!(is_well_formed_branch(name), "should accept: {name:?}");
        }
    }

    #[test]
    fn rejects_unsafe_names() {
        for name in [
            "_",
            "",
            "a/b",
            "a\\b",
            "a.b",
            "a-b",
            "..",
            "x/../../secret",
        ] {
            assert!(!is_well_formed_branch(name), "should reject: {name:?}");
        }
    }
}
