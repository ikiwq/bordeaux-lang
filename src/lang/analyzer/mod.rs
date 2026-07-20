use std::collections::HashMap;

use crate::lang::{
    analyzer::err::AnalysisError,
    parser::{
        expressions::{CastKind, Expr},
        statements::Statement,
        types::{Literal, VarType},
    },
    scanner::token::{Token, TokenType},
};

pub mod err;

#[derive(Clone)]
struct VariableDefinition {
    identifier: Token,
    var_type: VarType,
    defined: bool,
}

pub struct Analyzer {
    environments: Vec<HashMap<String, VariableDefinition>>,
}

impl Analyzer {
    fn new() -> Self {
        let mut environments: Vec<HashMap<String, VariableDefinition>> = Vec::new();

        let mut root_env: HashMap<String, VariableDefinition> = HashMap::new();

        let mut print_params = Vec::new();
        print_params.push(VarType::Str);
        root_env.insert(
            "print".to_string(),
            VariableDefinition {
                identifier: Token {
                    line: 0,
                    lexeme: "println".to_string(),
                    token_type: TokenType::Identifier,
                },
                var_type: VarType::Function {
                    params: print_params,
                    return_type: Box::new(VarType::Void),
                },
                defined: true,
            },
        );

        environments.push(root_env); // Root environment

        return Analyzer { environments };
    }

    pub fn analyze(program: &mut Statement) -> Result<(), Vec<AnalysisError>> {
        let mut analyzer = Analyzer::new();
        analyzer.analyze_inner(program)
    }

    pub fn analyze_inner(&mut self, program: &mut Statement) -> Result<(), Vec<AnalysisError>> {
        let mut errs = Vec::new();

        match program {
            Statement::Program(statements) => {
                for stmt in statements {
                    if let Err(e) = self.analyze_inner(stmt) {
                        errs.extend(e);
                    }
                }
            }

            Statement::Block { statements, .. } => {
                self.push_env();
                for stmt in statements {
                    if let Err(e) = self.analyze_inner(stmt) {
                        errs.extend(e);
                    }
                }
                self.pop_env();
            }

            Statement::If {
                condition,
                then_branch,
                else_branch,
                ..
            } => {
                match self.expression_type(condition) {
                    Ok(VarType::Bool) => {}
                    Ok(other) => errs.push(AnalysisError {
                        message: format!("Expected bool in 'if' condition, received {other}"),
                    }),
                    Err(e) => errs.push(e),
                }
                if let Err(e) = self.analyze_inner(then_branch) {
                    errs.extend(e);
                }
                if let Some(actual_else) = else_branch {
                    if let Err(e) = self.analyze_inner(actual_else) {
                        errs.extend(e);
                    }
                }
            }

            Statement::While {
                condition, body, ..
            } => {
                match self.expression_type(condition) {
                    Ok(VarType::Bool) => {}
                    Ok(other) => errs.push(AnalysisError {
                        message: format!("Expected bool in 'while' condition, received {other}"),
                    }),
                    Err(e) => errs.push(e),
                }
                if let Err(e) = self.analyze_inner(body) {
                    errs.extend(e);
                }
            }

            Statement::For {
                init,
                condition,
                increment,
                body,
                ..
            } => {
                self.push_env();

                if let Err(e) = self.analyze_inner(init) {
                    errs.extend(e);
                }

                match self.expression_type(condition) {
                    Ok(VarType::Bool) => {}
                    Ok(other) => errs.push(AnalysisError {
                        message: format!("Expected bool in 'for' condition, received {other}"),
                    }),
                    Err(e) => errs.push(e),
                }

                if let Err(e) = self.expression_type(increment) {
                    errs.push(e);
                }
                if let Err(e) = self.analyze_inner(body) {
                    errs.extend(e);
                }

                self.pop_env();
            }

            Statement::VarDeclaration {
                identifier,
                var_type,
                initializer,
                ..
            } => {
                match self.expression_type(initializer) {
                    Ok(init_type) => {
                        if *var_type == VarType::Unknown {
                            *var_type = init_type;
                        } else if *var_type != init_type {
                            errs.push(AnalysisError {
                                message: format!(
                                    "Type mismatch: cannot initialize {} with {}",
                                    var_type, init_type
                                ),
                            });
                        }
                    }
                    Err(e) => errs.push(e),
                }

                // Always prioritize the user's type definition
                self.declare_var(identifier.clone(), var_type.clone());

                if let Err(e) = self.define_var(identifier) {
                    errs.push(e);
                }
            }
            Statement::FunDeclaration { signature, body } => {
                let fun_type = VarType::Function {
                    params: signature
                        .parameters
                        .iter()
                        .map(|p| p.var_type.clone())
                        .collect(),
                    return_type: Box::new(signature.return_type.clone()),
                };

                self.declare_var(signature.name.clone(), fun_type);
                if let Err(e) = self.define_var(&signature.name.clone()) {
                    errs.push(e);
                }

                self.push_env();

                for param in signature.parameters.clone() {
                    self.declare_var(param.name.clone(), param.var_type.clone());
                    if let Err(e) = self.define_var(&param.name) {
                        errs.push(e);
                    }
                }

                if let Err(e) = self.analyze_inner(body) {
                    errs.extend(e);
                }

                self.pop_env();
            }

            Statement::Return { value, .. } => {
                if let Err(e) = self.expression_type(value) {
                    errs.push(e);
                }
            }

            Statement::Expression(expr) => {
                if let Err(e) = self.expression_type(expr) {
                    errs.push(e);
                }
            }
        }

        if errs.is_empty() { Ok(()) } else { Err(errs) }
    }

    fn expression_type(&mut self, expression: &mut Expr) -> Result<VarType, AnalysisError> {
        match expression {
            Expr::Literal(literal) => match literal {
                Literal::Bool(_) => Ok(VarType::Bool),
                Literal::Integer(_) => Ok(VarType::Int64),
                Literal::Float(_) => Ok(VarType::Float64),
                Literal::Str(_) => Ok(VarType::Str),
            },
            Expr::Variable(identifier) => match self.get_var(identifier.clone()) {
                Some(def) => Ok(def.var_type),
                None => Err(AnalysisError {
                    message: format!("Using undeclared variable {}", identifier.lexeme),
                }),
            },
            Expr::Binary {
                left,
                operator,
                right,
            } => {
                let mut left_type = self.expression_type(left)?;
                let right_type = self.expression_type(right)?;

                if left_type != right_type {
                    if left_type == VarType::Float64 && right_type == VarType::Int64 {
                        let old =
                            std::mem::replace(right, Box::new(Expr::Literal(Literal::Integer(0))));
                        *right = Box::new(Expr::Cast {
                            expr: old,
                            to: VarType::Float64,
                            kind: CastKind::IntToFloat,
                        });
                    } else if left_type == VarType::Int64 && right_type == VarType::Float64 {
                        let old =
                            std::mem::replace(left, Box::new(Expr::Literal(Literal::Integer(0))));
                        *left = Box::new(Expr::Cast {
                            expr: old,
                            to: VarType::Float64,
                            kind: CastKind::IntToFloat,
                        });
                        left_type = VarType::Float64;
                    } else {
                        return Err(AnalysisError {
                            message: format!(
                                "Cannot apply {} to {} and {}",
                                operator.lexeme, left_type, right_type
                            ),
                        });
                    }
                }

                match operator.lexeme.as_str() {
                    "==" | "!=" | "<" | "<=" | ">" | ">=" => Ok(VarType::Bool),
                    _ => Ok(left_type),
                }
            }
            Expr::Unary { right, .. } => self.expression_type(right),
            Expr::Assign { name, value } => {
                let current_var = self.get_var(name.clone()).ok_or_else(|| AnalysisError {
                    message: format!("Cannot assign undeclared variable {}", name.lexeme),
                })?;

                let val_type = self.expression_type(value)?;

                if current_var.var_type != val_type {
                    return Err(AnalysisError {
                        message: format!(
                            "Cannot assign type {} to variable {} of type {}",
                            val_type, name.lexeme, current_var.var_type
                        ),
                    });
                }

                self.define_var(name)?;
                Ok(val_type)
            }
            Expr::Call { callee, arguments } => {
                let callee_type = self.expression_type(callee)?;

                match callee_type {
                    VarType::Function {
                        params,
                        return_type,
                    } => {
                        if params.len() != arguments.len() {
                            return Err(AnalysisError {
                                message: format!(
                                    "Expected {} arguments but got {}",
                                    params.len(),
                                    arguments.len()
                                ),
                            });
                        }

                        for (param_type, arg_expr) in params.iter().zip(arguments.iter_mut()) {
                            let arg_type = self.expression_type(arg_expr)?;
                            if param_type != &arg_type {
                                return Err(AnalysisError {
                                    message: format!(
                                        "Expected type {}, found {}",
                                        param_type, arg_type
                                    ),
                                });
                            }
                        }

                        Ok(*return_type)
                    }
                    _ => Err(AnalysisError {
                        message: "Cannot call non-function type".to_string(),
                    }),
                }
            }
            Expr::Grouping(expr) => self.expression_type(expr),
            Expr::Cast { .. } => panic!("Parser cannot insert casting"),
        }
    }

    fn define_var(&mut self, identifier: &Token) -> Result<(), AnalysisError> {
        for env in self.environments.iter_mut().rev() {
            if let Some(existing) = env.get(&identifier.lexeme) {
                let old_var_type = existing.var_type.clone();
                env.insert(
                    identifier.lexeme.clone(),
                    VariableDefinition {
                        identifier: identifier.clone(),
                        var_type: old_var_type,
                        defined: true,
                    },
                );
                return Ok(());
            }
        }

        Err(AnalysisError {
            message: format!("Cannot assign undeclared variable {}", identifier.lexeme),
        })
    }

    fn declare_var(&mut self, identifier: Token, var_type: VarType) {
        self.environments.last_mut().unwrap().insert(
            identifier.clone().lexeme,
            VariableDefinition {
                identifier,
                var_type,
                defined: false,
            },
        );
    }

    fn get_var(&mut self, identifier: Token) -> Option<VariableDefinition> {
        for env in self.environments.iter().rev() {
            if let Some(def) = env.get(&identifier.lexeme) {
                return Some(def.clone());
            }
        }
        None
    }

    fn push_env(&mut self) {
        self.environments.push(HashMap::new());
    }

    fn pop_env(&mut self) {
        self.environments.pop();
    }
}
