// A query filter value is a regex. An incomplete pattern — e.g. an
// unbalanced "(" typed into a search bar — must match nothing rather than
// abort the whole select. This contract was established for the
// full-record query path (dataset/query/tablet.rs); these cover the
// option and prose-search paths, which used to propagate the compile
// error and abort the operation.

use csvs::{Dataset, Entry, Result};
use serde_json::{json, Value};
use std::fs;
use temp_dir::TempDir;

fn make_store(temp: &TempDir) {
    let dir = temp.path();
    fs::write(dir.join(".csvs.csv"), "version,0.0.4\nid,test\n").unwrap();
    fs::write(dir.join("_-_.csv"), "event,date\n").unwrap();
    fs::write(dir.join("event-date.csv"), "visited,2001-01-01\n").unwrap();
}

async fn select(dir: &std::path::Path, query: Value) -> Result<Vec<Entry>> {
    let entry: Entry = query.try_into()?;
    let dataset = Dataset::open(&dir.to_owned()).await?;
    dataset.select_record(vec![entry], true).await
}

#[tokio::test]
async fn option_path_invalid_regex_matches_nothing() -> Result<()> {
    let temp = TempDir::new()?;
    make_store(&temp);
    let dir = temp.path();

    // a valid pattern still selects the option
    let ok = select(dir, json!({ "_": "event", "event": "vis" })).await?;
    assert_eq!(ok.len(), 1, "valid regex should select the option");

    // an invalid pattern matches nothing without erroring
    let bad = select(dir, json!({ "_": "event", "event": "vis(" })).await?;
    assert!(bad.is_empty(), "invalid regex should match nothing");

    Ok(())
}

#[tokio::test]
async fn query_path_invalid_regex_matches_nothing() -> Result<()> {
    let temp = TempDir::new()?;
    make_store(&temp);

    let data = select(temp.path(), json!({ "_": "event", "date": "2001(" })).await?;
    assert!(data.is_empty(), "invalid regex should match nothing");

    Ok(())
}

#[tokio::test]
async fn prose_search_invalid_regex_matches_nothing() -> Result<()> {
    let temp = TempDir::new()?;
    make_store(&temp);
    let dir = temp.path();

    fs::create_dir_all(dir.join("@"))?;
    fs::write(dir.join("@").join("visited"), "a long description\n")?;

    // a valid prose regex finds the record
    let ok = select(dir, json!({ "_": "event", "@": "description" })).await?;
    assert_eq!(ok.len(), 1, "valid prose regex should find the record");

    // an invalid prose regex matches nothing without erroring
    let bad = select(dir, json!({ "_": "event", "@": "desc(" })).await?;
    assert!(bad.is_empty(), "invalid prose regex should match nothing");

    Ok(())
}
