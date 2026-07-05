use crate::lang::{
    parser::{Parser, err::ParserError, types::Literal},
    scanner::token::{Token, TokenType},
};

#[derive(Debug)]
pub enum Expr {
    Literal(Literal),
    Variable(Token),
    Binary {
        left: Box<Expr>,
        operator: Token,
        right: Box<Expr>,
    },
    Unary {
        operator: Token,
        right: Box<Expr>,
    },
    Grouping(Box<Expr>),
    Call {
        callee: Box<Expr>,
        arguments: Vec<Expr>,
    },
    Assign {
        name: Token,
        value: Box<Expr>,
    },
}

impl Expr {
    pub fn str(value: String) -> Self {
        Expr::Literal(Literal::Str(value))
    }

    pub fn integer(value: String) -> Self {
        Expr::Literal(Literal::Integer(value.parse().unwrap_or_else(|e| {
            panic!("Parser panicked! Tried to parse the string '{value}' as an integer. Error: {e}")
        })))
    }

    pub fn float(value: String) -> Self {
        Expr::Literal(Literal::Float(value.parse().unwrap_or_else(|e| {
            panic!("Parser panicked! Tried to parse the string '{value}' as a float. Error: {e}")
        })))
    }

    pub fn bool(value: bool) -> Self {
        Expr::Literal(Literal::Bool(value))
    }
}

impl Parser {
    pub fn expression(&mut self) -> Result<Expr, ParserError> {
        self.expr_assignment()
    }

    fn expr_assignment(&mut self) -> Result<Expr, ParserError> {
        let expr = self.expr_equality()?;

        if self.check(TokenType::Equal) {
            let equals = self.advance().unwrap();
            let value = self.expr_assignment()?;

            if let Expr::Variable(name) = expr {
                return Ok(Expr::Assign {
                    name,
                    value: Box::new(value),
                });
            }

            return Err(ParserError::new(
                "Invalid assignment target".to_string(),
                equals.line,
            ));
        }

        Ok(expr)
    }

    fn expr_equality(&mut self) -> Result<Expr, ParserError> {
        let mut left = self.expr_comparison()?;

        loop {
            if !self.check_any(&[TokenType::BangEqual, TokenType::EqualEqual]) {
                break;
            }

            let operator = self.advance().unwrap();
            let right = self.expr_comparison()?;

            left = Expr::Binary {
                left: Box::new(left),
                operator,
                right: Box::new(right),
            }
        }

        Ok(left)
    }

    fn expr_comparison(&mut self) -> Result<Expr, ParserError> {
        let mut left = self.expr_term()?;

        loop {
            if !self.check_any(&[
                TokenType::Less,
                TokenType::LessEqual,
                TokenType::Greater,
                TokenType::GreaterEqual,
            ]) {
                break;
            }

            let operator = self.advance().unwrap();
            let right = self.expr_term()?;

            left = Expr::Binary {
                left: Box::new(left),
                operator,
                right: Box::new(right),
            }
        }

        Ok(left)
    }

    fn expr_term(&mut self) -> Result<Expr, ParserError> {
        let mut left = self.expr_factor()?;

        loop {
            if !self.check_any(&[TokenType::Minus, TokenType::Plus]) {
                break;
            }

            let operator = self.advance().unwrap();
            let right = self.expr_factor()?;

            left = Expr::Binary {
                left: Box::new(left),
                operator,
                right: Box::new(right),
            }
        }

        Ok(left)
    }

    fn expr_factor(&mut self) -> Result<Expr, ParserError> {
        let mut left = self.expr_unary()?;

        loop {
            if !self.check_any(&[TokenType::Slash, TokenType::Star]) {
                break;
            }

            let operator = self.advance().unwrap();
            let right = self.expr_unary()?;

            left = Expr::Binary {
                left: Box::new(left),
                operator,
                right: Box::new(right),
            }
        }

        Ok(left)
    }

    fn expr_unary(&mut self) -> Result<Expr, ParserError> {
        if self.check_any(&[TokenType::Bang, TokenType::Minus]) {
            let operator = self.advance().unwrap();
            return Ok(Expr::Unary {
                operator,
                right: Box::new(self.expr_unary()?),
            });
        }

        self.expr_call()
    }

    fn expr_call(&mut self) -> Result<Expr, ParserError> {
        let mut expr = self.expr_primary()?;

        loop {
            if self.check(TokenType::LeftParen) {
                self.advance();
                expr = self.finish_call(expr)?;
            } else {
                break;
            }
        }

        Ok(expr)
    }

    fn finish_call(&mut self, callee: Expr) -> Result<Expr, ParserError> {
        let mut arguments = Vec::new();

        if !self.check(TokenType::RightParen) {
            loop {
                arguments.push(self.expression()?);
                if !self.check(TokenType::Comma) {
                    break;
                }
                self.advance();
            }
        }

        self.expect_token(TokenType::RightParen)?;

        Ok(Expr::Call {
            callee: Box::new(callee),
            arguments,
        })
    }

    fn expr_primary(&mut self) -> Result<Expr, ParserError> {
        if self.is_at_end() {
            return Err(ParserError::new(
                "Expected literal, reached EOF".to_string(),
                self.last().unwrap().line,
            ));
        }

        let next = self.advance().unwrap();

        return match next.token_type {
            TokenType::Str => Ok(Expr::str(next.lexeme)),
            TokenType::Float => Ok(Expr::float(next.lexeme)),
            TokenType::Integer => Ok(Expr::integer(next.lexeme)),
            TokenType::False => Ok(Expr::bool(false)),
            TokenType::True => Ok(Expr::bool(true)),
            TokenType::Identifier => Ok(Expr::Variable(next)),
            TokenType::LeftParen => {
                let expr = self.expression()?;
                self.expect_token(TokenType::RightParen)?;
                Ok(Expr::Grouping(Box::new(expr)))
            }
            _ => Err(ParserError::new(
                format!("Expected literal, found {}", next.lexeme),
                next.line,
            )),
        };
    }
}
