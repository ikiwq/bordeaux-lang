use crate::lang::scanner::token::Token;

#[derive(Debug)]
pub struct Program {
    pub statements: Vec<Statement>,
}

impl Program {
    pub fn new(statements: Vec<Statement>) -> Program {
        Program { statements }
    }
}

#[derive(Debug)]
pub enum Statement {
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
    TypeDeclaration {
        identifier: Token,
        var_type: VarType,
    },
    VarDeclaration {
        keyword: Token,
        identifier: Token,
        var_type: VarType,
        initializer: Option<Expr>,
    },
    FunDeclaration {
        keyword: Token,
        identifier: Token,
        params: Vec<FunParameter>,
        return_type: VarType,
        body: Box<Statement>,
    },
    Return {
        keyword: Token,
        value: Expr,
    },
    Expression(Expr),
}

#[derive(Debug)]
pub struct FunParameter {
    pub name: Token,
    pub var_type: VarType,
}

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
}

impl Expr {
    pub fn str(value: String) -> Self {
        Expr::Literal(Literal::Str(value))
    }

    pub fn float(value: String) -> Self {
        Expr::Literal(Literal::Float(value.parse().unwrap()))
    }

    pub fn integer(value: String) -> Self {
        Expr::Literal(Literal::Integer(value.parse().unwrap()))
    }

    pub fn bool(value: bool) -> Self {
        Expr::Literal(Literal::Bool(value))
    }
}

#[derive(Debug)]
pub enum Literal {
    Bool(bool),
    Integer(i64),
    Float(f64),
    Str(String),
}

#[derive(Debug)]
pub enum VarType {
    Int64,
    Int32,
    Int16,
    Int8,
    Uint64,
    Uint32,
    Uint16,
    Uint8,
    Float64,
    Float32,
    Str,
    Bool,
    Char,
    Void,
    Unknown,
    Pointer(Box<VarType>),
    Array(Box<VarType>),
    Named(Box<VarType>),
    Struct { name: String },
}

impl VarType {
    pub fn is_void(&self) -> bool {
        matches!(self, VarType::Void)
    }

    pub fn from_token(token: Token) -> VarType {
        match token.lexeme.as_str() {
            "int64" => VarType::Int64,
            "int32" => VarType::Int32,
            "int16" => VarType::Int16,
            "int8" => VarType::Int8,
            "uint64" => VarType::Uint64,
            "uint32" => VarType::Uint32,
            "uint16" => VarType::Uint16,
            "uint8" => VarType::Uint8,
            "float64" => VarType::Float64,
            "float32" => VarType::Float32,
            "string" => VarType::Str,
            "bool" => VarType::Bool,
            "char" => VarType::Char,
            "void" => VarType::Void,
            _ => VarType::Struct { name: token.lexeme },
        }
    }
}
