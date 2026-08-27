#include "parser_new.h"
#include <iostream>
#include <cctype>
#include <algorithm>

namespace ibex {

ParserNew::ParserNew(const std::vector<Token>& tokens, Arena& arena)
    : tokens_(tokens), program_(arena) {}

// ============================================================================
// HELPERS
// ============================================================================

bool ParserNew::check(TokenType type) const {
    if (is_at_end()) return false;
    return current().type == type;
}

bool ParserNew::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool ParserNew::match_any(std::span<const TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token ParserNew::current() const {
    if (current_ >= tokens_.size()) {
        return tokens_.back();  // Return EOF
    }
    return tokens_[current_];
}

Token ParserNew::peek() const {
    if (current_ + 1 >= tokens_.size()) {
        return tokens_.back();  // Return EOF
    }
    return tokens_[current_ + 1];
}

Token ParserNew::advance() {
    if (!is_at_end()) {
        current_++;
    }
    std::cerr << "Parser advanced past: '" << tokens_[current_ - 1].lexeme << "'\n";
    return tokens_[current_ - 1];
}

void ParserNew::error(std::string_view message) {
    Token t = current();
    std::string err_msg = "Line " + std::to_string(t.line) + ": " + std::string(message);
    errors_.push_back(err_msg);
}

bool ParserNew::is_at_end() const {
    return current().type == TokenType::EOF_TOKEN;
}

bool ParserNew::is_type_keyword() const {
    return is_primitive_type() || check(TokenType::IDENTIFIER) ||
           check(TokenType::STRUCT) || check(TokenType::CLASS);
}

bool ParserNew::is_primitive_type() const {
    TokenType type = current().type;
    return type == TokenType::I8 || type == TokenType::I16 ||
           type == TokenType::I32 || type == TokenType::I64 ||
           type == TokenType::U8 || type == TokenType::U16 ||
           type == TokenType::U32 || type == TokenType::U64 ||
           type == TokenType::BYTE || 
           type == TokenType::F32 || type == TokenType::F64 ||
           type == TokenType::BOOL;
}

std::vector<Str> ParserNew::parse_attributes() {
    std::vector<Str> attributes;

    while (check(TokenType::LBRACKET_LBRACKET)) {
        advance();  // consume [[
        if (check(TokenType::IDENTIFIER)) {
            attributes.push_back(alloc_str(advance().lexeme));
        }
        if (!match(TokenType::RBRACKET_RBRACKET)) {
            error("Expected ']]' after attribute");
        }
    }

    return attributes;
}

// ============================================================================
// PROGRAM PARSING
// ============================================================================

std::vector<DeclHandle> ParserNew::parse_program() {
    std::vector<DeclHandle> declarations;

    while (!is_at_end()) {
        auto decl_handle = parse_declaration();
        if (!decl_handle.is_null()) {
            declarations.push_back(decl_handle);
        } else {
            // Panic mode recovery
            advance();
        }
    }

    return declarations;
}

// ============================================================================
// DECLARATIONS
// ============================================================================

DeclHandle ParserNew::parse_declaration() {
    // Check for struct/class/enum/flag keywords first
    if (check(TokenType::STRUCT)) {
        advance();
        return parse_struct_decl();
    }
    if (check(TokenType::CLASS)) {
        advance();
        return parse_class_decl();
    }
    if (check(TokenType::ENUM)) {
        advance();
        return parse_enum_decl();
    }
    if (check(TokenType::FLAG)) {
        advance();
        return parse_flag_decl();
    }
    if (check(TokenType::USING)) {
        advance();
        return parse_function_binding_decl();
    }

    if (check(TokenType::CONST_KW)) {
        if (peek().type != TokenType::LPAREN) {
            advance(); // consume const
            return parse_variable_decl(true);
        }
    }

    // Check for name: ... pattern
    if (check(TokenType::IDENTIFIER)) {
        size_t save_pos = current_;
        Token name_token = advance();

        if (match(TokenType::COLON)) {
            // Could be variable, function, or type declaration
            if (check(TokenType::LPAREN)) {
                // Function: name: (params) -> return_type { body }
                current_ = save_pos;  // Reset
                return parse_function_decl();
            } else {
                // Variable: name: type = value; or name := value;
                current_ = save_pos;  // Reset
                return parse_variable_decl();
            }
        } else if (check(TokenType::COLON_EQUAL)) {
            // Type deduction: name := value;
            current_ = save_pos;  // Reset
            return parse_variable_decl();
        }
    }

    error("Expected declaration (struct, class, enum, or function/variable)");
    return DeclHandle();  // null handle
}

DeclHandle ParserNew::parse_function_decl() {
    if (!check(TokenType::IDENTIFIER)) {
        error("Expected function name");
        return DeclHandle();
    }

    Str func_name = alloc_str(advance().lexeme);

    if (!match(TokenType::COLON)) {
        error("Expected ':' after function name");
        return DeclHandle();
    }

    if (!match(TokenType::LPAREN)) {
        error("Expected '(' after ':'");
        return DeclHandle();
    }

    auto params = parse_parameter_list();

    if (!match(TokenType::RPAREN)) {
        error("Expected ')' after parameters");
        return DeclHandle();
    }

    if (!match(TokenType::ARROW)) {
        error("Expected '->' before return type");
        return DeclHandle();
    }

    TypeHandle return_type = parse_type();

    // Parse attributes
    auto attributes = parse_attributes();

    if (!match(TokenType::LBRACE)) {
        error("Expected '{' to start function body");
        return DeclHandle();
    }

    // Parse function body as a block statement
    std::vector<StmtHandle> body_stmts;
    while (!check(TokenType::RBRACE) && !is_at_end()) {
        auto stmt = parse_statement();
        if (!stmt.is_null()) {
            body_stmts.push_back(stmt);
        } else {
            // Panic mode recovery
            advance();
            while (!check(TokenType::RBRACE) && !check(TokenType::SEMICOLON) && !is_at_end()) {
                advance();
            }
            if (check(TokenType::SEMICOLON)) advance();
        }
    }

    if (!match(TokenType::RBRACE)) {
        error("Expected '}' after function body");
        return DeclHandle();
    }

    // Create block statement for body
    std::span<StmtHandle> body_span = program_.allocate_array(body_stmts);
    StmtHandle body = store_stmt(BlockStmt{body_span});

    // Allocate function parameters array
    std::span<FunctionParameter> params_span = program_.allocate_array(params);
    std::span<Str> attr_span = program_.allocate_array(attributes);

    FunctionDecl func{
        .name = func_name,
        .parameters = params_span,
        .return_type = return_type,
        .body = body,
        .attributes = attr_span
    };

    return store_decl(func);
}

DeclHandle ParserNew::parse_struct_decl() {
    if (!check(TokenType::IDENTIFIER)) {
        error("Expected struct name");
        return DeclHandle();
    }

    Str struct_name = alloc_str(advance().lexeme);

    // Parse optional base structs: struct S : Base1, Base2 { ... }
    std::vector<Str> bases;
    if (match(TokenType::COLON)) {
        while (true) {
            if (!check(TokenType::IDENTIFIER)) {
                error("Expected base struct name");
                return DeclHandle();
            }
            bases.push_back(alloc_str(advance().lexeme));
            
            if (!match(TokenType::COMMA)) {
                break;
            }
        }
    }

    if (!match(TokenType::LBRACE)) {
        error("Expected '{' after struct name/bases");
        return DeclHandle();
    }

    std::vector<StructMember> members;

    while (!check(TokenType::RBRACE) && !is_at_end()) {
        // Parse optional offset annotation [[offset:N]]
        std::optional<uint32_t> offset;
        while (check(TokenType::LBRACKET_LBRACKET)) {
            advance();  // consume [[
            if (check(TokenType::IDENTIFIER)) {
                std::string attr = std::string(advance().lexeme);
                if (attr == "offset") {
                    if (!match(TokenType::COLON)) {
                        error("Expected ':' in offset annotation");
                        return DeclHandle();
                    }
                    if (!check(TokenType::INTEGER_LITERAL)) {
                        error("Expected integer value for offset");
                        return DeclHandle();
                    }
                    auto offset_token = advance();
                    offset = static_cast<uint32_t>(offset_token.value.int_value);
                }
            }
            if (!match(TokenType::RBRACKET_RBRACKET)) {
                error("Expected ']]' after attribute");
            }
        }

        if (check(TokenType::IDENTIFIER)) {
            Str member_name = alloc_str(advance().lexeme);

            if (!match(TokenType::COLON)) {
                error("Expected ':' after member name");
                return DeclHandle();
            }

            TypeHandle member_type = parse_type();

            std::optional<ExprHandle> default_value;
            if (match(TokenType::EQUAL)) {
                default_value = parse_expression();
            }

            members.push_back(StructMember{
                .name = member_name,
                .type = member_type,
                .default_value = default_value,
                .offset = offset
            });

            if (!match(TokenType::SEMICOLON)) {
                error("Expected ';' after struct member");
                return DeclHandle();
            }
        }
    }

    if (!match(TokenType::RBRACE)) {
        error("Expected '}' after struct body");
        return DeclHandle();
    }

    std::span<StructMember> members_span = program_.allocate_array(members);
    std::span<Str> bases_span = program_.allocate_array(bases);
    StructDecl decl{
        .name = struct_name,
        .bases = bases_span,
        .members = members_span
    };

    return store_decl(decl);
}

DeclHandle ParserNew::parse_class_decl() {
    error("Class declaration not yet implemented");
    return DeclHandle();
}

DeclHandle ParserNew::parse_enum_decl() {
    if (!check(TokenType::IDENTIFIER)) {
        error("Expected enum name");
        return DeclHandle();
    }

    Str enum_name = alloc_str(advance().lexeme);

    TypeHandle base_type;
    std::optional<Str> extends;
    
    if (match(TokenType::COLON)) {
        if (is_primitive_type()) {
            base_type = parse_type();
        } else if (check(TokenType::IDENTIFIER)) {
            // Could be an enum to extend - parse the identifier
            Str base_name = alloc_str(advance().lexeme);
            extends = base_name;
            
            // For now, we'll create a NamedType for the base_type (will be resolved later)
            Type named_type = NamedType{.name = base_name};
            base_type = store_type(named_type);
        } else {
            error("Expected type or enum name after ':'");
            return DeclHandle();
        }
    } else {
        // Default base type is i32
        Type primitive = PrimitiveType{.primitive = TokenType::I32};
        base_type = store_type(primitive);
    }

    if (!match(TokenType::LBRACE)) {
        error("Expected '{' after enum base type");
        return DeclHandle();
    }

    std::vector<EnumMember> members;

    while (!check(TokenType::RBRACE) && !is_at_end()) {
        if (check(TokenType::IDENTIFIER)) {
            Str member_name = alloc_str(advance().lexeme);
            
            std::optional<ExprHandle> value;
            if (match(TokenType::EQUAL)) {
                value = parse_expression();
            }

            members.push_back(EnumMember{
                .name = member_name,
                .value = value
            });

            if (!check(TokenType::RBRACE)) {
                if (!match(TokenType::COMMA)) {
                    error("Expected ',' after enum member");
                    break;
                }
            }
        } else {
            error("Expected identifier for enum member");
            break;
        }
    }

    if (!match(TokenType::RBRACE)) {
        error("Expected '}' after enum body");
        return DeclHandle();
    }

    std::span<EnumMember> members_span = program_.allocate_array(members);
    EnumDecl decl{
        .name = enum_name,
        .base_type = base_type,
        .extends = extends,
        .members = members_span
    };

    return store_decl(decl);
}

DeclHandle ParserNew::parse_flag_decl() {
    if (!check(TokenType::IDENTIFIER)) {
        error("Expected flag name");
        return DeclHandle();
    }

    Str flag_name = alloc_str(advance().lexeme);

    TypeHandle base_type;
    std::optional<Str> extends;

    if (match(TokenType::COLON)) {
        if (is_primitive_type()) {
            base_type = parse_type();
        } else if (check(TokenType::IDENTIFIER)) {
            extends = alloc_str(advance().lexeme);
        } else {
            error("Expected primitive type or flag name to extend after ':'");
            return DeclHandle();
        }
    }

    if (!match(TokenType::LBRACE)) {
        error("Expected '{' after flag declaration");
        return DeclHandle();
    }

    std::vector<EnumMember> members;

    while (!check(TokenType::RBRACE) && !is_at_end()) {
        if (check(TokenType::IDENTIFIER)) {
            Str member_name = alloc_str(advance().lexeme);
            
            // Flags can have optional explicit values (usually for zero value)
            std::optional<ExprHandle> value;
            if (match(TokenType::EQUAL)) {
                value = parse_expression();
            }

            members.push_back(EnumMember{
                .name = member_name,
                .value = value
            });

            if (!check(TokenType::RBRACE)) {
                if (!match(TokenType::COMMA)) {
                    error("Expected ',' after flag member");
                    break;
                }
            }
        } else {
            error("Expected identifier for flag member");
            break;
        }
    }

    if (!match(TokenType::RBRACE)) {
        error("Expected '}' after flag body");
        return DeclHandle();
    }

    std::span<EnumMember> members_span = program_.allocate_array(members);
    FlagDecl decl{
        .name = flag_name,
        .base_type = base_type,
        .extends = extends,
        .members = members_span
    };

    return store_decl(decl);
}

DeclHandle ParserNew::parse_function_binding_decl() {
    // using name := #function_name(arg1, , arg3);
    if (!check(TokenType::IDENTIFIER)) {
        error("Expected variable name for function binding");
        return DeclHandle();
    }

    Str binding_name = alloc_str(advance().lexeme);

    if (!match(TokenType::COLON_EQUAL)) {
        error("Expected ':=' for function binding");
        return DeclHandle();
    }

    if (!match(TokenType::HASH)) {
        error("Expected '#' for compile-time function reference");
        return DeclHandle();
    }

    if (!check(TokenType::IDENTIFIER)) {
        error("Expected function name after '#'");
        return DeclHandle();
    }

    Str target_func = alloc_str(advance().lexeme);

    if (!match(TokenType::LPAREN)) {
        error("Expected '(' after function name");
        return DeclHandle();
    }

    // Parse the binding arguments (empty slots for unbound parameters)
    std::vector<std::optional<ExprHandle>> bound_args;

    while (!check(TokenType::RPAREN)) {
        if (match(TokenType::COMMA)) {
            // Empty argument - parameter left open
            bound_args.push_back(std::nullopt);
        } else {
            // Actual argument to bind
            bound_args.push_back(parse_expression());
            if (!check(TokenType::RPAREN)) {
                if (!match(TokenType::COMMA)) {
                    error("Expected ',' or ')' in binding arguments");
                }
            }
        }
    }

    if (!match(TokenType::RPAREN)) {
        error("Expected ')' after binding arguments");
        return DeclHandle();
    }

    if (!match(TokenType::SEMICOLON)) {
        error("Expected ';' after function binding");
        return DeclHandle();
    }

    std::span<std::optional<ExprHandle>> args_span = program_.allocate_array(bound_args);
    FunctionBindingDecl decl{
        .name = binding_name,
        .target_function = target_func,
        .bound_args = args_span
    };

    return store_decl(decl);
}

DeclHandle ParserNew::parse_variable_decl(bool is_const) {
    if (!check(TokenType::IDENTIFIER)) {
        error("Expected variable name");
        return DeclHandle();
    }

    Str var_name = alloc_str(advance().lexeme);
    std::optional<TypeHandle> var_type;
    std::optional<ExprHandle> initializer;

    if (match(TokenType::COLON_EQUAL)) {
        // Type deduction: name := value;
        var_type = std::nullopt;
        initializer = parse_expression();
    } else if (match(TokenType::COLON)) {
        // Explicit type: name: type = value;
        var_type = parse_type();
        if (match(TokenType::EQUAL)) {
            initializer = parse_expression();
        }
    } else {
        error("Expected ':' or ':=' after variable name");
        return DeclHandle();
    }

    if (!var_type && !initializer) {
        error("Variable with type deduction requires initializer");
        return DeclHandle();
    }

    if (!match(TokenType::SEMICOLON)) {
        error("Expected ';' after variable declaration");
        return DeclHandle();
    }

    VariableDecl decl{
        .name = var_name,
        .type = var_type,
        .initializer = initializer,
        .is_const = is_const
    };

    return store_decl(decl);
}

std::vector<FunctionParameter> ParserNew::parse_parameter_list() {
    std::vector<FunctionParameter> params;

    while (!check(TokenType::RPAREN)) {
        bool is_const = false;
        if (match(TokenType::CONST_KW)) {
            is_const = true;
        }
        
        if (!check(TokenType::IDENTIFIER)) {
            error("Expected parameter name");
            return params;
        }

        Str param_name = alloc_str(advance().lexeme);

        if (!match(TokenType::COLON)) {
            error("Expected ':' after parameter name");
            return params;
        }

        TypeHandle param_type = parse_type();

        std::optional<ExprHandle> default_value;
        if (match(TokenType::LBRACE)) {
            default_value = parse_expression();
            if (!match(TokenType::RBRACE)) {
                error("Expected '}' after default value");
            }
        }

        params.push_back(FunctionParameter{
            .name = param_name,
            .type = param_type,
            .default_value = default_value,
            .is_const = is_const
        });

        if (!check(TokenType::RPAREN)) {
            if (!match(TokenType::COMMA)) {
                error("Expected ',' between parameters");
            }
        }
    }

    return params;
}

// ============================================================================
// STATEMENTS
// ============================================================================

StmtHandle ParserNew::parse_statement() {
    if (match(TokenType::LBRACE)) {
        return parse_block_statement();
    }
    if (match(TokenType::RETURN)) {
        return parse_return_statement();
    }
    if (match(TokenType::IF)) {
        return parse_if_statement();
    }
    if (match(TokenType::WHILE)) {
        return parse_while_statement();
    }
    if (match(TokenType::FOR)) {
        return parse_for_statement();
    }
    if (match(TokenType::BREAK)) {
        if (!match(TokenType::SEMICOLON)) {
            error("Expected ';' after break");
        }
        return store_stmt(BreakStmt{});
    }
    if (match(TokenType::CONTINUE)) {
        if (!match(TokenType::SEMICOLON)) {
            error("Expected ';' after continue");
        }
        return store_stmt(ContinueStmt{});
    }

    if (match(TokenType::CONST_KW)) {
        if (match(TokenType::LPAREN)) {
            std::vector<Str> vars;
            do {
                if (check(TokenType::IDENTIFIER)) {
                    vars.push_back(alloc_str(advance().lexeme));
                } else {
                    error("Expected variable name in const()");
                    break;
                }
            } while (match(TokenType::COMMA));
            
            if (!match(TokenType::RPAREN)) {
                error("Expected ')' after const variables");
            }
            
            if (match(TokenType::SEMICOLON)) {
                std::span<Str> vars_span = program_.allocate_array(vars);
                return store_stmt(ConstModifierStmt{vars_span});
            } else if (match(TokenType::LBRACE)) {
                auto body = parse_block_statement();
                std::span<Str> vars_span = program_.allocate_array(vars);
                return store_stmt(ConstBlockStmt{vars_span, body});
            } else {
                error("Expected ';' or '{' after const(...)");
                return StmtHandle();
            }
        } else {
            return parse_var_decl_statement(true);
        }
    }

    // Check for variable declaration
    if (check(TokenType::IDENTIFIER)) {
        size_t save_pos = current_;
        Token name_token = advance();
        
        if (check(TokenType::COLON) || check(TokenType::COLON_EQUAL)) {
            current_ = save_pos;  // Reset
            return parse_var_decl_statement();
        }
        current_ = save_pos;  // Reset (fallthrough to expression)
    }

    return parse_expression_statement();
}

StmtHandle ParserNew::parse_block_statement() {
    std::vector<StmtHandle> stmts;

    while (!check(TokenType::RBRACE) && !is_at_end()) {
        auto stmt = parse_statement();
        if (!stmt.is_null()) {
            stmts.push_back(stmt);
        } else {
            // Panic mode recovery
            advance();
            while (!check(TokenType::RBRACE) && !check(TokenType::SEMICOLON) && !is_at_end()) {
                advance();
            }
            if (check(TokenType::SEMICOLON)) advance();
        }
    }

    if (!match(TokenType::RBRACE)) {
        error("Expected '}' after block");
        return StmtHandle();
    }

    std::span<StmtHandle> stmts_span = program_.allocate_array(stmts);
    return store_stmt(BlockStmt{stmts_span});
}

StmtHandle ParserNew::parse_return_statement() {
    std::optional<ExprHandle> value;
    
    if (!check(TokenType::SEMICOLON)) {
        value = parse_expression();
    }
    
    if (!match(TokenType::SEMICOLON)) {
        error("Expected ';' after return statement");
    }
    
    return store_stmt(ReturnStmt{value});
}

StmtHandle ParserNew::parse_if_statement() {
    auto condition = parse_expression();
    
    if (!match(TokenType::LBRACE)) {
        error("Expected '{' after if condition");
        return StmtHandle();
    }
    
    auto then_branch = parse_block_statement();
    std::optional<StmtHandle> else_branch;
    
    if (match(TokenType::ELSE)) {
        if (check(TokenType::IF)) {
            advance();  // consume 'if'
            else_branch = parse_if_statement();
        } else if (!match(TokenType::LBRACE)) {
            error("Expected '{' after else");
            return StmtHandle();
        } else {
            else_branch = parse_block_statement();
        }
    }
    
    return store_stmt(IfStmt{condition, then_branch, else_branch});
}

StmtHandle ParserNew::parse_while_statement() {
    auto condition = parse_expression();
    
    if (!match(TokenType::LBRACE)) {
        error("Expected '{' after while condition");
        return StmtHandle();
    }
    
    auto body = parse_block_statement();
    
    // Parse optional else block
    std::optional<StmtHandle> else_branch;
    if (match(TokenType::ELSE)) {
        if (match(TokenType::LBRACE)) {
            else_branch = parse_block_statement();
        } else {
            error("Expected '{' after else");
            return StmtHandle();
        }
    }
    
    return store_stmt(WhileStmt{condition, body, else_branch});
}

StmtHandle ParserNew::parse_for_statement() {
    if (!check(TokenType::IDENTIFIER)) {
        error("Expected variable in for loop");
        return StmtHandle();
    }
    
    Str loop_var = alloc_str(advance().lexeme);
    
    if (!match(TokenType::IN)) {
        error("Expected 'in' after for variable");
        return StmtHandle();
    }
    
    auto range_expr = parse_expression();
    
    if (!match(TokenType::LBRACE)) {
        error("Expected '{' after for range expression");
        return StmtHandle();
    }
    
    auto body = parse_block_statement();
    
    // Parse optional else block
    std::optional<StmtHandle> else_branch;
    if (match(TokenType::ELSE)) {
        if (match(TokenType::LBRACE)) {
            else_branch = parse_block_statement();
        } else {
            error("Expected '{' after else");
            return StmtHandle();
        }
    }
    
    return store_stmt(ForStmt{loop_var, range_expr, body, else_branch});
}

StmtHandle ParserNew::parse_var_decl_statement(bool is_const) {
    auto decl_handle = parse_variable_decl(is_const);
    
    if (decl_handle.is_null()) {
        return StmtHandle();
    }
    
    // Retrieve the parsed VariableDecl from the program
    if (auto* var_decl = std::get_if<VariableDecl>(&program_.declarations[decl_handle.index])) {
        VarDeclStmt stmt{
            .name = var_decl->name,
            .type = var_decl->type,
            .initializer = var_decl->initializer,
            .is_const = var_decl->is_const
        };
        return store_stmt(stmt);
    }
    
    return StmtHandle();
}

StmtHandle ParserNew::parse_expression_statement() {
    auto expr = parse_expression();
    
    if (expr.is_null()) {
        return StmtHandle();
    }
    
    if (!match(TokenType::SEMICOLON)) {
        // Allow implicit semicolon at end of block
        if (!check(TokenType::RBRACE) && !is_at_end()) {
            error("Expected ';' after expression");
        }
    }
    
    return store_stmt(ExprStmt{expr});
}

// ============================================================================
// EXPRESSIONS
// ============================================================================

ExprHandle ParserNew::parse_expression() {
    return parse_assignment();
}

ExprHandle ParserNew::parse_assignment() {
    auto expr = parse_logical_or();
    
    TokenType op = current().type;
    if (op == TokenType::EQUAL || op == TokenType::PLUS_EQUAL || 
        op == TokenType::MINUS_EQUAL || op == TokenType::STAR_EQUAL || 
        op == TokenType::SLASH_EQUAL) {
        advance();
        auto right = parse_assignment();
        // Return a dummy BinaryExpr or AssignmentExpr
        // We'll use AssignmentExpr if it exists, or just return expr
        return expr;
    }
    
    return expr;
}

ExprHandle ParserNew::parse_logical_or() {
    auto left = parse_logical_and();
    
    while (match(TokenType::PIPE_PIPE)) {
        auto right = parse_logical_and();
        BinaryExpr binop{left, TokenType::PIPE_PIPE, right};
        left = store_expr(binop);
    }
    
    return left;
}

ExprHandle ParserNew::parse_logical_and() {
    auto left = parse_bitwise_or();
    
    while (match(TokenType::AMPERSAND_AMPERSAND)) {
        auto right = parse_bitwise_or();
        BinaryExpr binop{left, TokenType::AMPERSAND_AMPERSAND, right};
        left = store_expr(binop);
    }
    
    return left;
}

ExprHandle ParserNew::parse_bitwise_or() {
    auto left = parse_bitwise_xor();
    
    while (match(TokenType::PIPE)) {
        auto right = parse_bitwise_xor();
        BinaryExpr binop{left, TokenType::PIPE, right};
        left = store_expr(binop);
    }
    
    return left;
}

ExprHandle ParserNew::parse_bitwise_xor() {
    auto left = parse_bitwise_and();
    
    while (match(TokenType::CARET)) {
        auto right = parse_bitwise_and();
        BinaryExpr binop{left, TokenType::CARET, right};
        left = store_expr(binop);
    }
    
    return left;
}

ExprHandle ParserNew::parse_bitwise_and() {
    auto left = parse_equality();
    
    while (match(TokenType::AMPERSAND)) {
        auto right = parse_equality();
        BinaryExpr binop{left, TokenType::AMPERSAND, right};
        left = store_expr(binop);
    }
    
    return left;
}

ExprHandle ParserNew::parse_equality() {
    auto left = parse_comparison();
    
    while (match(TokenType::EQ_EQ) || match(TokenType::NOT_EQ)) {
        TokenType op = tokens_[current_ - 1].type;
        auto right = parse_comparison();
        BinaryExpr binop{left, op, right};
        left = store_expr(binop);
    }
    
    return left;
}

ExprHandle ParserNew::parse_comparison() {
    auto left = parse_shift();
    
    while (match(TokenType::LESS) || match(TokenType::GREATER) ||
           match(TokenType::LESS_EQ) || match(TokenType::GREATER_EQ)) {
        TokenType op = tokens_[current_ - 1].type;
        auto right = parse_shift();
        BinaryExpr binop{left, op, right};
        left = store_expr(binop);
    }
    
    return left;
}

ExprHandle ParserNew::parse_shift() {
    auto left = parse_addition();
    
    while (match(TokenType::LSHIFT) || match(TokenType::RSHIFT)) {
        TokenType op = tokens_[current_ - 1].type;
        auto right = parse_addition();
        BinaryExpr binop{left, op, right};
        left = store_expr(binop);
    }
    
    return left;
}

ExprHandle ParserNew::parse_addition() {
    auto left = parse_multiplication();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        TokenType op = tokens_[current_ - 1].type;
        auto right = parse_multiplication();
        BinaryExpr binop{left, op, right};
        left = store_expr(binop);
    }
    
    return left;
}

ExprHandle ParserNew::parse_multiplication() {
    auto left = parse_unary();
    
    while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::PERCENT)) {
        TokenType op = tokens_[current_ - 1].type;
        auto right = parse_unary();
        BinaryExpr binop{left, op, right};
        left = store_expr(binop);
    }
    
    return left;
}

ExprHandle ParserNew::parse_unary() {
    if (match(TokenType::BANG) || match(TokenType::MINUS) || match(TokenType::STAR) ||
        match(TokenType::AMPERSAND) || match(TokenType::PLUS) || match(TokenType::TILDE)) {
        TokenType op = tokens_[current_ - 1].type;
        auto right = parse_unary();
        UnaryExpr unop{op, right};
        return store_expr(unop);
    }
    
    return parse_postfix();
}

ExprHandle ParserNew::parse_postfix() {
    auto expr = parse_primary();
    
    while (true) {
        if (match(TokenType::AS)) {
            // Type casting: expr as type
            auto cast_type = parse_type();
            CastExpr cast{expr, cast_type};
            expr = store_expr(cast);
        } else if (check(TokenType::LBRACE)) {
            // Check if expr is an identifier and this is struct initialization
            if (std::holds_alternative<IdentifierExpr>(program_.expressions[expr.index])) {
                advance(); // consume {
                IdentifierExpr& id_expr = std::get<IdentifierExpr>(program_.expressions[expr.index]);
                
                // Parse as struct initialization: Point { x: 10, y: 20 }
                std::vector<NamedArg> field_values;
                std::vector<ExprHandle> positional_values;
                
                while (!check(TokenType::RBRACE) && !is_at_end()) {
                    // Check if this is a named field
                    size_t save_pos = current_;
                    bool is_named = false;
                    std::string field_name;
                    
                    if (check(TokenType::IDENTIFIER)) {
                        Token name_tok = advance();
                        if (match(TokenType::COLON) || match(TokenType::EQUAL)) {
                            is_named = true;
                            field_name = name_tok.lexeme;
                        } else {
                            current_ = save_pos; // Reset
                        }
                    }
                    
                    if (is_named) {
                        auto val_expr = parse_expression();
                        field_values.push_back(NamedArg{
                            .name = alloc_str(field_name),
                            .value = val_expr
                        });
                    } else {
                        positional_values.push_back(parse_expression());
                    }
                    
                    if (!match(TokenType::COMMA)) {
                        break;
                    }
                }
                
                if (!match(TokenType::RBRACE)) {
                    error("Expected '}' after struct initializer");
                }
                
                StructInitExpr init;
                init.type_name = id_expr.name;
                init.field_values = program_.allocate_array(field_values);
                init.positional_values = program_.allocate_array(positional_values);
                expr = store_expr(init);
            } else {
                // Not an identifier, so this '{' is not for struct initialization.
                // It's probably the start of a block (like in if/while loops).
                break;
            }
        } else if (match(TokenType::LPAREN)) {
            // Function call: expr(args) or expr(name=expr, ...)
            std::vector<ExprHandle> args;
            std::vector<NamedArg> named_args;
            
            while (!check(TokenType::RPAREN)) {
                // Check if this is a named argument: ident = expr
                size_t save_pos = current_;
                bool is_named = false;
                std::string arg_name;
                
                if (check(TokenType::IDENTIFIER)) {
                    Token id = advance();
                    arg_name = std::string(id.lexeme);
                    if (match(TokenType::EQUAL)) {
                        is_named = true;
                    } else {
                        // Not a named arg, reset and parse as expression
                        current_ = save_pos;
                    }
                }
                
                ExprHandle arg_expr = parse_expression();
                
                if (is_named) {
                    named_args.push_back(NamedArg{
                        .name = alloc_str(arg_name),
                        .value = arg_expr
                    });
                } else {
                    args.push_back(arg_expr);
                }
                
                if (!check(TokenType::RPAREN)) {
                    if (!match(TokenType::COMMA)) {
                        error("Expected ',' between arguments");
                    }
                }
            }
            if (!match(TokenType::RPAREN)) {
                error("Expected ')' after arguments");
            }
            std::span<ExprHandle> args_span = program_.allocate_array(args);
            std::span<NamedArg> named_args_span = program_.allocate_array(named_args);
            CallExpr call{expr, args_span, named_args_span};
            expr = store_expr(call);
        } else if (match(TokenType::DOT)) {
            // Member access or UFCS
            if (!check(TokenType::IDENTIFIER)) {
                error("Expected member name after '.'");
            }
            Str member = alloc_str(advance().lexeme);
            
            if (check(TokenType::LPAREN)) {
                // UFCS function call: expr.func(args)
                advance();  // consume (
                std::vector<ExprHandle> args;
                while (!check(TokenType::RPAREN)) {
                    args.push_back(parse_expression());
                    if (!check(TokenType::RPAREN)) {
                        if (!match(TokenType::COMMA)) {
                            error("Expected ',' between arguments");
                        }
                    }
                }
                if (!match(TokenType::RPAREN)) {
                    error("Expected ')' after arguments");
                }
                // Create UFCS call (prepend expr as first arg)
                std::vector<ExprHandle> all_args;
                all_args.push_back(expr);
                all_args.insert(all_args.end(), args.begin(), args.end());
                std::span<ExprHandle> args_span = program_.allocate_array(all_args);
                
                // Create identifier for the function
                IdentifierExpr func_id{member};
                ExprHandle func_handle = store_expr(func_id);
                
                std::vector<NamedArg> empty_named_args;
                std::span<NamedArg> named_args_span = program_.allocate_array(empty_named_args);
                CallExpr call{func_handle, args_span, named_args_span};
                expr = store_expr(call);
            } else {
                // Simple member access
                MemberExpr member_access{expr, member};
                expr = store_expr(member_access);
            }
        } else if (match(TokenType::LBRACK)) {
            // Array indexing or slicing: expr[index] or expr[start:end]
            std::optional<ExprHandle> start;
            std::optional<ExprHandle> end;
            
            // Check for slice syntax with leading colon: expr[:end]
            if (check(TokenType::COLON)) {
                start = std::nullopt;
            } else {
                start = parse_expression();
            }
            
            if (match(TokenType::COLON)) {
                // Slice: expr[start:end] or expr[start:] or expr[:end]
                if (!check(TokenType::RBRACK)) {
                    end = parse_expression();
                }
                
                if (!match(TokenType::RBRACK)) {
                    error("Expected ']' after slice");
                }
                
                SliceExpr slice{expr, 
                                start.value_or(ExprHandle()), 
                                end.value_or(ExprHandle())};
                expr = store_expr(slice);
            } else {
                // Simple indexing: expr[index]
                if (!match(TokenType::RBRACK)) {
                    error("Expected ']' after array index");
                }
                
                if (!start) {
                    error("Expected index expression");
                }
                IndexExpr index_access{expr, start.value()};
                expr = store_expr(index_access);
            }
        } else {
            break;
        }
    }
    
    return expr;
}

ExprHandle ParserNew::parse_primary() {
    if (match(TokenType::TRUE_LITERAL)) {
        LiteralExpr lit{LiteralExpr::Kind::BOOLEAN};
        lit.value.bool_value = true;
        return store_expr(lit);
    }
    
    if (match(TokenType::FALSE_LITERAL)) {
        LiteralExpr lit{LiteralExpr::Kind::BOOLEAN};
        lit.value.bool_value = false;
        return store_expr(lit);
    }
    
    if (match(TokenType::NULL_LITERAL)) {
        LiteralExpr lit{LiteralExpr::Kind::NULL_VALUE};
        return store_expr(lit);
    }
    
    if (match(TokenType::INTEGER_LITERAL)) {
        Token lit_token = tokens_[current_ - 1];
        LiteralExpr lit{LiteralExpr::Kind::INTEGER};
        lit.value.int_value = std::stoll(lit_token.lexeme);
        return store_expr(lit);
    }
    
    if (match(TokenType::FLOAT_LITERAL)) {
        Token lit_token = tokens_[current_ - 1];
        LiteralExpr lit{LiteralExpr::Kind::FLOAT};
        lit.value.float_value = std::stod(lit_token.lexeme);
        return store_expr(lit);
    }
    
    if (match(TokenType::STRING_LITERAL)) {
        LiteralExpr lit{LiteralExpr::Kind::STRING};
        lit.value.string_value = alloc_str(tokens_[current_ - 1].lexeme);
        return store_expr(lit);
    }
    
    if (match(TokenType::IDENTIFIER)) {
        IdentifierExpr id{alloc_str(tokens_[current_ - 1].lexeme)};
        return store_expr(id);
    }
    
    // Allocation expressions: @alloc(...) and @alloc_vec(...)
    if (check(TokenType::AT)) {
        if (peek().type == TokenType::IDENTIFIER) {
            Token peek_tok = peek();
            std::string_view next_lex = peek_tok.lexeme;
            if (next_lex == "alloc" || next_lex == "alloc_vec") {
                advance(); // consume @
                Token alloc_token = advance(); // consume identifier
                
                if (match(TokenType::LPAREN)) {
                    TypeHandle element_type = parse_type();
                    std::optional<ExprHandle> size;
                    
                    if (alloc_token.lexeme == "alloc_vec") {
                        if (!match(TokenType::COMMA)) {
                            error("Expected ',' after element type in @alloc_vec");
                            return ExprHandle();
                        }
                        size = parse_expression();
                    }
                    
                    if (!match(TokenType::RPAREN)) {
                        error("Expected ')' after allocation arguments");
                        return ExprHandle();
                    }
                    
                    AllocExpr alloc{
                        .kind = (alloc_token.lexeme == "alloc_vec") ? AllocExpr::Kind::VECTOR : AllocExpr::Kind::SCALAR,
                        .element_type = element_type,
                        .size = size
                    };
                    return store_expr(alloc);
                }
            }
        }
        
        // Address-of operator: @operand
        advance(); // consume @
        auto operand = parse_unary();
        AddressOfExpr addr_of{operand};
        return store_expr(addr_of);
    }
    
    // Array literal: { expr, expr, ... } or [ expr, expr, ... ]
    if (match(TokenType::LBRACK)) {
        std::vector<ExprHandle> elements;
        
        if (!check(TokenType::RBRACK)) {
            elements.push_back(parse_expression());
            
            while (match(TokenType::COMMA)) {
                if (!check(TokenType::RBRACK)) {
                    elements.push_back(parse_expression());
                }
            }
        }
        
        if (!match(TokenType::RBRACK)) {
            error("Expected ']' after array elements");
        }
        
        ArrayLiteralExpr arr_lit;
        arr_lit.elements = program_.allocate_array(elements);
        return store_expr(arr_lit);
    }
    
    if (match(TokenType::LBRACE)) {
        // Try to parse as array literal first (comma-separated expressions)
        // If we see a semicolon, backtrack and parse as block
        std::vector<ExprHandle> elements;
        
        if (!check(TokenType::RBRACE)) {
            elements.push_back(parse_expression());
            
            while (check(TokenType::COMMA)) {
                advance();
                if (check(TokenType::RBRACE)) {
                    break;  // Trailing comma
                }
                elements.push_back(parse_expression());
            }
        }
        
        if (match(TokenType::RBRACE)) {
            // Successfully parsed array literal
            std::span<ExprHandle> elements_span = program_.allocate_array(elements);
            ArrayLiteralExpr arr_lit{elements_span};
            return store_expr(arr_lit);
        }
        
        // Parse as block expression instead
        // We already consumed some elements, so backtrack and reparse
        // For now, just treat as block (simplified)
        error("Mixed block statements and array literals not supported in same block");
        return ExprHandle();
    }
    
    if (match(TokenType::LPAREN)) {
        auto expr = parse_expression();
        if (!match(TokenType::RPAREN)) {
            error("Expected ')' after expression");
        }
        return expr;
    }
    
    error("Expected expression, got '" + std::string(current().lexeme) + "'");
    advance();
    return ExprHandle();
}

// ============================================================================
// TYPES
// ============================================================================

TypeHandle ParserNew::parse_type() {
    // Handle reference types: &T
    if (match(TokenType::AMPERSAND)) {
        TypeHandle base = parse_type();
        ReferenceType ref{base};
        return store_type(ref);
    }
    
    // Handle pointer types: *T
    if (match(TokenType::STAR)) {
        TypeHandle base = parse_type();
        PointerType ptr{base};
        return store_type(ptr);
    }
    
    // Handle array types: [N]T or slice types: [:T] or []T
    if (match(TokenType::LBRACK)) {
        if (match(TokenType::COLON)) {
            // Slice type: [:T]
            if (!match(TokenType::RBRACK)) {
                error("Expected ']' after slice type");
            }
            TypeHandle base = parse_type();
            SliceType slice{base};
            return store_type(slice);
        } else if (match(TokenType::RBRACK)) {
            // Slice type: []T
            TypeHandle base = parse_type();
            SliceType slice{base};
            return store_type(slice);
        } else {
            // Array type: [N]T
            auto size_expr = parse_expression();
            if (!match(TokenType::RBRACK)) {
                error("Expected ']' after array size");
            }
            TypeHandle base = parse_type();
            // TODO: Extract size from size_expr at compile time. For now just set 0.
            ArrayType arr{base, 0};
            return store_type(arr);
        }
    }
    
    return parse_base_type();
}

TypeHandle ParserNew::parse_base_type() {
    if (is_primitive_type()) {
        TokenType prim = current().type;
        advance();
        PrimitiveType prim_type{prim};
        return store_type(prim_type);
    }
    
    if (check(TokenType::IDENTIFIER)) {
        Str name = alloc_str(advance().lexeme);
        NamedType named{name};
        return store_type(named);
    }
    
    error("Expected type");
    return TypeHandle();
}

}  // namespace ibex
