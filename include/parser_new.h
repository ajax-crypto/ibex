#pragma once

#include "lexer.h"
#include "ast_new.h"
#include "arena.h"
#include "str.h"

#include <vector>
#include <optional>

namespace ibex {

// High-performance parser using arena allocator and discriminated unions
class ParserNew {
public:
    explicit ParserNew(const std::vector<Token>& tokens, Program& program);

    // Parse a complete program
    std::vector<DeclHandle> parse_program();

    // Access compiled AST (valid after parse_program completes)
    const Program& program() const { return program_; }
    Program& program() { return program_; }

    // Get error messages
    const std::vector<std::string>& get_errors() const { return errors_; }
    bool has_errors() const { return !errors_.empty(); }

private:
    const std::vector<Token>& tokens_;
    size_t current_ = 0;
    Program& program_;
    std::vector<std::string> errors_;
    bool allow_struct_init_ = true;

    // ========================================================================
    // HELPERS
    // ========================================================================

    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match_any(std::span<const TokenType> types);
    Token current() const;
    Token peek() const;
    Token advance();
    void error(std::string_view message);
    bool is_at_end() const;

    // Allocate string in arena
    Str alloc_str(std::string_view sv) {
        char* ptr = static_cast<char*>(program_.arena().allocate(sv.size()));
        std::memcpy(ptr, sv.data(), sv.size());
        return Str(ptr, sv.size());
    }

    // Store declaration in program and return handle
    DeclHandle store_decl(Decl decl) {
        DeclHandle handle(static_cast<uint32_t>(program_.declarations.size()));
        program_.declarations.push_back(std::move(decl));
        return handle;
    }

    // Store statement in program and return handle
    StmtHandle store_stmt(Stmt stmt) {
        StmtHandle handle(static_cast<uint32_t>(program_.statements.size()));
        program_.statements.push_back(std::move(stmt));
        return handle;
    }

    // Store expression in program and return handle
    ExprHandle store_expr(Expr expr) {
        ExprHandle handle(static_cast<uint32_t>(program_.expressions.size()));
        program_.expressions.push_back(std::move(expr));
        return handle;
    }

    // Store type in program and return handle
    TypeHandle store_type(Type type) {
        TypeHandle handle(static_cast<uint32_t>(program_.types.size()));
        program_.types.push_back(std::move(type));
        return handle;
    }
    // ========================================================================
    // DECLARATIONS
    // ========================================================================

    DeclHandle parse_declaration();
    DeclHandle parse_function_decl(std::span<Attribute> attrs = {});
    DeclHandle parse_struct_decl(std::span<Attribute> attrs = {});
    DeclHandle parse_package_decl(std::span<Attribute> attrs = {});
    DeclHandle parse_module_decl(std::span<Attribute> attrs = {});
    DeclHandle parse_import_decl(std::span<Attribute> attrs = {});
    DeclHandle parse_export_packages_decl(std::span<Attribute> attrs = {});

    DeclHandle parse_enum_decl(std::span<Attribute> attrs = {});
    DeclHandle parse_flag_decl(std::span<Attribute> attrs = {});
    DeclHandle parse_using_decl(std::span<Attribute> attrs = {});
    DeclHandle parse_variable_decl(bool is_const = false, bool is_static = false, std::span<Attribute> attrs = {});

    std::span<Attribute> parse_attributes();
    bool evaluate_platform_attribute(std::span<Attribute> attrs);
    void skip_balanced_block();

    // ========================================================================
    // STATEMENTS
    // ========================================================================

    StmtHandle parse_statement(std::span<Attribute> attrs = {});
    StmtHandle parse_block_statement(std::span<Attribute> attrs = {});
    StmtHandle parse_return_statement();
    StmtHandle parse_if_statement(std::span<Attribute> attrs = {});
    StmtHandle parse_while_statement(std::span<Attribute> attrs = {});
    StmtHandle parse_for_statement(std::span<Attribute> attrs = {});
    StmtHandle parse_var_decl_statement(bool is_const = false, bool is_static = false, std::span<Attribute> attrs = {});
    StmtHandle parse_expression_statement();

    // ========================================================================
    // EXPRESSIONS
    // ========================================================================

    ExprHandle parse_expression();
    ExprHandle parse_sizeof_expr();
    ExprHandle parse_assignment();
    ExprHandle parse_logical_or();
    ExprHandle parse_logical_and();
    ExprHandle parse_bitwise_or();
    ExprHandle parse_bitwise_xor();
    ExprHandle parse_bitwise_and();
    ExprHandle parse_equality();
    ExprHandle parse_comparison();
    ExprHandle parse_shift();
    ExprHandle parse_addition();
    ExprHandle parse_multiplication();
    ExprHandle parse_unary();
    ExprHandle parse_postfix();
    ExprHandle parse_primary();

    // ========================================================================
    // TYPES
    // ========================================================================

    TypeHandle parse_type();
    TypeHandle parse_typeof_type();
    TypeHandle parse_type_postfix(TypeHandle base);
    TypeHandle parse_base_type();

    // ========================================================================
    // UTILITIES
    // ========================================================================

    bool is_type_keyword() const;
    bool is_primitive_type() const;
    std::vector<FunctionParameter> parse_parameter_list();
};

}  // namespace ibex
