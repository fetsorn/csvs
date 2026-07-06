use crate::{Entry, line::Line, Result};
use std::path::PathBuf;
use std::fs::File;
use super::strategy::Tablet;
use super::line::{State, build_line};

pub async fn build_tablet(path: PathBuf, tablet: Tablet, entry: Entry) -> Result<Entry> {
    let filepath = path.join(&tablet.filename);

    if std::fs::metadata(&filepath).is_err() {
        return Ok(entry);
    }

    let mut rdr = csv::ReaderBuilder::new()
        .has_headers(false)
        .flexible(true)
        .from_reader(File::open(&filepath)?);

    let grains = entry.mow(&tablet.trait_, &tablet.thing);

    let mut state = State {
        entry,
        fst: None,
        is_match: false,
        last: None,
    };

    for result in rdr.records() {
        let record = result?;

        let line = Line::from_record(&tablet.filename, &record)?;

        state = build_line(tablet.clone(), grains.clone(), state.clone(), line)?;

        if state.last.is_some()  {
            return Ok(state.last.unwrap());
        }
    }

    // if matched, push to the next tablet
    // if not matched, still push to the next tablet
    Ok(state.entry)
}
