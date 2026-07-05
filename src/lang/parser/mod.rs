use crate::lang::{
    parser::{err::ParserError, statements::Statement, types::VarType},
    scanner::token::{Token, TokenType},
};

pub mod err;
mod expressions;
pub mod statements;
pub mod types;

pub struct Parser {
    tokens: Vec<Token>,
    current_index: usize,
}

impl Parser {
    pub fn parse(tokens: Vec<Token>) -> Result<Statement, Vec<ParserError>> {
        let mut parser = Parser {
            tokens,
            current_index: 0,
        };

        let mut statements = Vec::new();
        let mut errors = Vec::new();

        while !parser.is_at_end() {
            match parser.statement() {
                Ok(statement) => statements.push(statement),
                Err(err) => {
                    errors.push(err);
                    parser.sync();
                }
            }
        }

        if !errors.is_empty() {
            return Err(errors);
        }

        Ok(Statement::Program(statements))
    }

    fn sync(&mut self) {
        while !self.is_at_end() {
            if self.matches(TokenType::Semicolon) {
                return;
            }

            if self.check_any(&[TokenType::Fun, TokenType::Let]) {
                return;
            }

            self.advance();
        }
    }

    fn var_type(&mut self) -> Result<VarType, ParserError> {
        let next = self.expect_tokens(&[
            TokenType::Ampersand,
            TokenType::LeftBracket,
            TokenType::Identifier,
        ])?;

        return match next.token_type {
            TokenType::Ampersand => {
                let var_type = VarType::Reference(Box::new(self.var_type()?));
                Ok(var_type)
            }
            TokenType::LeftBracket => {
                let var_type = VarType::Array(Box::new(self.var_type()?));
                self.expect_token(TokenType::RightBracket)?;
                Ok(var_type)
            }
            TokenType::Identifier => Ok(VarType::from_token(next)),
            _ => unreachable!(),
        };
    }

    fn expect_token(&mut self, expected: TokenType) -> Result<Token, ParserError> {
        match self.advance() {
            Some(next) if next.token_type == expected => Ok(next),
            Some(next) => Err(ParserError::new(
                format!("Expected {:?}, found {}", expected, next.lexeme),
                next.line,
            )),
            None => Err(ParserError::new(
                format!("Expected {:?}, but reached EOF", expected),
                self.last().unwrap().line,
            )),
        }
    }

    fn expect_tokens(&mut self, expected: &[TokenType]) -> Result<Token, ParserError> {
        match self.advance() {
            Some(next) if expected.contains(&next.token_type) => Ok(next),

            Some(next) => Err(ParserError::new(
                format!("Expected one of {:?}, found {}", expected, next.lexeme),
                next.line,
            )),

            None => Err(ParserError::new(
                format!("Expected one of {:?}, but reached EOF", expected),
                self.last().map(|t| t.line).unwrap_or(0),
            )),
        }
    }

    fn check(&mut self, expected: TokenType) -> bool {
        match self.peek() {
            Some(next) => {
                if next.token_type == expected {
                    return true;
                }
                false
            }
            None => false,
        }
    }

    fn check_any(&mut self, expected: &[TokenType]) -> bool {
        match self.peek() {
            Some(next) => {
                if expected.contains(&next.token_type) {
                    return true;
                }
                return false;
            }
            None => false,
        }
    }

    fn matches(&mut self, expected: TokenType) -> bool {
        match self.peek() {
            Some(next) => {
                if next.token_type == expected {
                    self.current_index += 1;
                    return true;
                }
                return false;
            }
            None => false,
        }
    }

    fn peek(&self) -> Option<Token> {
        self.tokens.get(self.current_index).cloned()
    }

    fn advance(&mut self) -> Option<Token> {
        let val = self.tokens.get(self.current_index).cloned();
        self.current_index += 1;
        val
    }

    fn last(&self) -> Option<Token> {
        self.tokens.last().cloned()
    }

    fn is_at_end(&self) -> bool {
        self.current_index >= self.tokens.len()
    }
}
