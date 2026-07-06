use crate::line::Line;
use crate::Result;
use std::fs::File;
use std::path::Path;

/// A group of values sharing the same key in a sorted CSV tablet.
#[derive(Debug, Clone)]
pub struct KeyGroup {
    pub key: String,
    pub values: Vec<String>,
}

/// Layer 1 — reads a sorted CSV tablet and yields key groups.
/// Each group collects all values for a contiguous run of the same key.
/// Pure CSV logic, no csvs domain concepts.
pub fn key_groups(filepath: &Path) -> Result<Vec<KeyGroup>> {
    if std::fs::metadata(filepath).is_err() {
        return Ok(vec![]);
    }

    let mut rdr = csv::ReaderBuilder::new()
        .has_headers(false)
        .flexible(true)
        .from_reader(File::open(filepath)?);

    let mut groups: Vec<KeyGroup> = Vec::new();
    let mut current_key: Option<String> = None;
    let mut current_values: Vec<String> = Vec::new();

    let filename = filepath
        .file_name()
        .and_then(|s| s.to_str())
        .unwrap_or_default();

    for result in rdr.records() {
        let record = result?;

        let line = Line::from_record(filename, &record)?;

        match &current_key {
            Some(k) if *k != line.key => {
                groups.push(KeyGroup {
                    key: k.clone(),
                    values: std::mem::take(&mut current_values),
                });
                current_key = Some(line.key);
                current_values.push(line.value);
            }
            None => {
                current_key = Some(line.key);
                current_values.push(line.value);
            }
            _ => {
                current_values.push(line.value);
            }
        }
    }

    if let Some(key) = current_key {
        groups.push(KeyGroup {
            key,
            values: current_values,
        });
    }

    Ok(groups)
}
