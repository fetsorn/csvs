use super::{Branch, Schema, Trunks};
use std::collections::HashSet;

pub fn is_connected(schema: &Schema, base: &str, branch: &str) -> bool {
    is_connected_inner(schema, base, branch, &mut HashSet::new())
}

/// The schema graph may contain cycles: the format explicitly permits
/// recursive relations such as `event,event` or `event -> date -> event`.
/// `visited` records the branches already seen on the current walk so a
/// cycle terminates instead of recursing until the stack overflows.
fn is_connected_inner(
    schema: &Schema,
    base: &str,
    branch: &str,
    visited: &mut HashSet<String>,
) -> bool {
    if branch == base {
        // if branch is base, it is connected
        return true;
    }

    let Branch {
        trunks: Trunks(trunks),
        ..
    } = match schema.0.get(branch) {
        // if schema root is reached, leaf is not connected to base
        None => return false,
        Some(vs) => vs,
    };

    if !visited.insert(branch.to_string()) {
        // already explored this branch on the current walk: a cycle,
        // so this path yields no new way to reach base
        return false;
    }

    for trunk in trunks.iter() {
        if trunk.as_str() == base {
            // if trunk is base, leaf is connected to base
            return true;
        }

        if is_connected_inner(schema, base, trunk, visited) {
            // if trunk is connected to base, leaf is also connected to base
            return true;
        }
    }

    // if trunk is not connected to base, leaf is also not connected to base
    false
}

#[cfg(test)]
mod tests {
    use crate::schema::Schema;
    use serde_json::json;
    use std::convert::TryFrom;

    // a spec-legal self-recursive branch must not overflow the stack
    #[test]
    fn self_recursive_branch_terminates() {
        let schema = Schema::try_from(json!({ "_": "_", "landmark": "landmark" })).unwrap();

        assert!(!schema.is_connected("unrelated", "landmark"));
        assert!(schema.is_connected("landmark", "landmark"));
        assert_eq!(schema.find_crown("landmark"), vec!["landmark".to_string()]);
    }

    // spec example: event <-> date mutual recursion
    #[test]
    fn mutually_recursive_branches_terminate() {
        let schema =
            Schema::try_from(json!({ "_": "_", "event": "date", "date": "event" })).unwrap();

        assert!(!schema.is_connected("unrelated", "event"));
        assert!(schema.is_connected("event", "date"));
        assert!(schema.is_connected("date", "event"));
    }
}
