use crate::lang::scanner::{
    err::ScannerError,
    token::{Token, TokenType},
};

pub mod err;
pub mod token;

pub struct Scanner {
    content: Vec<char>,
    current_index: usize,
    current_line: usize,
}

impl Scanner {
    pub fn scan(content: &str) -> Result<Vec<Token>, Vec<ScannerError>> {
        let mut scanner = Scanner {
            content: content.chars().collect(),
            current_index: 0,
            current_line: 1,
        };

        let mut tokens = Vec::new();
        let mut errors = Vec::new();

        while !scanner.is_at_end() {
            match scanner.scan_next() {
                Ok(token) => {
                    if token.token_type == TokenType::EOF {
                        break;
                    }

                    tokens.push(token)
                }
                Err(err) => errors.push(err),
            }
        }

        if !errors.is_empty() {
            return Err(errors);
        }

        Ok(tokens)
    }

    fn scan_next(&mut self) -> Result<Token, ScannerError> {
        self.skip_whitespace();

        if self.is_at_end() {
            return Result::Ok(Token::new(
                self.current_index,
                "EOF".to_string(),
                TokenType::EOF,
            ));
        }

        let c = self.peek().unwrap();
        if c == '"' {
            return self.string();
        }
        if c.is_numeric() {
            return self.number();
        }
        if c.is_alphabetic() {
            return self.identifier();
        }

        let mut lexeme = self.advance().unwrap().to_string();

        if TokenType::can_precede_equals(c) && self.match_char('=') {
            lexeme.push('=');
        }
        if c == '|' && self.match_char('|') {
            lexeme.push('|');
        }
        if c == '&' && self.match_char('&') {
            lexeme.push('&');
        }

        if let Some(token_type) = TokenType::get_symbol_type(&lexeme) {
            return Result::Ok(Token::new(self.current_line, lexeme, token_type));
        }

        Result::Err(ScannerError::new(
            format!("Unexpected character {}", c),
            self.current_line,
        ))
    }

    fn skip_whitespace(&mut self) {
        loop {
            if self.is_at_end() {
                break;
            }

            let c = self.peek().unwrap();

            if c.is_whitespace() {
                if c == '\n' {
                    self.current_line += 1;
                }
                self.advance();
                continue;
            }
            if c == '/' {
                if let Some(next) = self.peek_next()
                    && next == '/'
                {
                    while !self.is_at_end() && self.peek().unwrap() != '\n' {
                        self.advance();
                    }
                    self.advance(); // Consume the trailing '\n'
                }
                break;
            }
            break;
        }
    }

    fn string(&mut self) -> Result<Token, ScannerError> {
        self.expect_char('"')?;

        let mut lexeme = String::new();
        while !self.is_at_end() && self.peek().unwrap() != '"' {
            let c = self.advance().unwrap();

            if c == '\n' {
                self.current_line += 1;
            }
            lexeme.push(c);
        }

        if self.is_at_end() {
            return Err(ScannerError::new(
                "Unterminated string".to_string(),
                self.current_line,
            ));
        }
        self.advance(); // Consume the trailing '"'

        Ok(Token::new(self.current_line, lexeme, TokenType::Str))
    }

    fn number(&mut self) -> Result<Token, ScannerError> {
        let mut lexeme = String::new();
        let mut has_dot = false;

        while !self.is_at_end() {
            let c = self.peek().unwrap();
            if c == '.' {
                if has_dot {
                    return Err(ScannerError::new(
                        "Invalid number, cannot contain more than 1 dot".to_string(),
                        self.current_line,
                    ));
                }

                has_dot = true;
                lexeme.push(self.advance().unwrap());
                continue;
            }

            if c.is_numeric() {
                lexeme.push(self.advance().unwrap());
                continue;
            }

            break;
        }

        let token_type = if has_dot {
            TokenType::Float
        } else {
            TokenType::Integer
        };

        Ok(Token::new(self.current_line, lexeme, token_type))
    }

    fn identifier(&mut self) -> Result<Token, ScannerError> {
        let mut lexeme = String::new();

        while !self.is_at_end() {
            let c = self.peek().unwrap();
            if !c.is_alphanumeric() && c != '_' {
                break;
            }

            lexeme.push(self.advance().unwrap());
        }

        let token_type = TokenType::get_identifier_type(&lexeme);
        Ok(Token::new(self.current_line, lexeme, token_type))
    }

    fn advance(&mut self) -> Option<char> {
        let val = self.content.get(self.current_index).cloned();
        self.current_index += 1;
        return val;
    }

    fn peek(&self) -> Option<char> {
        self.content.get(self.current_index).cloned()
    }

    fn peek_next(&self) -> Option<char> {
        self.content.get(self.current_index + 1).cloned()
    }

    fn expect_char(&mut self, expected: char) -> Result<char, ScannerError> {
        match self.advance() {
            Some(next) if next == expected => Ok(next),
            Some(next) => Err(ScannerError::new(
                format!("Expected '{}', received '{}' instead", expected, next),
                self.current_line,
            )),
            None => Err(ScannerError::new(
                format!("Expected '{}', but reached EOF", expected),
                self.current_line,
            )),
        }
    }

    fn match_char(&mut self, expected: char) -> bool {
        if self.is_at_end() {
            return false;
        }

        if let Some(next) = self.peek()
            && next == expected
        {
            self.current_index += 1;
            return true;
        }

        false
    }

    fn is_at_end(&self) -> bool {
        self.current_index >= self.content.len()
    }
}
