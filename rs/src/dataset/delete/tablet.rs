use crate::{line::Line, Entry, Error, Result};
use std::fs;
use std::fs::File;
use std::path::PathBuf;
use temp_dir::TempDir;
use super::strategy::Tablet;

/// Write all lines in the tablet except the "deleted" ones to a temp file
pub async fn filter_lines_to_temp(path: PathBuf, tablet: Tablet, _entry: Entry, temp_path: PathBuf) -> Result<()> {
    let filepath = path.join(&tablet.filename);

    let temp_file = File::create(&temp_path)?;

    let mut wtr = csv::WriterBuilder::new()
        .has_headers(false)
        .flexible(true)
        .from_writer(temp_file);

    let file = File::open(&filepath)?;

    let mut rdr = csv::ReaderBuilder::new()
        .has_headers(false)
        .flexible(true)
        .from_reader(file);

    for result in rdr.records() {
        let record = result?;

        let line = Line::from_record(&tablet.filename, &record)?;

        let trait_ = if tablet.trait_is_first {
            line.key.to_owned()
        } else {
            line.value.to_owned()
        };

        let is_match = trait_ == tablet.trait_;

        if !is_match {
            wtr.serialize(line.escape())?;
        }
    }

    wtr.flush()?;

    Ok(())
}

pub async fn prune_tablet(path: PathBuf, tablet: Tablet, entry: Entry) -> Result<()> {
    let filepath = path.join(&tablet.filename);

    match fs::metadata(&filepath) {
        Err(_) => return Ok(()),
        Ok(m) => {
            if m.len() == 0 {
                return Ok(());
            }
        }
    }

    // must assign a variable to create the directory
    // must assign inside the stream scope to keep the directory
    let temp_d = TempDir::new();

    // on android <13 std::env::temp_dir() returns /data/local/tmp
    // which is inaccessible on some android systems
    let temp_d = match temp_d {
        Err(e) if e.kind() == std::io::ErrorKind::PermissionDenied => {
            TempDir::from_path(path.clone())
        }
        Err(e) => Err(e),
        Ok(td) => Ok(td)
    };

    let temp_d = temp_d?;

    let filename = match filepath.file_name() {
        None => return Err(Error::from_message("unexpected missing filename")),
        Some(s) => s,
    };

    let temp_path = temp_d.as_ref().join(filename);

    filter_lines_to_temp(path, tablet, entry, temp_path.clone()).await?;

    // if empty
    match fs::metadata(&temp_path) {
        Err(_) => fs::remove_file(filepath)?,
        Ok(m) => {
            if m.len() == 0 {
                fs::remove_file(filepath)?;
            } else {
                fs::copy(temp_path, filepath)?;
            }
        }
    }

    Ok(())
}
