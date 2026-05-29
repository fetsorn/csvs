use assert_json_diff::{assert_json_matches_no_panic, Config, CompareMode};
use temp_dir::TempDir;
use serde_json::{json, Value};
use csvs::{
    Result,
    Entry, IntoValue, Dataset
};
use csvs_test::{read_record, read_records, copy};
use std::fs;

#[tokio::test]
async fn build_without_prose_flag() -> Result<()> {
    let temp_path = TempDir::new()?;
    copy("prose_default", temp_path.path());

    let query: Entry = read_record("query_prose_japan").try_into()?;
    let dataset = Dataset::open(&temp_path.path().to_owned()).await?;
    let entry = dataset.build_record(query).await?;
    let entry_json = entry.into_value();

    let expected_json = read_record("record_prose_japan_no_prose");

    let r = assert_json_matches_no_panic(&entry_json, &expected_json, Config::new(CompareMode::Strict));
    assert!(r.is_ok(), "build without prose flag should not include @ keys\n{:#?}\n{:#?}", entry_json, expected_json);

    Ok(())
}

#[tokio::test]
async fn build_with_prose_flag() -> Result<()> {
    let temp_path = TempDir::new()?;
    copy("prose_default", temp_path.path());

    let query: Entry = read_record("query_prose_japan").try_into()?;
    let dataset = Dataset::open(&temp_path.path().to_owned()).await?;
    let entry = dataset.build_record_with_prose(query).await?;
    let entry_json = entry.into_value();

    let expected_json = read_record("record_prose_japan");

    let r = assert_json_matches_no_panic(&entry_json, &expected_json, Config::new(CompareMode::Strict));
    assert!(r.is_ok(), "build with prose flag should include @ keys\n{:#?}\n{:#?}", entry_json, expected_json);

    Ok(())
}

#[tokio::test]
async fn build_with_prose_flag_no_blobs() -> Result<()> {
    let temp_path = TempDir::new()?;
    copy("prose_default", temp_path.path());

    // climbed-everest has no prose blobs
    let query: Entry = read_record("query_prose_japan").try_into()?;
    let dataset = Dataset::open(&temp_path.path().to_owned()).await?;

    // build everest directly
    let everest = Entry::new("event");
    let everest = Entry {
        base: "event".to_string(),
        base_value: Some("climbed-everest".to_string()),
        leader_value: None,
        leaves: std::collections::HashMap::new(),
        prose: std::collections::HashMap::new(),
    };
    let entry = dataset.build_record_with_prose(everest).await?;
    let entry_json = entry.into_value();

    let expected_json = read_record("record_prose_everest");

    let r = assert_json_matches_no_panic(&entry_json, &expected_json, Config::new(CompareMode::Strict));
    assert!(r.is_ok(), "build with prose flag should work when no blobs exist\n{:#?}\n{:#?}", entry_json, expected_json);

    Ok(())
}

#[tokio::test]
async fn search_by_untagged_prose() -> Result<()> {
    let temp_path = TempDir::new()?;
    copy("prose_default", temp_path.path());

    let query: Entry = read_record("query_prose_search").try_into()?;
    let dataset = Dataset::open(&temp_path.path().to_owned()).await?;
    let entries = dataset.select_record(vec![query], true).await?;
    let entries_json: Vec<Value> = entries.iter().map(|i| i.clone().into_value()).collect();

    let expected_json = read_records(&vec!["record_prose_japan_light".to_string()]);

    let r = assert_json_matches_no_panic(&entries_json, &expected_json, Config::new(CompareMode::Strict));
    assert!(r.is_ok(), "search by untagged prose\n{:#?}\n{:#?}", entries_json, expected_json);

    Ok(())
}

#[tokio::test]
async fn search_by_tagged_prose() -> Result<()> {
    let temp_path = TempDir::new()?;
    copy("prose_default", temp_path.path());

    let query: Entry = read_record("query_prose_search_ru").try_into()?;
    let dataset = Dataset::open(&temp_path.path().to_owned()).await?;
    let entries = dataset.select_record(vec![query], true).await?;
    let entries_json: Vec<Value> = entries.iter().map(|i| i.clone().into_value()).collect();

    let expected_json = read_records(&vec!["record_prose_japan_light".to_string()]);

    let r = assert_json_matches_no_panic(&entries_json, &expected_json, Config::new(CompareMode::Strict));
    assert!(r.is_ok(), "search by tagged prose\n{:#?}\n{:#?}", entries_json, expected_json);

    Ok(())
}

#[tokio::test]
async fn insert_with_prose() -> Result<()> {
    let temp_path = TempDir::new()?;
    copy("prose_default", temp_path.path());

    let record: Entry = read_record("record_prose_insert").try_into()?;
    let dataset = Dataset::open(&temp_path.path().to_owned()).await?;
    dataset.insert_record(vec![record]).await?;

    // Check that the blob was written
    let blob_path = temp_path.path().join("@").join("moved-to-bath.en");
    assert!(blob_path.exists(), "prose blob should be written");

    let content = fs::read_to_string(&blob_path)?;
    assert_eq!(content, "Relocated to Bath for work");

    // Check that the tablet was written without @ keys
    let tablet_path = temp_path.path().join("event-date.csv");
    let tablet_content = fs::read_to_string(&tablet_path)?;
    assert!(tablet_content.contains("moved-to-bath"), "tablet should contain the event");
    assert!(!tablet_content.contains("Relocated"), "tablet should not contain prose content");

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
    let entry_json = entry.into_value();

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
