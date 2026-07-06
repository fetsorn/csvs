use csvs::{Dataset, Entry, Result};
use serde_json::json;
use std::fs;
use temp_dir::TempDir;

/// Regression (mirrors csvs-js): a data tablet contained a line without
/// a comma (a key fused with the next value). The readers defaulted the
/// missing field to "" and silently built corrupt records. A malformed
/// line must abort with an error naming the tablet and the line.
fn make_store(dir: &std::path::Path) -> Result<()> {
    fs::write(
        &dir.join(".csvs.csv"),
        "version,0.0.4\nid,malformed-line-test\n",
    )?;

    fs::write(&dir.join("_-_.csv"), "event,actname\n")?;

    fs::write(
        &dir.join("event-actname.csv"),
        "event1,name1\nevent2name2\nevent3,name3\n",
    )?;

    Ok(())
}

#[tokio::test]
async fn select_reports_malformed_line() -> Result<()> {
    let temp = TempDir::new()?;
    let dir = temp.path().to_owned();

    make_store(&dir)?;

    let dataset = Dataset::open(&dir).await?;

    let query: Entry = json!({ "_": "event" }).try_into()?;

    let err = match dataset.select_record(vec![query], false).await {
        Ok(records) => panic!("expected an error, got {records:#?}"),
        Err(e) => e.to_string(),
    };

    assert!(
        err.contains("malformed line in event-actname.csv"),
        "error must name the tablet: {err}"
    );

    assert!(
        err.contains("event2name2"),
        "error must show the offending line: {err}"
    );

    Ok(())
}

#[tokio::test]
async fn light_select_reports_malformed_line() -> Result<()> {
    let temp = TempDir::new()?;
    let dir = temp.path().to_owned();

    make_store(&dir)?;

    let dataset = Dataset::open(&dir).await?;

    let query: Entry = json!({ "_": "event", "actname": "name" }).try_into()?;

    let err = match dataset.select_record(vec![query], true).await {
        Ok(records) => panic!("expected an error, got {records:#?}"),
        Err(e) => e.to_string(),
    };

    assert!(
        err.contains("malformed line in event-actname.csv"),
        "error must name the tablet: {err}"
    );

    Ok(())
}
