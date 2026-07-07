//! csvs command-line interface.
//!
//! `csvs select -q a.json` runs the queries from `a.json` against the
//! dataset in the current directory and streams matched records to
//! stdout as JSON lines.

use clap::{Parser, Subcommand};
use csvs::{Dataset, Entry, Error, Result};
use futures_util::{pin_mut, StreamExt};
use std::io::Write;
use std::path::PathBuf;

#[derive(Parser)]
#[command(name = "csvs", version, about = "Query csvs datasets")]
struct Cli {
    #[command(subcommand)]
    command: Command,
}

#[derive(Subcommand)]
enum Command {
    /// Stream records matching a query file to stdout as JSON lines
    Select {
        /// Query file: a SON query object or an array of them
        #[arg(short, long)]
        query: PathBuf,

        /// Dataset directory
        #[arg(short, long, default_value = ".")]
        dir: PathBuf,

        /// Skip building nested subrecords
        #[arg(long)]
        light: bool,
    },
}

#[tokio::main]
async fn main() -> std::process::ExitCode {
    let cli = Cli::parse();

    let result = match cli.command {
        Command::Select { query, dir, light } => select(query, dir, light).await,
    };

    match result {
        Ok(()) => std::process::ExitCode::SUCCESS,
        Err(error) => {
            eprintln!("csvs: {error}");
            std::process::ExitCode::FAILURE
        }
    }
}

/// Parse the text of a query file: a single SON query (object) or an
/// array of them. A bare object reads as a one-query array.
fn parse_queries(text: &str) -> Result<Vec<serde_json::Value>> {
    let value: serde_json::Value = serde_json::from_str(text)?;

    match value {
        serde_json::Value::Array(queries) => Ok(queries),
        object @ serde_json::Value::Object(_) => Ok(vec![object]),
        other => Err(Error::from_message(format!(
            "query must be a JSON object or an array of objects, got: {other}"
        ))),
    }
}

/// Run the queries against the dataset at `dir`, streaming each
/// matched record to stdout as one JSON line as soon as it lands.
async fn select(query_path: PathBuf, dir: PathBuf, light: bool) -> Result<()> {
    let text = std::fs::read_to_string(&query_path)
        .map_err(|e| Error::with_context(e, format!("failed to read {}", query_path.display())))?;

    let entries = parse_queries(&text)?
        .iter()
        .map(Entry::try_from)
        .collect::<Result<Vec<_>>>()?;

    let dataset = Dataset::open(&dir).await?.with_schema().await?;

    // select is the one-stop entry: it dispatches base-value-only
    // queries to the option scan and leaf-constrained queries
    // (including `~` recursion) to the query planner.
    let stream = dataset.select_record_stream(entries, light);

    pin_mut!(stream);

    let stdout = std::io::stdout();
    let mut out = stdout.lock();

    while let Some(matched) = stream.next().await {
        let value: serde_json::Value = matched?.into();

        serde_json::to_writer(&mut out, &value)?;
        out.write_all(b"\n")?;
        out.flush()?;
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::parse_queries;

    #[test]
    fn array_of_queries() {
        let queries = parse_queries(r#"[{ "_": "name" }, { "_": "event" }]"#).unwrap();

        assert_eq!(queries.len(), 2);
    }

    #[test]
    fn bare_object_reads_as_one_query_array() {
        let queries = parse_queries(r#"{ "_": "name", "name": "john" }"#).unwrap();

        assert_eq!(queries.len(), 1);
        assert_eq!(queries[0]["_"], "name");
    }

    #[test]
    fn scalar_is_rejected() {
        assert!(parse_queries(r#""john""#).is_err());
    }
}
