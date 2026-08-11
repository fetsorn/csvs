<div align="center">

part of the [ontonomy](https://norcivilianlabs.org) software suite

</div>

# csvs

A plain-text relational database. A csvs dataset is a directory
of two-column CSV files called tablets. The schema tablet
`_-_.csv` names the relationships between collections. Data
tablets hold the values.

```
_-_.csv              name-age.csv       name-city.csv
name,age             john,35            john,Bath
name,city            jane,36            john,London
```

This is the Rust implementation (library + CLI).

Full format description: [spec](https://norcivilianlabs.org).
Source and other implementations: [codeberg.org/fetsorn/csvs](https://codeberg.org/fetsorn/csvs).

## Install

```
cargo install --git https://codeberg.org/fetsorn/csvs
```

## CLI

```sh
# all names
csvs select -q query.json

# from a specific dataset
csvs select -q query.json -d /path/to/dataset

# skip building nested subrecords
csvs select -q query.json --light
```

Streams matched records to stdout as JSON lines.

A query file is a JSON object or array:

```json
{ "_": "name", "name": "john" }
```

## Library

```rust
use csvs::{Dataset, Entry};

let dataset = Dataset::open(&dir).await?.with_schema().await?;
let stream = dataset.select_record_stream(entries, false);
```

AGPL-3.0. Anton Davydov.
