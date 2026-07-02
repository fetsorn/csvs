use crate::{Result, Dataset, Entry, Schema, line::Line};
use std::collections::HashMap;
use std::fs::File;

pub async fn select_schema(dataset: &Dataset) -> Result<Entry> {
    let filepath = dataset.dir.join("_-_.csv");

    let mut entry = Entry::new("_");

    if std::fs::metadata(&filepath).is_err() {
        return Ok(entry);
    }

    let mut rdr = csv::ReaderBuilder::new()
        .has_headers(false)
        .flexible(true)
        .from_reader(File::open(&filepath)?);

    for result in rdr.records() {
        let record = result?;

        let trunk = match record.get(0) { None => String::from(""), Some(s) => s.to_owned() };

        let leaf = match record.get(1) { None => String::from(""), Some(s) => s.to_owned() };

        let leaves = match entry.leaves.get(&trunk) { None => vec![], Some(ls) => ls.to_vec() };

        // append leaf
        let leaves_new = [leaves.clone(), vec![Entry {
            base: trunk.to_owned(),
            base_value: Some(leaf.to_owned()),
            leader_value: None,
            leaves: HashMap::new(),
            prose: HashMap::new(),
        }]].concat();

        // set leaves of trunk
        entry.leaves.insert(trunk.to_owned(), leaves_new);
    }

    Ok(entry)
}

pub async fn build_schema(dataset: &Dataset) -> Result<Schema> {
    let schema_record = dataset.select_schema().await?;

    Ok(schema_record.try_into()?)
}

pub async fn update_schema(dataset: &Dataset, query: Entry) -> Result<()> {
    let filepath = dataset.dir.join("_-_.csv");

    // reject collection names that would be unsafe as tablet filenames
    // before writing them to the schema tablet (see schema::is_well_formed_branch)
    for (trunk, leaves) in query.leaves.iter() {
        crate::schema::assert_well_formed_branch(trunk)?;

        for leaf in leaves.iter().filter_map(|e| e.base_value.as_ref()) {
            crate::schema::assert_well_formed_branch(leaf)?;
        }
    }

    let filepath = File::create(&filepath)?;

    let mut wtr = csv::WriterBuilder::new()
        .has_headers(false)
        .from_writer(filepath);

    let mut keys: Vec<String> = query.leaves.clone().into_keys().collect();

    keys.sort();

    let mut lines: Vec<Line> = vec![];

    for trunk in keys {
        let leaves = query.leaves.get(&trunk).unwrap();

        for entry in leaves {
            let line = Line {
                key: entry.base.clone(),
                value: entry.base_value.clone().unwrap(),
            };

            lines.push(line);
        }
    }

    lines.sort();

    for line in lines {
        wtr.serialize(line)?;
    }

    wtr.flush()?;

    Ok(())
}
