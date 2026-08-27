#pragma once

#include "ast_new.h"
#include "ast_visitor.h"
#include "type_registry.h"
#include <vector>
#include <string_view>
#include <span>
#include <optional>
#include <string>

namespace ibex {

struct Symbol {
    Str name;
    TypeHandle type;
    DeclHandle decl_handle; // Optional reference to where it was declared
    bool is_const;
};

struct Scope {
    std::vector<Symbol> symbols;
};

class SemanticAnalyzer : public TypeVisitor, public ExprVisitor, public StmtVisitor, public DeclVisitor {
public:
    SemanticAnalyzer(Program& program, TypeRegistry& type_registry);

    bool analyze();

    // Expose error checking
    bool has_errors() const { return !errors_.empty(); }
    const std::vector<std::string>& get_errors() const { return errors_; }

    // TypeVisitor
    void visit(const PrimitiveType& type) override;
    void visit(const PointerType& type) override;
    void visit(const ReferenceType& type) override;
    void visit(const ArrayType& type) override;
    void visit(const SliceType& type) override;
    void visit(const NamedType& type) override;

    // ExprVisitor
    void visit(const BinaryExpr& expr) override;
    void visit(const UnaryExpr& expr) override;
    void visit(const LiteralExpr& expr) override;
    void visit(const IdentifierExpr& expr) override;
    void visit(const CallExpr& expr) override;
    void visit(const CastExpr& expr) override;
    void visit(const MemberExpr& expr) override;
    void visit(const TypeMemberExpr& expr) override;
    void visit(const IndexExpr& expr) override;
    void visit(const SliceExpr& expr) override;
    void visit(const AddressOfExpr& expr) override;
    void visit(const ArrayLiteralExpr& expr) override;
    void visit(const StructInitExpr& expr) override;
    void visit(const AllocExpr& expr) override;
    void visit(const FreeExpr& expr) override;
    void visit(const BlockExpr& expr) override;

    // StmtVisitor
    void visit(const BlockStmt& stmt) override;
    void visit(const ReturnStmt& stmt) override;
    void visit(const IfStmt& stmt) override;
    void visit(const WhileStmt& stmt) override;
    void visit(const ForStmt& stmt) override;
    void visit(const BreakStmt& stmt) override;
    void visit(const ContinueStmt& stmt) override;
    void visit(const ExprStmt& stmt) override;
    void visit(const VarDeclStmt& stmt) override;
    void visit(const ConstBlockStmt& stmt) override;
    void visit(const ConstModifierStmt& stmt) override;

    // DeclVisitor
    void visit(const FunctionDecl& decl) override;
    void visit(const VariableDecl& decl) override;
    void visit(const StructDecl& decl) override;
    void visit(const ClassDecl& decl) override;
    void visit(const EnumDecl& decl) override;
    void visit(const FlagDecl& decl) override;
    void visit(const FunctionBindingDecl& decl) override;
    void visit(const AllocDecl& decl) override;

private:
    void push_scope();
    void pop_scope();
    void add_symbol(Str name, TypeHandle type, DeclHandle decl_handle = DeclHandle{}, bool is_const = false);
    std::optional<Symbol> find_symbol(Str name) const;
    void report_error(const std::string& msg);

    // Helpers
    TypeHandle resolve_type(const Type& type_variant);
    
    // Struct inheritance helper
    void flatten_struct_bases(StructDecl& decl, std::vector<StructMember>& out_members, uint32_t& current_offset);
    
    // Enum inheritance helper
    void flatten_enum_bases(EnumDecl& decl, std::vector<EnumMember>& out_members);

    // Flag inheritance helper
    void flatten_flag_bases(FlagDecl& decl, std::vector<EnumMember>& out_members);

    Program& program_;
    TypeRegistry& type_registry_;
    std::vector<Scope> scopes_;
    std::vector<std::string> errors_;

    TypeHandle current_expr_type_; // For returning type from Expr visitor
};

} // namespace ibex
