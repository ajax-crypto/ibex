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
    bool is_used = false;
    bool allow_unused = false;
    
    // For namespaces/packages
    bool is_namespace = false;
    std::string namespace_target;
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
    const std::vector<std::string>& get_warnings() const { return warnings_; }

    void report_warning(const std::string& message) {
        warnings_.push_back(message);
    }
    
    void validate_attributes(std::span<Attribute> attrs);

    // TypeVisitor
    void visit(const PrimitiveType& type) override;
    void visit(const PointerType& type) override;
    void visit(const ReferenceType& type) override;
    void visit(const ArrayType& type) override;
    void visit(const SliceType& type) override;
    void visit(const NamedType& type) override;
    void visit(const TypeofType& type) override;
    void visit(const FunctionType& type) override;

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
    void visit(const SizeofExpr& expr) override;
    void visit(const BindingExpr& expr) override;
    void visit(const LambdaExpr& expr) override;

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

    void visit(const EnumDecl& decl) override;
    void visit(const FlagDecl& decl) override;
    void visit(const FunctionBindingDecl& decl) override;
    void visit(const AllocDecl& decl) override;
    void visit(const PackageDecl& decl) override;
    void visit(const ModuleDecl& decl) override;
    void visit(const ImportDecl& decl) override;
    void visit(const ExportPackagesDecl& decl) override;

    void visit(const TypeAliasDecl& decl) override;

private:
    void push_scope();
    void pop_scope();
    void add_symbol(Str name, TypeHandle type, DeclHandle decl_handle = DeclHandle{}, bool is_const = false, bool allow_unused = false);
    std::optional<Symbol> find_symbol(Str name);
    void report_error(const std::string& msg);

    // Helpers
    bool has_attribute(std::span<Attribute> attrs, std::string_view name) const;
    void check_deprecated(const Symbol& sym);
    void check_discard(ExprHandle expr_handle);
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
    std::vector<std::string> warnings_;

    // Global package registry: map package_name -> Scope
    std::unordered_map<std::string, Scope> packages_;
    
    // Global module registry: map module_name -> exported packages
    std::unordered_map<std::string, std::vector<std::string>> modules_;
    std::string current_package_;
    
    // Imports for the current file
    std::vector<ImportDecl> current_imports_;

    TypeHandle current_expr_type_; // For returning type from Expr visitor
};

} // namespace ibex
