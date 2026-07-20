use crate::lang::scanner::token::Token;

#[derive(Debug)]
pub struct StructField {
    pub name: Token,
    pub var_type: VarType,
}

#[derive(Debug, Clone)]
pub struct FunSignature {
    pub keyword: Token,
    pub name: Token,
    pub parameters: Vec<FunParameter>,
    pub return_type: VarType,
}

#[derive(Debug, Clone)]
pub struct FunParameter {
    pub name: Token,
    pub var_type: VarType,
}

#[derive(Clone, Debug)]
pub enum Literal {
    Bool(bool),
    Integer(i64),
    Float(f64),
    Str(String),
}

#[derive(PartialEq, Debug, Clone)]
pub enum VarType {
    Int64,
    Float64,
    Str,
    Bool,
    Char,
    Void,

    Function {
        params: Vec<VarType>,
        return_type: Box<VarType>,
    },

    // Wrappers
    Reference(Box<VarType>),
    Array(Box<VarType>),

    // Unknown
    Unknown,
}

impl std::fmt::Display for VarType {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            // Primitives
            VarType::Int64 => write!(f, "i64"),
            VarType::Float64 => write!(f, "f64"),
            VarType::Str => write!(f, "string"),
            VarType::Bool => write!(f, "bool"),
            VarType::Char => write!(f, "char"),
            VarType::Void => write!(f, "void"),
            VarType::Function { .. } => write!(f, "function"),

            VarType::Reference(inner) => write!(f, "&{}", inner),
            VarType::Array(inner) => write!(f, "[{}]", inner),
            VarType::Unknown => write!(f, "unknown"),
        }
    }
}

impl VarType {
    pub fn is_void(&self) -> bool {
        matches!(self, VarType::Void)
    }

    pub fn from_token(token: Token) -> VarType {
        match token.lexeme.as_str() {
            "int64" => VarType::Int64,
            "float64" => VarType::Float64,
            "string" => VarType::Str,
            "bool" => VarType::Bool,
            "char" => VarType::Char,
            "void" => VarType::Void,
            _ => panic!("Uknown type {}", token.lexeme),
        }
    }
}
