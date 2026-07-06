use crate::{Entry, Error, line::Line, Result};
use std::collections::HashMap;
use std::fs;
use std::fs::File;
use std::fs::OpenOptions;
use std::path::PathBuf;
use temp_dir::TempDir;
use super::strategy::Tablet;
use super::line::{State, update_line};

pub async fn append_tablet(path: PathBuf, tablet: Tablet, entry: Entry, temp_path: PathBuf) -> Result<()> {
    File::create(&temp_path)?;

    let temp_file = OpenOptions::new()
        .append(true)
        .open(&temp_path)
        .expect("cannot open file");

    let mut wtr = csv::WriterBuilder::new()
        .has_headers(false)
        .from_writer(temp_file);

    let filepath = path.join(&tablet.filename);

    if fs::metadata(&filepath).is_err() { File::create(&filepath)?; }

    let mut rdr = csv::ReaderBuilder::new()
        .has_headers(false)
        .flexible(true)
        .from_reader(File::open(&filepath)?);

    let grains = entry.mow(&tablet.trunk, &tablet.branch);

    let mut keys: Vec<String> = grains
        .iter()
        .map(|grain| match &grain.base_value {
            None => "".to_owned(),
            Some(s) => s.to_owned(),
        })
        .collect::<std::collections::HashSet<_>>()
        .into_iter()
        .collect::<Vec<String>>();

    keys.sort();

    let values: HashMap<String, Vec<String>> =
        grains.iter().fold(HashMap::new(), |with_grain, grain| {
            let key = match &grain.base_value {
                None => "",
                Some(s) => s,
            };

            if grain.leaf_value.is_none() {
                return with_grain;
            }

            let value = match &grain.leaf_value {
                None => "",
                Some(s) => s,
            };

            let values_old: Vec<String> = match with_grain.get(key) {
                None => vec![],
                Some(vs) => vs.to_vec(),
            };

            let mut values_new = [&values_old[..], &[value.to_owned()]].concat();

            values_new.sort();

            let mut with_grain_new = with_grain;

            with_grain_new.insert(key.to_owned(), values_new);

            with_grain_new
        });

    let mut state = State {
        fst: None,
        is_match: false,
        keys: keys.clone(),
        keys_inserted: vec![],
    };

    for result in rdr.records() {
        let record = result?;

        let line = Line::from_record(&tablet.filename, &record)?;

        state = update_line(state.clone(), line.clone())?;

        for key in &state.keys_inserted {
            match values.get(key) {
                None => (),
                Some(vs) => {
                    for value in vs {
                        let line_new = Line {
                            key: key.to_owned(),
                            value: value.to_owned()
                        };

                        let line_escaped = line_new.escape();

                        //println!("{} {:#?}", tablet.filename, line_escaped);

                        wtr.serialize(line_escaped)?;

                        wtr.flush()?;
                    }
                }
            }

            state.keys = state.keys.iter().filter(|k| *k != key).cloned().collect();
        }

        if state.is_match {
            // if keys include this key, prune line
            // it will be inserted again before the next key
        } else {
            // otherwise write line unchanged
            wtr.serialize(line.escape())?;

            wtr.flush()?;
        }
    }

    for key in state.keys.clone() {
        match values.get(&key) {
            None => (),
            Some(vs) => {
                for value in vs {
                    let line_new = Line {
                        key: key.to_owned(),
                        value: value.to_owned()
                    };

                    let line_escaped = line_new.escape();

                    wtr.serialize(line_escaped)?;

                    wtr.flush()?;
                }
            }
        }

        state.keys = state.keys.iter().filter(|k| **k != key).cloned().collect();
    }

    Ok(())
}

pub async fn update_tablet(path: PathBuf, tablet: Tablet, entry: Entry) -> Result<()> {
    let filepath = path.join(&tablet.filename);

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

    // wrap in result here for try_stream! proc to pick up error from ?
    let filename = match filepath.file_name() {
        None => Err(Error::from_message("unexpected missing filename")),
        Some(s) => Ok(s)
    }?;

    let temp_path = temp_d.as_ref().join(filename);

    append_tablet(path, tablet, entry, temp_path.clone()).await?;

    match fs::metadata(&temp_path) {
        Err(_) => (),
        Ok(m) => if m.len() == 0 {
            if fs::metadata(&filepath).is_ok() {
                fs::remove_file(&filepath)?;
            }
        } else {
            // fs::rename fails with invalid cross-device link
            fs::copy(temp_path, &filepath)?;
        }
    }

    match fs::metadata(&filepath) {
        Err(_) => return Ok(()),
        Ok(m) => if m.len() == 0 {
            fs::remove_file(filepath)?;
        }
    }

    Ok(())
}
