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

#[derive(Debug)]
pub enum Literal {
    Bool(bool),
    Integer(i64),
    Float(f64),
    Str(String),
}

#[derive(Debug, Clone)]
pub enum VarType {
    // Primitives
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
    SelfType,
    Unknown,

    // Wrappers
    Reference(Box<VarType>),
    Array(Box<VarType>),
    Named(Box<VarType>),

    // Struct
    Struct { name: Token },

    // Generics
    GenericParam(Token),
    GenericInstantiation { base: Token, args: Vec<VarType> },
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
            _ => VarType::Struct { name: token },
        }
    }
}
