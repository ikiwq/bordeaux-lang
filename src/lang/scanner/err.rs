use std::fmt;

pub struct ScannerError {
    message: String,
    line: usize,
}

impl fmt::Display for ScannerError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{} at line {}", self.message, self.line)
    }
}

impl ScannerError {
    pub fn new(message: String, line: usize) -> Self {
        ScannerError { line, message }
    }
}
