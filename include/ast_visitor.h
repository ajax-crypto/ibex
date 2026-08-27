#pragma once

#include "ast_new.h"
#include <functional>

namespace ibex {

// ============================================================================
// AST VISITORS - Switch-based visitation for discriminated unions
// No virtual functions, minimal overhead
// ============================================================================

// Type visitor pattern - implement this to process types
class TypeVisitor {
public:
    virtual ~TypeVisitor() = default;

    virtual void visit(const PrimitiveType& type) = 0;
    virtual void visit(const PointerType& type) = 0;
    virtual void visit(const ReferenceType& type) = 0;
    virtual void visit(const ArrayType& type) = 0;
    virtual void visit(const SliceType& type) = 0;
    virtual void visit(const NamedType& type) = 0;
    virtual void visit(const TypeofType& type) = 0;
};

inline void visit_type(const Type& type, TypeVisitor* visitor) {
    std::visit([visitor](const auto& t) { visitor->visit(t); }, type);
}

// Expression visitor pattern
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;

    virtual void visit(const BinaryExpr& expr) = 0;
    virtual void visit(const UnaryExpr& expr) = 0;
    virtual void visit(const LiteralExpr& expr) = 0;
    virtual void visit(const IdentifierExpr& expr) = 0;
    virtual void visit(const CallExpr& expr) = 0;
    virtual void visit(const CastExpr& expr) = 0;
    virtual void visit(const MemberExpr& expr) = 0;
    virtual void visit(const TypeMemberExpr& expr) = 0;
    virtual void visit(const IndexExpr& expr) = 0;
    virtual void visit(const SliceExpr& expr) = 0;
    virtual void visit(const AddressOfExpr& expr) = 0;
    virtual void visit(const ArrayLiteralExpr& expr) = 0;
    virtual void visit(const StructInitExpr& expr) = 0;
    virtual void visit(const AllocExpr& expr) = 0;
    virtual void visit(const FreeExpr& expr) = 0;
    virtual void visit(const BlockExpr& expr) = 0;
    virtual void visit(const SizeofExpr& expr) = 0;
};

inline void visit_expr(const Expr& expr, ExprVisitor* visitor) {
    std::visit([visitor](const auto& e) { visitor->visit(e); }, expr);
}

// Statement visitor pattern
class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;

    virtual void visit(const BlockStmt& stmt) = 0;
    virtual void visit(const ReturnStmt& stmt) = 0;
    virtual void visit(const IfStmt& stmt) = 0;
    virtual void visit(const WhileStmt& stmt) = 0;
    virtual void visit(const ForStmt& stmt) = 0;
    virtual void visit(const BreakStmt& stmt) = 0;
    virtual void visit(const ContinueStmt& stmt) = 0;
    virtual void visit(const ExprStmt& stmt) = 0;
    virtual void visit(const VarDeclStmt& stmt) = 0;
    virtual void visit(const ConstBlockStmt& stmt) = 0;
    virtual void visit(const ConstModifierStmt& stmt) = 0;
};

inline void visit_stmt(const Stmt& stmt, StmtVisitor* visitor) {
    std::visit([visitor](const auto& s) { visitor->visit(s); }, stmt);
}

// Declaration visitor pattern
class DeclVisitor {
public:
    virtual ~DeclVisitor() = default;

    virtual void visit(const FunctionDecl& decl) = 0;
    virtual void visit(const VariableDecl& decl) = 0;
    virtual void visit(const StructDecl& decl) = 0;
    virtual void visit(const EnumDecl& decl) = 0;
    virtual void visit(const FlagDecl& decl) = 0;
    virtual void visit(const FunctionBindingDecl& decl) = 0;
    virtual void visit(const AllocDecl& decl) = 0;
    virtual void visit(const PackageDecl& decl) = 0;
    virtual void visit(const ModuleDecl& decl) = 0;
    virtual void visit(const ImportDecl& decl) = 0;
    virtual void visit(const ExportPackagesDecl& decl) = 0;
    virtual void visit(const TypeAliasDecl& decl) = 0;
};

inline void visit_decl(const Decl& decl, DeclVisitor* visitor) {
    std::visit([visitor](const auto& d) { visitor->visit(d); }, decl);
}

// ============================================================================
// MATCH FUNCTIONS - Alternative pattern matching style
// Can be used in place of visitors for simpler cases
// ============================================================================

template <typename T>
struct TypeMatch {
    std::function<void(const T&)> handler;
};

template <typename T>
inline void match_type(const Type& type, TypeMatch<T> match) {
    if (auto* ptr = std::get_if<T>(&type)) {
        match.handler(*ptr);
    }
}

template <typename T>
struct ExprMatch {
    std::function<void(const T&)> handler;
};

template <typename T>
inline void match_expr(const Expr& expr, ExprMatch<T> match) {
    if (auto* ptr = std::get_if<T>(&expr)) {
        match.handler(*ptr);
    }
}

template <typename T>
struct StmtMatch {
    std::function<void(const T&)> handler;
};

template <typename T>
inline void match_stmt(const Stmt& stmt, StmtMatch<T> match) {
    if (auto* ptr = std::get_if<T>(&stmt)) {
        match.handler(*ptr);
    }
}

template <typename T>
struct DeclMatch {
    std::function<void(const T&)> handler;
};

template <typename T>
inline void match_decl(const Decl& decl, DeclMatch<T> match) {
    if (auto* ptr = std::get_if<T>(&decl)) {
        match.handler(*ptr);
    }
}

}  // namespace ibex
