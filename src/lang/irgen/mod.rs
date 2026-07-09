// use crate::lang::parser::{expressions::Expr, statements::Statement};

// struct IrGenerator {
//     temp_var_count: i64,
//     label_count: i64,
// }

// impl IrGenerator {
//     fn new() -> Self {
//         IrGenerator {
//             temp_var_count: 0,
//             label_count: 0,
//         }
//     }
//     pub fn generate_ir(program: Statement) -> String {
//         Self::new().translate_statement(program)
//     }

//     fn translate_statement(&self, statement: Statement) -> String {
//         let mut buf = String::new();

//         match statement {
//             Statement::Block { statements, .. } => {
//                 for stmt in statements {
//                     buf.push_str(&self.translate_statement(stmt));
//                 }
//             }
//             Statement::Expression(expr) => {
//                 buf.push_str(&self.translate_expression(expr));
//             }
//             Statement::For {
//                 init,
//                 condition,
//                 increment,
//                 body,
//                 ..
//             } => {}
//         };

//         return buf;
//     }

//     fn translate_expression(&self, expression: Expr) -> String {
//         match expression {
//             Expr::Assign { name, value } => {

//             },
//             Expr::Binary { left, operator, right } => {

//             }
//         }
//     }

//     fn get_temp_var(&mut self) -> String {
//         let name = format!("t{}", self.temp_var_count);
//         self.temp_var_count += 1;
//         name
//     }

//     fn get_label(&mut self) -> String {
//         let name = format!("L{}", self.label_count);
//         self.label_count += 1;
//         name
//     }
// }
