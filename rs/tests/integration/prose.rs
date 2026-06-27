use assert_json_diff::{assert_json_matches_no_panic, Config, CompareMode};
use temp_dir::TempDir;
use serde_json::Value;
use csvs::{
    Result,
    Entry, Dataset
};
use csvs_test::{read_record, read_records, copy, read_testcase};
use serde::{Deserialize, Serialize};
use std::fs;

#[derive(Debug, Serialize, Deserialize, Clone)]
struct ExpectedBlob {
    path: String,
    content: String,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
struct ProseTest {
    name: String,
    op: String,
    initial: String,
    query: Vec<String>,
    #[serde(default)]
    prose: Option<bool>,
    #[serde(default)]
    expected: Option<Vec<String>>,
    #[serde(default)]
    expected_blob: Option<ExpectedBlob>,
    #[serde(default)]
    expected_blobs: Option<Vec<ExpectedBlob>>,
}

fn check_blobs(temp_path: &std::path::Path, blobs: &[ExpectedBlob], test_name: &str) {
    for blob in blobs {
        let blob_path = temp_path.join(&blob.path);
        assert!(blob_path.exists(), "{}: blob {} should be written", test_name, blob.path);

        let content = fs::read_to_string(&blob_path)
            .unwrap_or_else(|e| panic!("{}: failed to read {}: {}", test_name, blob.path, e));
        assert_eq!(content, blob.content, "{}: blob {} content mismatch", test_name, blob.path);
    }
}

#[tokio::test]
async fn prose_test() -> Result<()> {
    let tests: Vec<ProseTest> = read_testcase("prose");

    for test in tests.iter() {
        let temp_path = TempDir::new()?;
        copy(&test.initial, temp_path.path());

        let queries: Vec<Entry> = test
            .query
            .iter()
            .map(|q| read_record(q).try_into())
            .collect::<Result<Vec<Entry>>>()?;

        let dataset = Dataset::open(&temp_path.path().to_owned()).await?;

        match test.op.as_str() {
            "build" => {
                let query = queries.into_iter().next().unwrap();
                let entry = if test.prose.unwrap_or(false) {
                    dataset.build_record_with_prose(query).await?
                } else {
                    dataset.build_record(query).await?
                };
                let entry_json: Value = entry.into();
                let expected_json = read_record(&test.expected.as_ref().unwrap()[0]);

                let r = assert_json_matches_no_panic(
                    &entry_json, &expected_json, Config::new(CompareMode::Strict),
                );
                assert!(r.is_ok(), "{} failed\n{:#?}\n{:#?}", test.name, entry_json, expected_json);
            }
            "select" => {
                let entries = dataset.select_record(queries, true).await?;
                let entries_json: Vec<Value> = entries.iter().map(|i| i.clone().into()).collect();
                let expected_json = read_records(test.expected.as_ref().unwrap());

                let r = assert_json_matches_no_panic(
                    &entries_json, &expected_json, Config::new(CompareMode::Strict),
                );
                assert!(r.is_ok(), "{} failed\n{:#?}\n{:#?}", test.name, entries_json, expected_json);
            }
            "insert" => {
                dataset.insert_record(queries).await?;

                if let Some(ref blob) = test.expected_blob {
                    check_blobs(temp_path.path(), &[blob.clone()], &test.name);
                }
                if let Some(ref blobs) = test.expected_blobs {
                    check_blobs(temp_path.path(), blobs, &test.name);
                }
            }
            "update" => {
                dataset.update_record(queries).await?;

                if let Some(ref blob) = test.expected_blob {
                    check_blobs(temp_path.path(), &[blob.clone()], &test.name);
                }
                if let Some(ref blobs) = test.expected_blobs {
                    check_blobs(temp_path.path(), blobs, &test.name);
                }
            }
            _ => panic!("unknown op: {}", test.op),
        }
    }

    Ok(())
}

#[tokio::test]
async fn update_with_nested_prose() -> Result<()> {
    let temp_path = TempDir::new()?;
    copy("prose_nested_empty", temp_path.path());

    let record: Entry = read_record("record_prose_nested_update").try_into()?;
    let dataset = Dataset::open(&temp_path.path().to_owned()).await?;
    dataset.update_record(vec![record]).await?;

    // Check that nested prose blobs were written
    let prose_dir = temp_path.path().join("@");
    assert!(prose_dir.exists(), "prose dir should exist");

    let event_en = fs::read_to_string(prose_dir.join("event.en"))?;
    assert_eq!(event_en, "Record");

    let event_ru = fs::read_to_string(prose_dir.join("event.ru"))?;
    assert_eq!(event_ru, "Запись");

    let actdate_en = fs::read_to_string(prose_dir.join("actdate.en"))?;
    assert_eq!(actdate_en, "Date of the event");

    // Verify tablets were written without @ keys
    let tablet_path = temp_path.path().join("mind-branch.csv");
    let tablet_content = fs::read_to_string(&tablet_path)?;
    assert!(tablet_content.contains("abc123"), "tablet should contain the mind");
    assert!(!tablet_content.contains("Record"), "tablet should not contain prose content");

    Ok(())
}

#[tokio::test]
async fn build_nested_prose_with_into_value() -> Result<()> {
    let temp_path = TempDir::new()?;
    copy("prose_nested", temp_path.path());

    let query: Entry = read_record("query_prose_mind").try_into()?;
    let dataset = Dataset::open(&temp_path.path().to_owned()).await?;
    let entry = dataset.build_record_with_prose(query).await?;
    let entry_json: Value = entry.into();

    // The branch leaves should be objects with @en/@ru, not collapsed to strings
    let branches = entry_json.get("branch").expect("should have branch");
    let branch_array = branches.as_array().expect("branch should be array");

    // Find event branch
    let event_branch = branch_array.iter().find(|b| {
        b.get("branch").and_then(|v| v.as_str()) == Some("event")
    }).expect("should have event branch");

    assert_eq!(event_branch.get("@en").and_then(|v| v.as_str()), Some("Record"),
        "event branch should have @en prose");
    assert_eq!(event_branch.get("@ru").and_then(|v| v.as_str()), Some("Запись"),
        "event branch should have @ru prose");

    // Find actdate branch
    let actdate_branch = branch_array.iter().find(|b| {
        b.get("branch").and_then(|v| v.as_str()) == Some("actdate")
    }).expect("should have actdate branch");

    assert_eq!(actdate_branch.get("@en").and_then(|v| v.as_str()), Some("Date of the event"),
        "actdate branch should have @en prose");

    Ok(())
}
