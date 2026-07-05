#[derive(Clone, Debug, PartialEq)]
pub enum TokenType {
    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    Comma,
    Dot,
    Minus,
    Plus,
    Colon,
    Semicolon,
    Slash,
    Star,
    Modulo,
    Ampersand,

    Bang,
    BangEqual,
    Equal,
    EqualEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,

    Identifier,
    Str,
    Integer,
    Float,

    And,
    Or,
    If,
    Else,
    For,
    Null,
    Return,
    While,
    Pub,
    Let,
    Fun,
    False,
    True,

    EOF,
}

impl TokenType {
    pub fn get_symbol_type(s: &str) -> Option<TokenType> {
        match s {
            "(" => Some(TokenType::LeftParen),
            ")" => Some(TokenType::RightParen),
            "[" => Some(TokenType::LeftBracket),
            "]" => Some(TokenType::RightBracket),
            "{" => Some(TokenType::LeftBrace),
            "}" => Some(TokenType::RightBrace),
            ";" => Some(TokenType::Semicolon),
            "," => Some(TokenType::Comma),
            "." => Some(TokenType::Dot),
            "-" => Some(TokenType::Minus),
            "+" => Some(TokenType::Plus),
            "/" => Some(TokenType::Slash),
            "*" => Some(TokenType::Star),
            ":" => Some(TokenType::Colon),
            "%" => Some(TokenType::Modulo),
            "&" => Some(TokenType::Ampersand),
            "&&" => Some(TokenType::And),
            "||" => Some(TokenType::Or),

            "!" => Some(TokenType::Bang),
            "!=" => Some(TokenType::BangEqual),
            "=" => Some(TokenType::Equal),
            "==" => Some(TokenType::EqualEqual),
            "<=" => Some(TokenType::LessEqual),
            "<" => Some(TokenType::Less),
            ">=" => Some(TokenType::GreaterEqual),
            ">" => Some(TokenType::Greater),
            _ => None,
        }
    }

    pub fn get_identifier_type(s: &str) -> TokenType {
        match s {
            "and" => TokenType::And,
            "else" => TokenType::Else,
            "false" => TokenType::False,
            "for" => TokenType::For,
            "fun" => TokenType::Fun,
            "if" => TokenType::If,
            "let" => TokenType::Let,
            "null" => TokenType::Null,
            "or" => TokenType::Or,
            "return" => TokenType::Return,
            "true" => TokenType::True,
            "while" => TokenType::While,
            _ => TokenType::Identifier,
        }
    }

    pub fn can_precede_equals(c: char) -> bool {
        match c {
            '=' | '<' | '>' | '!' => true,
            _ => false,
        }
    }
}

#[derive(Clone, Debug)]
pub struct Token {
    pub line: usize,
    pub lexeme: String,
    pub token_type: TokenType,
}

impl Token {
    pub fn new(line: usize, lexeme: String, token_type: TokenType) -> Token {
        Token {
            line,
            lexeme,
            token_type,
        }
    }
}
