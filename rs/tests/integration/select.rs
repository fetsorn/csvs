use assert_json_diff::{assert_json_matches_no_panic, Config, CompareMode};
use temp_dir::TempDir;
use serde_json::Value;
use async_stream::try_stream;
use csvs::{
    Result,
    Entry, Dataset
};
use csvs_test::{read_record, read_records, copy, read_testcase};
use serde::{Deserialize, Serialize};
use std::fs;

// make sure that select stream is Send
fn assert_send<T: Send>(_: T) {}

#[tokio::test]
async fn stream_is_send() -> Result<()> {
    let temp_path = TempDir::new()?;

    let dataset = Dataset::create(&temp_path.path().to_owned(), true).await?;

    assert_send(dataset.select_record_stream(vec![Entry::new("")], false));

    Ok(())
}

#[derive(Debug, Serialize, Deserialize, Clone)]
struct SelectTest {
    name: String,
    initial: String,
    query: Vec<String>,
    expected: Vec<String>,
}

#[tokio::test]
async fn select_test() -> Result<()> {
    let tests: Vec<SelectTest> = read_testcase("select");

    for test in tests.iter() {
        let temp_path = TempDir::new()?;

        copy(&test.initial, temp_path.path());

        // parse query to Entry
        let queries: Vec<Entry> = test
            .query
            .iter()
            .map(|query| read_record(&query).try_into())
            .collect::<Result<Vec<Entry>>>()?;

        let dataset = Dataset::open(&temp_path.path().to_owned()).await?;

        let entries = dataset.select_record(queries, false).await?;

        let entries_json: Vec<Value> = entries.iter().map(|i| i.clone().into()).collect();

        let expected_json: Vec<Value> = read_records(&test.expected);

        let r = assert_json_matches_no_panic(&entries_json, &expected_json, Config::new(CompareMode::Strict));

        assert!(r.is_ok(), "{} failed\n{:#?}\n{:#?}", test.name, entries_json, expected_json);
    }

    Ok(())
}
