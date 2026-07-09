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
    // Primitives
    Int64,
    // Int32,
    // Int16,
    // Int8,
    // Uint64,
    // Uint32,
    // Uint16,
    // Uint8,
    // Float64,
    Float32,
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
}

impl std::fmt::Display for VarType {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            // Primitives
            VarType::Int64 => write!(f, "i64"),
            // VarType::Int32 => write!(f, "i32"),
            // VarType::Int16 => write!(f, "i16"),
            // VarType::Int8 => write!(f, "i8"),
            // VarType::Uint64 => write!(f, "u64"),
            // VarType::Uint32 => write!(f, "u32"),
            // VarType::Uint16 => write!(f, "u16"),
            // VarType::Uint8 => write!(f, "u8"),
            // VarType::Float64 => write!(f, "f64"),
            VarType::Float32 => write!(f, "f32"),
            VarType::Str => write!(f, "string"),
            VarType::Bool => write!(f, "bool"),
            VarType::Char => write!(f, "char"),
            VarType::Void => write!(f, "void"),
            VarType::Function { .. } => write!(f, "function"),

            VarType::Reference(inner) => write!(f, "&{}", inner),
            VarType::Array(inner) => write!(f, "[{}]", inner),
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
            // "int32" => VarType::Int32,
            // "int16" => VarType::Int16,
            // "int8" => VarType::Int8,
            // "uint64" => VarType::Uint64,
            // "uint32" => VarType::Uint32,
            // "uint16" => VarType::Uint16,
            // "uint8" => VarType::Uint8,
            // "float64" => VarType::Float64,
            "float32" => VarType::Float32,
            "string" => VarType::Str,
            "bool" => VarType::Bool,
            "char" => VarType::Char,
            "void" => VarType::Void,
            _ => panic!("Uknown type {}", token.lexeme),
        }
    }
}
