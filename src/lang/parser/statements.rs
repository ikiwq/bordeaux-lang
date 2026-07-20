use crate::lang::{
    parser::{
        Parser,
        err::ParserError,
        expressions::Expr,
        types::{FunParameter, FunSignature, VarType},
    },
    scanner::token::{Token, TokenType},
};

#[derive(Clone, Debug)]
pub enum Statement {
    Program(Vec<Statement>),
    Block {
        left_brace: Token,
        statements: Vec<Statement>,
        right_brace: Token,
    },
    If {
        keyword: Token,
        condition: Expr,
        then_branch: Box<Statement>,
        else_branch: Option<Box<Statement>>,
    },
    While {
        keyword: Token,
        condition: Expr,
        body: Box<Statement>,
    },
    For {
        keyword: Token,
        init: Box<Statement>,
        condition: Expr,
        increment: Expr,
        body: Box<Statement>,
    },
    VarDeclaration {
        keyword: Token,
        identifier: Token,
        var_type: VarType,
        initializer: Expr,
    },
    FunDeclaration {
        signature: FunSignature,
        body: Box<Statement>,
    },
    Return {
        keyword: Token,
        value: Expr,
    },
    Expression(Expr),
}

impl Parser {
    pub fn statement(&mut self) -> Result<Statement, ParserError> {
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
            TokenType::Let => self.statement_var_declaration(),
            TokenType::Fun => self.statement_fun_declaration(),
            _ => self.statement_expr(),
        };
    }

    fn statement_for(&mut self) -> Result<Statement, ParserError> {
        let keyword = self.expect_token(TokenType::For)?;

        let init = Box::new(self.statement()?); // Statement already consumes ;

        let condition = self.expression()?;
        self.expect_token(TokenType::Semicolon)?;
        let increment = self.expression()?;

        let body = Box::new(self.statement_block()?);

        Ok(Statement::For {
            keyword,
            init,
            condition,
            increment,
            body,
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
        while !self.check(TokenType::RightBrace) {
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
        let then_branch = Box::new(self.statement_block()?);

        let mut else_branch = None;
        if self.matches(TokenType::Else) {
            else_branch = Some(Box::new(self.statement_block()?));
        }

        Ok(Statement::If {
            keyword,
            condition,
            then_branch,
            else_branch,
        })
    }

    fn statement_return(&mut self) -> Result<Statement, ParserError> {
        let keyword = self.expect_token(TokenType::Return)?;
        let value = self.expression()?;

        self.expect_token(TokenType::Semicolon)?;

        return Ok(Statement::Return { keyword, value });
    }

    fn statement_var_declaration(&mut self) -> Result<Statement, ParserError> {
        let keyword = self.expect_token(TokenType::Let)?;
        let identifier = self.expect_token(TokenType::Identifier)?;

        let mut var_type = VarType::Unknown;
        if self.matches(TokenType::Colon) {
            var_type = self.var_type()?;
        }

        self.expect_token(TokenType::Equal)?;

        let initializer = self.expression()?;
        self.expect_token(TokenType::Semicolon)?;

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

        let parameters = self.parse_parameters()?;

        let mut return_type = VarType::Void;
        if self.check(TokenType::Colon) {
            self.advance();
            return_type = self.var_type()?;
        }

        let body = self.statement_block()?;

        Ok(Statement::FunDeclaration {
            signature: FunSignature {
                keyword,
                name: identifier,
                parameters,
                return_type,
            },
            body: Box::new(body),
        })
    }

    fn statement_expr(&mut self) -> Result<Statement, ParserError> {
        let expr = self.expression()?;
        self.expect_token(TokenType::Semicolon)?;
        Ok(Statement::Expression(expr))
    }

    fn parse_parameters(&mut self) -> Result<Vec<FunParameter>, ParserError> {
        self.expect_token(TokenType::LeftParen)?;

        let mut parameters = Vec::new();
        if !self.check(TokenType::RightParen) {
            loop {
                let name = self.expect_token(TokenType::Identifier)?;
                self.expect_token(TokenType::Colon)?;
                let var_type = self.var_type()?;

                parameters.push(FunParameter { name, var_type });

                if !self.matches(TokenType::Comma) {
                    break;
                }
            }
        }

        self.expect_token(TokenType::RightParen)?;

        Ok(parameters)
    }
}
