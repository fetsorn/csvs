use std::{fmt, io};

use serde::{Serialize, Serializer};

pub type Result<T> = std::result::Result<T, Error>;

#[derive(Debug)]
pub struct Error {
    inner: Box<dyn std::error::Error + Send + Sync>,
}

#[derive(Debug)]
struct Context {
    message: String,
    error: Error,
}

impl Error {
    pub fn from_message(message: impl ToString) -> Self {
        Error {
            inner: message.to_string().into(),
        }
    }

    pub fn with_context(error: impl Into<Self>, message: impl ToString) -> Self {
        Self::from(Context {
            message: message.to_string(),
            error: error.into(),
        })
    }

    pub fn context(self, message: impl ToString) -> Self {
        Error::with_context(self, message)
    }
}

impl From<String> for Error {
    fn from(msg: String) -> Error {
        Error { inner: msg.into() }
    }
}

impl From<&str> for Error {
    fn from(msg: &str) -> Error {
        Error { inner: msg.to_string().into() }
    }
}

impl From<io::Error> for Error {
    fn from(err: io::Error) -> Error {
        Error { inner: err.into() }
    }
}

impl From<fmt::Error> for Error {
    fn from(err: fmt::Error) -> Error {
        Error { inner: err.into() }
    }
}

impl From<Context> for Error {
    fn from(ctx: Context) -> Error {
        Error { inner: ctx.into() }
    }
}

impl From<serde_json::Error> for Error {
    fn from(ctx: serde_json::Error) -> Error {
        Error { inner: ctx.into() }
    }
}

impl From<csv::Error> for Error {
    fn from(ctx: csv::Error) -> Error {
        Error { inner: ctx.into() }
    }
}

impl From<regex::Error> for Error {
    fn from(ctx: regex::Error) -> Error {
        Error { inner: ctx.into() }
    }
}

// TODO: move this to csvs-test by mapping dir_diff errors in the assert_dir macro
impl From<dir_diff::Error> for Error {
    fn from(ctx: dir_diff::Error) -> Error {
        Error { inner: ctx.into() }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        self.inner.fmt(f)
    }
}

impl std::error::Error for Error {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        self.inner.source()
    }
}

impl Serialize for Error {
    fn serialize<S>(&self, serializer: S) -> std::result::Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        #[derive(Serialize)]
        struct JsonError {
            message: String,
            source: Option<Box<JsonError>>,
        }

        fn to_json_error(err: &dyn std::error::Error) -> JsonError {
            JsonError {
                message: err.to_string(),
                source: err.source().map(to_json_error).map(Box::new),
            }
        }

        to_json_error(self).serialize(serializer)
    }
}

impl fmt::Display for Context {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        self.message.fmt(f)
    }
}

impl std::error::Error for Context {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        Some(&self.error)
    }
}

impl serde::de::Error for Error {
    fn custom<T>(msg: T) -> Self
    where
        T: fmt::Display,
    {
        Error::from_message(msg)
    }
}
