use regex::Regex;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize, Ord, Eq, PartialEq, PartialOrd)]
pub struct Line {
    pub key: String,
    pub value: String,
}

fn escape(s: &str) -> String {
    let re = Regex::new(r"\n").unwrap();

    re.replace_all(s, "\\n").to_string()
}

fn unescape(s: &str) -> String {
    let re = Regex::new(r"\\n").unwrap();

    re.replace_all(s, "\n").to_string()
}

impl Line {
    /// Parse one tablet record into an unescaped Line.
    /// A CSVS record is exactly "key,value" — a record that cannot be
    /// split into two fields is malformed data, and silently defaulting
    /// the missing field would corrupt the result far from the cause.
    /// Report the tablet and the offending line instead.
    pub fn from_record(filename: &str, record: &csv::StringRecord) -> crate::Result<Line> {
        match (record.get(0), record.get(1)) {
            (Some(key), Some(value)) => Ok(Line {
                key: key.to_owned(),
                value: value.to_owned(),
            }
            .unescape()),
            _ => {
                let content = record.iter().collect::<Vec<_>>().join(",");

                let position = match record.position() {
                    None => String::new(),
                    Some(p) => format!(" at line {}", p.line()),
                };

                Err(crate::Error::from_message(format!(
                    "malformed line in {filename}: expected two comma-separated fields, got {content:?}{position}"
                )))
            }
        }
    }

    pub fn escape(&self) -> Line {
        Line {
            key: escape(&self.key),
            value: escape(&self.value),
        }
    }

    pub fn unescape(&self) -> Line {
        Line {
            key: unescape(&self.key),
            value: unescape(&self.value),
        }
    }
}
