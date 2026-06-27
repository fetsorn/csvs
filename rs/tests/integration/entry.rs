use assert_json_diff::{assert_json_matches_no_panic, CompareMode, Config};
use csvs::{Entry, Result};
use csvs_test::read_testcase;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::fs;

#[derive(Debug, Serialize, Deserialize, Clone)]
struct EntryTest {
    name: String,
    value: Value,
    entry: Value,
}

#[test]
fn entry_try_from_test() -> Result<()> {
    let tests: Vec<EntryTest> = read_testcase("entry");

    for test in tests.iter() {
        let result: Entry = test.value.clone().try_into()?;

        let result_string = serde_json::to_string(&result)?;

        let result_json: Value = serde_json::from_str(&result_string)?;

        let r = assert_json_matches_no_panic(
            &result_json,
            &test.entry,
            Config::new(CompareMode::Strict),
        );

        assert!(
            r.is_ok(),
            "{} failed\n{:#?}\n{:#?}",
            test.name,
            result_json,
            test.entry
        );
    }

    Ok(())
}

#[test]
fn entry_into_test() -> Result<()> {
    let tests: Vec<EntryTest> = read_testcase("entry");

    for test in tests.iter() {
        let entry_string = serde_json::to_string(&test.entry)?;

        let entry: Entry = serde_json::from_str(&entry_string)?;

        let result: Value = entry.into();

        let r =
            assert_json_matches_no_panic(&result, &test.value, Config::new(CompareMode::Strict));

        assert!(
            r.is_ok(),
            "{} failed\n{:#?}\n{:#?}",
            test.name,
            result,
            test.value
        );
    }

    Ok(())
}
