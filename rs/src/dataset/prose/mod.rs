use crate::Result;
use std::collections::HashMap;
use std::path::{Path, PathBuf};

/// Addressing strategy for the prose store.
#[derive(Debug, Clone)]
pub enum ProseAddress {
    /// URI-encoded filenames: `prose/{encoded_value}[.lang]`
    Uri,
}

impl Default for ProseAddress {
    fn default() -> Self {
        ProseAddress::Uri
    }
}

impl ProseAddress {
    /// Resolve a value + optional language tag to a filesystem path.
    pub fn path(&self, dir: &Path, value: &str, lang: Option<&str>) -> PathBuf {
        let encoded = match self {
            ProseAddress::Uri => uri_encode(value),
        };

        let filename = match lang {
            None => encoded,
            Some(l) => format!("{}.{}", encoded, l),
        };

        dir.join("@").join(filename)
    }

    /// Read all prose for a value (untagged + all language tags).
    pub fn read_prose(&self, dir: &Path, value: &str) -> Result<HashMap<Option<String>, String>> {
        let mut result = HashMap::new();

        // Try untagged
        let untagged_path = self.path(dir, value, None);

        if untagged_path.is_file() {
            let content = std::fs::read_to_string(&untagged_path)?;
            result.insert(None, content);
        }

        // Scan for language-tagged files
        let prose_dir = dir.join("@");

        if prose_dir.is_dir() {
            let encoded = match self {
                ProseAddress::Uri => uri_encode(value),
            };

            let prefix = format!("{}.", encoded);

            for entry in std::fs::read_dir(&prose_dir)? {
                let entry = entry?;
                let name = entry.file_name();
                let name = name.to_string_lossy();

                if name.starts_with(&prefix) && name.len() > prefix.len() {
                    let lang = name[prefix.len()..].to_string();
                    let content = std::fs::read_to_string(entry.path())?;
                    result.insert(Some(lang), content);
                }
            }
        }

        Ok(result)
    }

    /// Write a single prose blob.
    pub fn write_prose(
        &self,
        dir: &Path,
        value: &str,
        lang: Option<&str>,
        content: &str,
    ) -> Result<()> {
        if let Some(l) = lang {
            if !is_well_formed_lang(l) {
                return Err(crate::Error::from_message(format!(
                    "prose language tag is not a well-formed BCP 47 tag: {l:?}"
                )));
            }
        }

        let path = self.path(dir, value, lang);

        if let Some(parent) = path.parent() {
            std::fs::create_dir_all(parent)?;
        }

        std::fs::write(&path, content)?;

        Ok(())
    }

    /// Search prose files for content matching a regex pattern.
    /// Returns values whose prose matches.
    pub fn search_prose(
        &self,
        dir: &Path,
        pattern: &str,
        lang: Option<&str>,
    ) -> Result<Vec<String>> {
        let prose_dir = dir.join("@");

        if !prose_dir.is_dir() {
            return Ok(vec![]);
        }

        let re = regex::Regex::new(pattern)
            .map_err(|e| crate::Error::from_message(format!("invalid prose regex: {}", e)))?;

        let mut matches = vec![];

        for entry in std::fs::read_dir(&prose_dir)? {
            let entry = entry?;
            let name = entry.file_name();
            let name_str = name.to_string_lossy();

            // Filter by language tag if specified
            let (value_encoded, file_lang) = match name_str.rfind('.') {
                Some(dot) => {
                    let before = &name_str[..dot];
                    let after = &name_str[dot + 1..];
                    (before.to_string(), Some(after.to_string()))
                }
                None => (name_str.to_string(), None),
            };

            // "@" (lang=None) searches all prose regardless of tag;
            // "@en" etc. matches only that specific language.
            let lang_matches = match lang {
                None => true,
                Some(l) => file_lang.as_deref() == Some(l),
            };

            if !lang_matches {
                continue;
            }

            let content = std::fs::read_to_string(entry.path())?;

            if re.is_match(&content) {
                let value = match self {
                    ProseAddress::Uri => uri_decode(&value_encoded),
                };
                matches.push(value);
            }
        }

        Ok(matches)
    }
}

/// Whether `lang` is a well-formed BCP 47 language tag: a controlled
/// vocabulary of alphanumeric subtags (1-8 chars each) joined by hyphens.
///
/// The prose filename is `{uri_encode(value)}.{lang}`. The value is
/// percent-encoded, but the tag is spliced in verbatim, so an
/// unconstrained tag (e.g. "../../evil") would escape the "@" prose
/// directory and allow arbitrary file writes. Restricting the tag to
/// this vocabulary — which excludes '/', '\\', '.', and empty subtags —
/// closes that path-traversal vector.
pub fn is_well_formed_lang(lang: &str) -> bool {
    !lang.is_empty()
        && lang.split('-').all(|subtag| {
            (1..=8).contains(&subtag.len()) && subtag.chars().all(|c| c.is_ascii_alphanumeric())
        })
}

/// Percent-encode filesystem-reserved characters.
fn uri_encode(value: &str) -> String {
    let mut encoded = String::with_capacity(value.len());

    for ch in value.chars() {
        match ch {
            '/' | '\\' | '<' | '>' | ':' | '"' | '|' | '?' | '*' | '.' | '%' | '\0' => {
                for byte in ch.to_string().as_bytes() {
                    encoded.push_str(&format!("%{:02X}", byte));
                }
            }
            _ => encoded.push(ch),
        }
    }

    encoded
}

/// Decode percent-encoded characters.
fn uri_decode(value: &str) -> String {
    let mut decoded = Vec::new();
    let bytes = value.as_bytes();
    let mut i = 0;

    while i < bytes.len() {
        if bytes[i] == b'%' && i + 2 < bytes.len() {
            if let Ok(byte) = u8::from_str_radix(&value[i + 1..i + 3], 16) {
                decoded.push(byte);
                i += 3;
                continue;
            }
        }
        decoded.push(bytes[i]);
        i += 1;
    }

    String::from_utf8_lossy(&decoded).to_string()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn uri_encode_roundtrip() {
        let values = vec![
            "visited-japan",
            "hello world",
            "path/to/thing",
            "file:name",
            "value.with.dots",
            "100%",
        ];

        for v in values {
            assert_eq!(uri_decode(&uri_encode(v)), v, "roundtrip failed for: {}", v);
        }
    }

    #[test]
    fn uri_encode_simple_value_unchanged() {
        assert_eq!(uri_encode("visited-japan"), "visited-japan");
    }

    #[test]
    fn uri_encode_dots() {
        // Dots are encoded to avoid collision with language tag suffix
        assert_eq!(uri_encode("2001-01-01"), "2001-01-01");
        assert!(uri_encode("file.txt").contains("%"));
    }

    #[test]
    fn lang_vocabulary_accepts_bcp47() {
        for l in ["en", "EN", "ru", "en-US", "zh-Hant-CN", "x-custom", "a1b2c3d4"] {
            assert!(is_well_formed_lang(l), "should accept: {l}");
        }
    }

    #[test]
    fn lang_vocabulary_rejects_traversal_and_junk() {
        for l in [
            "",
            "..",
            "../../evil",
            "en/../x",
            "en/us",
            "en.US",
            "toolongsubtag",
            "en-",
            "-en",
            "en--us",
        ] {
            assert!(!is_well_formed_lang(l), "should reject: {l:?}");
        }
    }

    #[test]
    fn write_prose_rejects_traversal_lang() {
        let dir = std::env::temp_dir().join(format!("csvs-prose-{}", std::process::id()));
        std::fs::create_dir_all(dir.join("@")).unwrap();

        let store = ProseAddress::Uri;
        let result = store.write_prose(&dir, "x", Some("../../../../PWNED"), "owned");

        assert!(result.is_err(), "traversal lang must be rejected");

        let escaped = dir.join("..").join("..").join("..").join("..").join("PWNED");
        assert!(!escaped.exists(), "no file may be written outside the store");

        std::fs::remove_dir_all(&dir).ok();
    }
}
