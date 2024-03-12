use ytflow::{data::DataError, resource::ResourceError};

pub struct ConnectError(pub String);

impl From<&'_ str> for ConnectError {
    fn from(error: &str) -> Self {
        ConnectError(error.into())
    }
}
impl From<String> for ConnectError {
    fn from(error: String) -> Self {
        ConnectError(error)
    }
}

impl From<windows::core::Error> for ConnectError {
    fn from(error: windows::core::Error) -> Self {
        ConnectError(format!("{}", error))
    }
}

impl From<DataError> for ConnectError {
    fn from(error: DataError) -> Self {
        ConnectError(format!("{}", error))
    }
}

impl From<ResourceError> for ConnectError {
    fn from(error: ResourceError) -> Self {
        ConnectError(format!("{}", error))
    }
}
