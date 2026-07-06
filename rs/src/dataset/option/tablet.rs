use super::line::{option_line, State};
use super::strategy::Tablet;
use crate::{line::Line, Dataset, Entry, Result};
use async_stream::try_stream;
use futures_core::stream::Stream;
use std::fs::File;

pub fn option_tablet_stream(
    dataset: Dataset,
    tablet: Tablet,
    state: State,
) -> impl Stream<Item = Result<State>> {
    try_stream! {
        let filepath = dataset.dir.join(&tablet.filename);

        if std::fs::metadata(&filepath).is_err() {
            return;
        }

        let mut rdr = csv::ReaderBuilder::new()
            .has_headers(false)
            .flexible(true)
            .from_reader(File::open(&filepath)?);

        let mut state = State {
            query: state.query.clone(),
            entry: Entry::new(&tablet.base),
            fst: None,
            is_match: false,
            match_map: state.match_map.clone(),
            last: None,
        };

        for result in rdr.records() {
            let record = result?;

            let line = Line::from_record(&tablet.filename, &record)?;

            state = option_line(tablet.clone(), state, line)?;

            if state.last.is_some()  {
                yield state.clone();

                state.last = None;
            }
        }

        if state.is_match {
            state.last = Some(state.clone().entry);

            yield state;
        }
    }
}
