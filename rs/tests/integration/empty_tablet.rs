use csvs::{Dataset, Entry, Result};
use serde_json::json;
use std::fs;
use temp_dir::TempDir;

/// A data tablet that exists on disk but is empty (0 bytes) must behave
/// exactly like a missing one: for a non-first tablet in a query join it
/// is a match-all passthrough, not a source that filters every candidate
/// out.
///
/// This mirrors the csvs-js contract (`isEmpty` in `stream.js`, which
/// treats size 0 the same as a missing file). Before the fix, the Rust
/// port guarded only on file *existence*, so an existing 0-byte tablet
/// fell through to the reader, produced no key groups, and silently
/// dropped the accumulated query state — returning nothing where csvs-js
/// returns the match.
#[tokio::test]
async fn query_passes_through_empty_intermediate_tablet() -> Result<()> {
    let temp = TempDir::new()?;
    let dir = temp.path().to_owned();

    // bare dataset: version tablet at the root
    fs::write(&dir.join(".csvs.csv"), "version,0.0.4\nid,empty-tablet-test\n")?;

    // schema: `event` has two twig leaves, `date` and `tag`
    fs::write(&dir.join("_-_.csv"), "event,date\nevent,tag\n")?;

    // At equal nesting level, branches sort reverse-alphabetically, so the
    // query plan visits `event-tag` before `event-date`. The populated
    // tablet is therefore first, the empty one second (non-first).
    fs::write(&dir.join("event-tag.csv"), "visited-japan,japan\n")?;

    // an existing but EMPTY intermediate tablet
    fs::write(&dir.join("event-date.csv"), "")?;

    let dataset = Dataset::open(&dir).await?;

    // `date` is constrained too, but its tablet is empty, so that
    // constraint must be ignored (match-all) rather than exclude the row.
    let query: Entry = json!({ "_": "event", "tag": "japan", "date": "unused" }).try_into()?;

    let records = dataset.query_record(query).await?;

    assert_eq!(
        records.len(),
        1,
        "the tag match must survive the empty date tablet, got {records:#?}"
    );
    assert_eq!(records[0].base, "event");
    assert_eq!(records[0].base_value.as_deref(), Some("visited-japan"));

    Ok(())
}
