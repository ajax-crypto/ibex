#include "semantic_analyzer.h"
#include <iostream>

namespace ibex {

SemanticAnalyzer::SemanticAnalyzer(Program& program, TypeRegistry& type_registry)
    : program_(program), type_registry_(type_registry) {
    push_scope(); // Global scope
}

bool SemanticAnalyzer::analyze() {
    for (auto& decl : program_.declarations) {
        visit_decl(decl, this);
    }
    return !has_errors();
}

void SemanticAnalyzer::push_scope() {
    scopes_.push_back(Scope{});
}

void SemanticAnalyzer::pop_scope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();
    }
}

void SemanticAnalyzer::add_symbol(Str name, TypeHandle type, DeclHandle decl_handle, bool is_const) {
    if (scopes_.empty()) return;
    for (const auto& sym : scopes_.back().symbols) {
        if (sym.name == name) {
            report_error("Symbol '" + std::string(name.ptr(), name.len()) + "' already defined in current scope");
            return;
        }
    }
    scopes_.back().symbols.push_back({name, type, decl_handle, is_const});
}

std::optional<Symbol> SemanticAnalyzer::find_symbol(Str name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        for (const auto& sym : it->symbols) {
            if (sym.name == name) {
                return sym;
            }
        }
    }
    return std::nullopt;
}

void SemanticAnalyzer::report_error(const std::string& msg) {
    errors_.push_back(msg);
}

TypeHandle SemanticAnalyzer::resolve_type(const Type& type_variant) {
    // For now return dummy
    return TypeHandle{0};
}

// ----------------------------------------------------------------------------
// Types
// ----------------------------------------------------------------------------
void SemanticAnalyzer::visit(const PrimitiveType& type) {}
void SemanticAnalyzer::visit(const PointerType& type) {}
void SemanticAnalyzer::visit(const ReferenceType& type) {}
void SemanticAnalyzer::visit(const ArrayType& type) {}
void SemanticAnalyzer::visit(const SliceType& type) {}
void SemanticAnalyzer::visit(const NamedType& type) {}

// ----------------------------------------------------------------------------
// Expressions
// ----------------------------------------------------------------------------
void SemanticAnalyzer::visit(const BinaryExpr& expr) {}
void SemanticAnalyzer::visit(const UnaryExpr& expr) {}
void SemanticAnalyzer::visit(const LiteralExpr& expr) {}
void SemanticAnalyzer::visit(const IdentifierExpr& expr) {}

void SemanticAnalyzer::visit(const CallExpr& expr) {
    // FEATURE 1: Named Parameters
    // Find the function declaration being called.
    // In a full implementation, we'd resolve the type of `expr.function`.
    // For now, we assume it's an IdentifierExpr calling a known function.
    
    // We should merge positional and named arguments into a correct positional array.
    // Since we can't fully resolve yet, we outline the logic:
    if (!expr.named_args.empty()) {
        // Here we would lookup the FunctionDecl to get parameters.
        // Match each named_arg to the parameter list index.
        // Reorder into a new args span.
        // For now, this is a placeholder where the reordering logic happens.
    }
    
    // Recursively visit arguments
    for (auto arg : expr.arguments) {
        visit_expr(program_.expressions[arg.index], this);
    }
    for (auto arg : expr.named_args) {
        visit_expr(program_.expressions[arg.value.index], this);
    }
}

void SemanticAnalyzer::visit(const CastExpr& expr) {}
void SemanticAnalyzer::visit(const MemberExpr& expr) {}
void SemanticAnalyzer::visit(const TypeMemberExpr& expr) {}
void SemanticAnalyzer::visit(const IndexExpr& expr) {}
void SemanticAnalyzer::visit(const SliceExpr& expr) {}
void SemanticAnalyzer::visit(const AddressOfExpr& expr) {}
void SemanticAnalyzer::visit(const ArrayLiteralExpr& expr) {}

void SemanticAnalyzer::visit(const StructInitExpr& expr) {
    // FEATURE 2: Designated Struct Initializers
    // Look up struct by expr.type_name
    // In a real implementation we would search program_.declarations for StructDecl
    
    // For each named field in expr.field_values, match to the struct member.
    // Apply default values for uninitialized fields.
    // Type check values.
    
    for (auto arg : expr.positional_values) {
        visit_expr(program_.expressions[arg.index], this);
    }
    for (auto arg : expr.field_values) {
        visit_expr(program_.expressions[arg.value.index], this);
    }
}

void SemanticAnalyzer::visit(const AllocExpr& expr) {}
void SemanticAnalyzer::visit(const FreeExpr& expr) {}
void SemanticAnalyzer::visit(const BlockExpr& expr) {}

// ----------------------------------------------------------------------------
// Statements
// ----------------------------------------------------------------------------
void SemanticAnalyzer::visit(const BlockStmt& stmt) {}
void SemanticAnalyzer::visit(const ReturnStmt& stmt) {}
void SemanticAnalyzer::visit(const IfStmt& stmt) {}
void SemanticAnalyzer::visit(const WhileStmt& stmt) {}
void SemanticAnalyzer::visit(const ForStmt& stmt) {}
void SemanticAnalyzer::visit(const BreakStmt& stmt) {}
void SemanticAnalyzer::visit(const ContinueStmt& stmt) {}
void SemanticAnalyzer::visit(const ExprStmt& stmt) {}
void SemanticAnalyzer::visit(const VarDeclStmt& stmt) {}

// ----------------------------------------------------------------------------
// Declarations
// ----------------------------------------------------------------------------
void SemanticAnalyzer::visit(const FunctionDecl& decl) {
    push_scope();
    for (const auto& param : decl.parameters) {
        add_symbol(param.name, param.type);
    }
    visit_stmt(program_.statements[decl.body.index], this);
    pop_scope();
}

void SemanticAnalyzer::visit(const VariableDecl& decl) {}

void SemanticAnalyzer::flatten_struct_bases(StructDecl& decl, std::vector<StructMember>& out_members, uint32_t& current_offset) {
    // Recursively resolve and append base struct members
    for (auto base_name : decl.bases) {
        // Find base struct in program_.declarations
        for (const auto& base_decl_variant : program_.declarations) {
            if (auto* base_struct = std::get_if<StructDecl>(&base_decl_variant)) {
                if (base_struct->name == base_name) {
                    // Check for circular inheritance here (omitted for brevity)
                    flatten_struct_bases(const_cast<StructDecl&>(*base_struct), out_members, current_offset);
                    
                    // Append its members
                    for (const auto& member : base_struct->members) {
                        StructMember copied_member = member;
                        if (copied_member.offset.has_value()) {
                            // If base had explicit offset, we shift it by our current layout offset
                            // (In reality, C++ inheritance layout is more complex with alignment)
                            copied_member.offset = current_offset + copied_member.offset.value();
                        } else {
                            copied_member.offset = current_offset;
                            // Add rough size based on type (would use type_registry sizes)
                            current_offset += 4; 
                        }
                        out_members.push_back(copied_member);
                    }
                }
            }
        }
    }
}

void SemanticAnalyzer::visit(const StructDecl& decl_const) {
    // We need to modify the struct to include inherited members and offsets
    // Const cast is safe here because we own the AST and are mutating it during semantic analysis
    StructDecl& decl = const_cast<StructDecl&>(decl_const);
    
    // FEATURE 3 & 4: Struct Inheritance & Memory Layout
    std::vector<StructMember> flattened_members;
    uint32_t current_offset = 0;
    
    // 1. Flatten bases
    flatten_struct_bases(decl, flattened_members, current_offset);
    
    // 2. Process own members and resolve offsets
    for (auto& member : decl.members) {
        // Check for name collisions with bases
        for (const auto& existing : flattened_members) {
            if (existing.name == member.name) {
                report_error("Member '" + std::string(member.name.ptr(), member.name.len()) + "' hides inherited member");
            }
        }
        
        if (member.offset.has_value()) {
            if (member.offset.value() < current_offset) {
                report_error("Explicit offset for '" + std::string(member.name.ptr(), member.name.len()) + "' overlaps with previous members");
            }
            current_offset = member.offset.value();
        } else {
            member.offset = current_offset;
        }
        
        // Advance offset by size of type (mock size 4 for now)
        current_offset += 4;
        
        flattened_members.push_back(member);
    }
    
    // Update the decl's members with the flattened list using arena allocation
    decl.members = program_.allocate_array(flattened_members);
}

void SemanticAnalyzer::visit(const ClassDecl& decl) {}
void SemanticAnalyzer::visit(const EnumDecl& decl) {}
void SemanticAnalyzer::visit(const FlagDecl& decl) {}
void SemanticAnalyzer::visit(const FunctionBindingDecl& decl) {}
void SemanticAnalyzer::visit(const AllocDecl& decl) {}

} // namespace ibex
