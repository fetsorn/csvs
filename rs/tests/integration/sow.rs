use assert_json_diff::{assert_json_matches_no_panic, CompareMode, Config};
use csvs::{Entry, Grain, Result};
use csvs_test::{read_record, read_testcase};
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::fs;

#[derive(Debug, Serialize, Deserialize, Clone)]
struct SowTest {
    name: String,
    initial: String,
    grain: String,
    trunk: String,
    branch: String,
    expected: String,
}

#[test]
fn sow_test() -> Result<()> {
    let tests: Vec<SowTest> = read_testcase("sow");

    for test in tests.iter() {
        let entry: Entry = read_record(&test.initial).try_into()?;

        let grain: Grain = read_record(&test.grain).try_into()?;

        let result: Entry = entry.sow(&grain, &test.trunk, &test.branch);

        let result_json: Value = result.into();

        let expected_json: Value = read_record(&test.expected);

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
