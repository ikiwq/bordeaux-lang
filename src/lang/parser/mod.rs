use crate::lang::{
    parser::{
        err::ParserError,
        types::{Expr, FunParameter, Program, Statement, VarType},
    },
    scanner::token::{Token, TokenType},
};

pub mod err;
pub mod types;

pub struct Parser {
    tokens: Vec<Token>,
    current_index: usize,
}

impl Parser {
    pub fn parse(tokens: Vec<Token>) -> Result<Program, Vec<ParserError>> {
        let mut parser = Parser {
            tokens,
            current_index: 0,
        };

        let mut statements = Vec::new();
        let mut errors = Vec::new();

        while !parser.is_at_end() {
            match parser.statement() {
                Ok(statement) => statements.push(statement),
                Err(err) => errors.push(err),
            }
        }

        if !errors.is_empty() {
            return Err(errors);
        }

        Ok(Program::new(statements))
    }

    fn statement(&mut self) -> Result<Statement, ParserError> {
        let token = match self.peek() {
            Some(t) => t,
            None => {
                return Err(ParserError::new(
                    "Expected statement, found end of file".to_string(),
                    self.last().unwrap().line,
                ));
            }
        };

        return match token.token_type {
            TokenType::For => self.statement_for(),
            TokenType::LeftBrace => self.statement_block(),
            TokenType::If => self.statement_if(),
            TokenType::While => self.statement_while(),
            TokenType::Return => self.statement_return(),
            TokenType::Type => self.statement_type_declaration(),
            TokenType::Let => self.statement_var_declaration(),
            TokenType::Fun => self.statement_fun_declaration(),
            _ => self.statement_expr(),
        };
    }

    fn statement_for(&mut self) -> Result<Statement, ParserError> {
        let keyword = self.expect_token(TokenType::For)?;
        let condition = self.expression()?;
        let then_branch = Box::new(self.statement_block()?);

        let mut else_branch = None;
        if !self.is_at_end() && self.peek().unwrap().token_type == TokenType::Else {
            else_branch = Some(Box::new(self.statement_block()?));
        }

        Ok(Statement::If {
            keyword,
            condition,
            then_branch,
            else_branch,
        })
    }

    fn statement_while(&mut self) -> Result<Statement, ParserError> {
        let keyword = self.expect_token(TokenType::While)?;
        let condition = self.expression()?;
        let body = Box::new(self.statement_block()?);

        Ok(Statement::While {
            keyword,
            condition,
            body,
        })
    }

    fn statement_block(&mut self) -> Result<Statement, ParserError> {
        let left_brace = self.expect_token(TokenType::LeftBrace)?;

        let mut statements = Vec::new();
        while !self.matches(TokenType::RightBrace) {
            statements.push(self.statement()?);
        }

        let right_brace = self.expect_token(TokenType::RightBrace)?;

        Ok(Statement::Block {
            left_brace,
            statements,
            right_brace,
        })
    }

    fn statement_if(&mut self) -> Result<Statement, ParserError> {
        let keyword = self.expect_token(TokenType::If)?;
        let condition = self.expression()?;
        let then_branch = self.statement_block()?;

        Ok(Statement::If {
            keyword,
            condition,
            then_branch: Box::new(then_branch),
            else_branch: Option::None,
        })
    }

    fn statement_return(&mut self) -> Result<Statement, ParserError> {
        let keyword = self.expect_token(TokenType::Return)?;
        let value = self.expression()?;

        self.expect_token(TokenType::Semicolon)?;

        return Ok(Statement::Return { keyword, value });
    }

    fn statement_type_declaration(&mut self) -> Result<Statement, ParserError> {
        self.expect_token(TokenType::Type)?;
        let identifier = self.expect_token(TokenType::Identifier)?;
        self.expect_token(TokenType::Equal)?;

        let var_type = self.var_type()?;

        Ok(Statement::TypeDeclaration {
            identifier,
            var_type: VarType::Named(Box::new(var_type)),
        })
    }

    fn statement_var_declaration(&mut self) -> Result<Statement, ParserError> {
        let keyword = self.expect_token(TokenType::Let)?;
        let identifier = self.expect_token(TokenType::Identifier)?;

        let mut next =
            self.expect_tokens(&[TokenType::Colon, TokenType::Semicolon, TokenType::Equal])?;

        let mut var_type = VarType::Unknown;
        let mut initializer = None;

        if next.token_type == TokenType::Colon {
            var_type = self.var_type()?;
            next = self.expect_tokens(&[TokenType::Equal, TokenType::Semicolon])?;
        }

        if next.token_type == TokenType::Equal {
            initializer = Some(self.expression()?);
            self.expect_token(TokenType::Semicolon)?;
        }

        Ok(Statement::VarDeclaration {
            keyword,
            identifier,
            var_type,
            initializer,
        })
    }

    fn statement_fun_declaration(&mut self) -> Result<Statement, ParserError> {
        let keyword = self.expect_token(TokenType::Fun)?;
        let identifier = self.expect_token(TokenType::Identifier)?;

        self.expect_token(TokenType::LeftParen)?;
        let mut params = Vec::new();

        while !self.is_at_end() && self.peek().unwrap().token_type != TokenType::RightParen {
            let name = self.expect_token(TokenType::Identifier)?;

            self.expect_token(TokenType::Colon)?;
            let var_type = self.var_type()?;

            params.push(FunParameter { name, var_type });

            if self.matches(TokenType::Comma) {
                self.advance(); // Consume the comma before resuming
            }
        }
        self.expect_token(TokenType::RightParen)?;

        let mut return_type = VarType::Void;
        if self.matches(TokenType::Colon) {
            self.advance();
            return_type = self.var_type()?;
        }

        let body = self.statement_block()?;

        Ok(Statement::FunDeclaration {
            keyword,
            identifier,
            params,
            return_type,
            body: Box::new(body),
        })
    }

    fn statement_expr(&mut self) -> Result<Statement, ParserError> {
        let expr = self.expression()?;
        self.expect_token(TokenType::Semicolon)?;
        Ok(Statement::Expression(expr))
    }

    fn expression(&mut self) -> Result<Expr, ParserError> {
        self.expr_equality()
    }

    fn expr_equality(&mut self) -> Result<Expr, ParserError> {
        let mut left = self.expr_comparison()?;

        loop {
            if !self.match_multple(&[TokenType::BangEqual, TokenType::EqualEqual]) {
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
            if !self.match_multple(&[
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
            if !self.match_multple(&[TokenType::Minus, TokenType::Plus]) {
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
            if !self.match_multple(&[TokenType::Slash, TokenType::Star]) {
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
        if self.match_multple(&[TokenType::Bang, TokenType::Minus]) {
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
            if self.matches(TokenType::LeftParen) {
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

        if !self.matches(TokenType::RightParen) {
            loop {
                arguments.push(self.expression()?);
                if !self.matches(TokenType::Comma) {
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

    fn var_type(&mut self) -> Result<VarType, ParserError> {
        let next = self.expect_tokens(&[
            TokenType::Ampersand,
            TokenType::LeftBracket,
            TokenType::Identifier,
        ])?;

        if next.token_type == TokenType::Ampersand {
            let var_type = VarType::Pointer(Box::new(self.var_type()?));
            return Ok(var_type);
        }

        if next.token_type == TokenType::LeftBracket {
            let var_type = VarType::Array(Box::new(self.var_type()?));
            self.expect_token(TokenType::RightBracket)?;
            return Ok(var_type);
        }

        Ok(VarType::from_token(next))
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

    fn matches(&mut self, expected: TokenType) -> bool {
        match self.peek() {
            Some(next) => {
                if next.token_type == expected {
                    return true;
                }
                return false;
            }
            None => false,
        }
    }

    fn match_multple(&mut self, expected: &[TokenType]) -> bool {
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
