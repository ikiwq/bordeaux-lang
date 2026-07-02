use std::fmt;

pub struct ParserError {
    message: String,
    line: usize,
}

impl fmt::Display for ParserError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{} at line {}", self.message, self.line)
    }
}

impl ParserError {
    pub fn new(message: String, line: usize) -> Self {
        ParserError { message, line }
    }
}
