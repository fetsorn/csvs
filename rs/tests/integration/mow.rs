use assert_json_diff::{assert_json_matches_no_panic, CompareMode, Config};
use csvs::{Entry, Grain, Result};
use csvs_test::{read_record, read_records, read_testcase};
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::fs;

#[derive(Debug, Serialize, Deserialize, Clone)]
struct MowTest {
    name: String,
    initial: String,
    trunk: String,
    branch: String,
    expected: Vec<String>,
}

#[test]
fn mow_test() -> Result<()> {
    let tests: Vec<MowTest> = read_testcase("mow");

    for test in tests.iter() {
        let entry: Entry = read_record(&test.initial).try_into()?;

        let result: Vec<Grain> = entry.mow(&test.trunk, &test.branch);

        let result_json: Vec<Value> = result.into_iter().map(|i| i.into()).collect();

        let expected_json: Vec<Value> = read_records(&test.expected);

        let r = assert_json_matches_no_panic(
            &result_json,
            &expected_json,
            Config::new(CompareMode::Strict),
        );

        assert!(
            r.is_ok(),
            "{} failed\n{:#?}\n{:#?}",
            test.name,
            result_json,
            expected_json
        );
    }

    Ok(())
}
