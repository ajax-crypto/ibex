#include "semantic_analyzer.h"
#include <iostream>
#include <functional>

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
void SemanticAnalyzer::visit(const PointerType& type) { visit_type(program_.types[type.base.index], this); }
void SemanticAnalyzer::visit(const ReferenceType& type) { visit_type(program_.types[type.base.index], this); }
void SemanticAnalyzer::visit(const ArrayType& type) { visit_type(program_.types[type.element.index], this); }
void SemanticAnalyzer::visit(const SliceType& type) { visit_type(program_.types[type.element.index], this); }
void SemanticAnalyzer::visit(const NamedType& type) {}

// ----------------------------------------------------------------------------
// Expressions
// ----------------------------------------------------------------------------
void check_mutation(Program& program_, ExprHandle handle, const std::function<std::optional<Symbol>(Str)>& find_symbol, const std::function<void(const std::string&)>& report_error) {
    if (handle.is_null()) return;
    auto& expr = program_.expressions[handle.index];
    if (auto* id = std::get_if<IdentifierExpr>(&expr)) {
        auto sym = find_symbol(id->name);
        if (sym && sym->is_const) {
            report_error("Cannot mutate const variable: '" + std::string(id->name.ptr(), id->name.len()) + "'");
        }
    } else if (auto* mem = std::get_if<MemberExpr>(&expr)) {
        check_mutation(program_, mem->object, find_symbol, report_error);
    } else if (auto* idx = std::get_if<IndexExpr>(&expr)) {
        check_mutation(program_, idx->object, find_symbol, report_error);
    }
}

void SemanticAnalyzer::visit(const BinaryExpr& expr) {
    if (expr.op == TokenType::EQUAL || expr.op == TokenType::PLUS_EQUAL || expr.op == TokenType::MINUS_EQUAL || 
        expr.op == TokenType::STAR_EQUAL || expr.op == TokenType::SLASH_EQUAL) {
        check_mutation(program_, expr.left, 
            [this](Str name) { return find_symbol(name); },
            [this](const std::string& msg) { report_error(msg); });
    }
    
    visit_expr(program_.expressions[expr.left.index], this);
    visit_expr(program_.expressions[expr.right.index], this);
}
void SemanticAnalyzer::visit(const UnaryExpr& expr) {
    visit_expr(program_.expressions[expr.operand.index], this);
}
void SemanticAnalyzer::visit(const LiteralExpr& expr) {}
void SemanticAnalyzer::visit(const IdentifierExpr& expr) {
    if (!find_symbol(expr.name)) {
        report_error("Undefined symbol: '" + std::string(expr.name.ptr(), expr.name.len()) + "'");
    }
}

void SemanticAnalyzer::visit(const CallExpr& expr) {
    // Find the function declaration being called.
    const FunctionDecl* func_decl = nullptr;
    if (!expr.function.is_null()) {
        if (auto* id_expr = std::get_if<IdentifierExpr>(&program_.expressions[expr.function.index])) {
            for (const auto& decl_variant : program_.declarations) {
                if (auto* f = std::get_if<FunctionDecl>(&decl_variant)) {
                    if (f->name.len() == id_expr->name.len() && std::strncmp(f->name.ptr(), id_expr->name.ptr(), f->name.len()) == 0) {
                        func_decl = f;
                        break;
                    }
                }
            }
        }
    }

    if (func_decl) {
        if (expr.arguments.size() + expr.named_args.size() > func_decl->parameters.size()) {
            report_error("Too many arguments for function '" + std::string(func_decl->name.ptr(), func_decl->name.len()) + "'");
        }
        
        for (const auto& named_arg : expr.named_args) {
            bool found = false;
            for (const auto& param : func_decl->parameters) {
                if (param.name.len() == named_arg.name.len() && std::strncmp(param.name.ptr(), named_arg.name.ptr(), param.name.len()) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                report_error("No such parameter '" + std::string(named_arg.name.ptr(), named_arg.name.len()) + "' in function");
            }
        }
    }
    
    if (!expr.function.is_null()) visit_expr(program_.expressions[expr.function.index], this);
    for (auto arg : expr.arguments) visit_expr(program_.expressions[arg.index], this);
    for (auto arg : expr.named_args) visit_expr(program_.expressions[arg.value.index], this);
}

void SemanticAnalyzer::visit(const CastExpr& expr) {
    visit_type(program_.types[expr.target_type.index], this);
    visit_expr(program_.expressions[expr.operand.index], this);
}
void SemanticAnalyzer::visit(const MemberExpr& expr) {
    visit_expr(program_.expressions[expr.object.index], this);
}
void SemanticAnalyzer::visit(const TypeMemberExpr& expr) {}
void SemanticAnalyzer::visit(const IndexExpr& expr) {
    visit_expr(program_.expressions[expr.object.index], this);
    visit_expr(program_.expressions[expr.index.index], this);
}
void SemanticAnalyzer::visit(const SliceExpr& expr) {
    visit_expr(program_.expressions[expr.object.index], this);
    if (!expr.start.is_null()) visit_expr(program_.expressions[expr.start.index], this);
    if (!expr.end.is_null()) visit_expr(program_.expressions[expr.end.index], this);
}
void SemanticAnalyzer::visit(const AddressOfExpr& expr) {
    visit_expr(program_.expressions[expr.operand.index], this);
}
void SemanticAnalyzer::visit(const ArrayLiteralExpr& expr) {
    for (auto el : expr.elements) visit_expr(program_.expressions[el.index], this);
}

void SemanticAnalyzer::visit(const StructInitExpr& expr) {
    const StructDecl* struct_decl = nullptr;
    for (const auto& decl_variant : program_.declarations) {
        if (auto* s = std::get_if<StructDecl>(&decl_variant)) {
            if (s->name.len() == expr.type_name.len() && std::strncmp(s->name.ptr(), expr.type_name.ptr(), s->name.len()) == 0) {
                struct_decl = s;
                break;
            }
        }
    }

    if (struct_decl) {
        std::vector<StructMember> all_members;
        uint32_t offset = 0;
        flatten_struct_bases(*const_cast<StructDecl*>(struct_decl), all_members, offset);
        for (const auto& named_arg : expr.field_values) {
            bool found = false;
            for (const auto& mem : all_members) {
                if (mem.name.len() == named_arg.name.len() && std::strncmp(mem.name.ptr(), named_arg.name.ptr(), mem.name.len()) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) report_error("No such field '" + std::string(named_arg.name.ptr(), named_arg.name.len()) + "' in struct");
        }
    } else {
        report_error("Unknown struct type '" + std::string(expr.type_name.ptr(), expr.type_name.len()) + "'");
    }
    
    for (auto arg : expr.positional_values) visit_expr(program_.expressions[arg.index], this);
    for (auto arg : expr.field_values) visit_expr(program_.expressions[arg.value.index], this);
}

void SemanticAnalyzer::visit(const AllocExpr& expr) {
    visit_type(program_.types[expr.element_type.index], this);
    if (expr.size) visit_expr(program_.expressions[expr.size.value().index], this);
}
void SemanticAnalyzer::visit(const FreeExpr& expr) {
    visit_expr(program_.expressions[expr.pointer.index], this);
}
void SemanticAnalyzer::visit(const BlockExpr& expr) {
    push_scope();
    for (auto stmt : expr.statements) visit_stmt(program_.statements[stmt.index], this);
    if (expr.value) visit_expr(program_.expressions[expr.value.value().index], this);
    pop_scope();
}

// ----------------------------------------------------------------------------
// Statements
// ----------------------------------------------------------------------------
void SemanticAnalyzer::visit(const BlockStmt& stmt) {
    push_scope();
    for (auto s : stmt.statements) visit_stmt(program_.statements[s.index], this);
    pop_scope();
}
void SemanticAnalyzer::visit(const ReturnStmt& stmt) {
    if (stmt.value) visit_expr(program_.expressions[stmt.value.value().index], this);
}
void SemanticAnalyzer::visit(const IfStmt& stmt) {
    visit_expr(program_.expressions[stmt.condition.index], this);
    visit_stmt(program_.statements[stmt.then_branch.index], this);
    if (stmt.else_branch) visit_stmt(program_.statements[stmt.else_branch.value().index], this);
}
void SemanticAnalyzer::visit(const WhileStmt& stmt) {
    visit_expr(program_.expressions[stmt.condition.index], this);
    visit_stmt(program_.statements[stmt.body.index], this);
    if (stmt.else_branch) visit_stmt(program_.statements[stmt.else_branch.value().index], this);
}
void SemanticAnalyzer::visit(const ForStmt& stmt) {
    visit_expr(program_.expressions[stmt.range.index], this);
    push_scope();
    add_symbol(stmt.variable, TypeHandle{0}, DeclHandle{}, true); // Mock loop variable type for now
    visit_stmt(program_.statements[stmt.body.index], this);
    pop_scope();
    if (stmt.else_branch) visit_stmt(program_.statements[stmt.else_branch.value().index], this);
}
void SemanticAnalyzer::visit(const BreakStmt& stmt) {}
void SemanticAnalyzer::visit(const ContinueStmt& stmt) {}
void SemanticAnalyzer::visit(const ExprStmt& stmt) {
    visit_expr(program_.expressions[stmt.expression.index], this);
}
void SemanticAnalyzer::visit(const VarDeclStmt& stmt) {
    if (stmt.type) visit_type(program_.types[stmt.type.value().index], this);
    if (stmt.initializer) visit_expr(program_.expressions[stmt.initializer.value().index], this);
    add_symbol(stmt.name, stmt.type.value_or(TypeHandle{0}), DeclHandle{}, stmt.is_const);
}
void SemanticAnalyzer::visit(const ConstBlockStmt& stmt) {
    push_scope();
    for (const auto& var : stmt.variables) {
        auto sym = find_symbol(var);
        if (sym) {
            scopes_.back().symbols.push_back({var, sym->type, sym->decl_handle, true});
        } else {
            report_error("Undefined symbol in const block: '" + std::string(var.ptr(), var.len()) + "'");
        }
    }
    visit_stmt(program_.statements[stmt.body.index], this);
    pop_scope();
}
void SemanticAnalyzer::visit(const ConstModifierStmt& stmt) {
    for (const auto& var : stmt.variables) {
        auto sym = find_symbol(var);
        if (sym) {
            bool in_current = false;
            for (auto& s : scopes_.back().symbols) {
                if (s.name == var) {
                    s.is_const = true;
                    in_current = true;
                    break;
                }
            }
            if (!in_current) {
                scopes_.back().symbols.push_back({var, sym->type, sym->decl_handle, true});
            }
        } else {
            report_error("Undefined symbol in const modifier: '" + std::string(var.ptr(), var.len()) + "'");
        }
    }
}

// ----------------------------------------------------------------------------
// Declarations
// ----------------------------------------------------------------------------
void SemanticAnalyzer::visit(const FunctionDecl& decl) {
    push_scope();
    for (const auto& param : decl.parameters) {
        add_symbol(param.name, param.type, DeclHandle{}, param.is_const);
    }
    visit_stmt(program_.statements[decl.body.index], this);
    pop_scope();
}

void SemanticAnalyzer::visit(const VariableDecl& decl) {
    if (decl.type) visit_type(program_.types[decl.type.value().index], this);
    if (decl.initializer) visit_expr(program_.expressions[decl.initializer.value().index], this);
    add_symbol(decl.name, decl.type.value_or(TypeHandle{0}), DeclHandle{}, decl.is_const);
}

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
    StructDecl& decl = const_cast<StructDecl&>(decl_const);
    
    std::vector<StructMember> flattened_members;
    uint32_t current_offset = 0;
    
    // Process bases
    flatten_struct_bases(decl, flattened_members, current_offset);
    
    // Append own members and compute offsets
    for (auto& member : decl.members) {
        if (member.offset.has_value()) {
            if (member.offset.value() < current_offset) {
                report_error("Overlap or invalid explicit offset for member '" + std::string(member.name.ptr(), member.name.len()) + "'");
            }
            current_offset = member.offset.value();
        } else {
            member.offset = current_offset;
        }
        
        // Ensure no name collisions
        for (const auto& base_mem : flattened_members) {
            if (base_mem.name.len() == member.name.len() && std::strncmp(base_mem.name.ptr(), member.name.ptr(), base_mem.name.len()) == 0) {
                report_error("Member name collision with base struct: '" + std::string(member.name.ptr(), member.name.len()) + "'");
            }
        }
        
        flattened_members.push_back(member);
        current_offset += 4; // Estimate size based on type registry (we'll just add 4 for now)
    }
    
    ibex::TypeDefinition type_def;
    type_def.name = type_registry_.alloc_str(std::string_view(decl.name.ptr(), decl.name.len()));
    type_def.kind = ibex::TypeDefinition::Kind::STRUCT;
    for (const auto& m : flattened_members) {
        ibex::TypeMember tm;
        tm.name = type_registry_.alloc_str(std::string_view(m.name.ptr(), m.name.len()));
        tm.type_name = type_registry_.alloc_str("unknown");
        tm.offset = m.offset.value();
        type_def.members.push_back(tm);
    }
    for (const auto& b : decl.bases) {
        type_def.bases.push_back(type_registry_.alloc_str(std::string_view(b.ptr(), b.len())));
    }
    type_registry_.register_type(type_def);

    decl.members = program_.allocate_array(flattened_members);
    
    add_symbol(decl.name, TypeHandle{0}); // Register struct type name
}

void SemanticAnalyzer::visit(const ClassDecl& decl) {
    add_symbol(decl.name, TypeHandle{0});
    push_scope();
    // Class members not implemented deeply yet
    pop_scope();
}
void SemanticAnalyzer::flatten_enum_bases(EnumDecl& decl, std::vector<EnumMember>& out_members) {
    if (!decl.extends) return;
    
    // Find base enum in program_.declarations
    for (const auto& base_decl_variant : program_.declarations) {
        if (auto* base_enum = std::get_if<EnumDecl>(&base_decl_variant)) {
            if (base_enum->name.len() == decl.extends.value().len() && std::strncmp(base_enum->name.ptr(), decl.extends.value().ptr(), base_enum->name.len()) == 0) {
                // Recursively flatten base
                flatten_enum_bases(const_cast<EnumDecl&>(*base_enum), out_members);
                
                // Append its members
                for (const auto& member : base_enum->members) {
                    out_members.push_back(member);
                }
                break;
            }
        }
    }
}

void SemanticAnalyzer::visit(const EnumDecl& decl_const) {
    EnumDecl& decl = const_cast<EnumDecl&>(decl_const);

    std::vector<EnumMember> flattened_members;
    flatten_enum_bases(decl, flattened_members);

    for (auto& member : decl.members) {
        // Ensure no name collisions
        for (const auto& base_mem : flattened_members) {
            if (base_mem.name.len() == member.name.len() && std::strncmp(base_mem.name.ptr(), member.name.ptr(), base_mem.name.len()) == 0) {
                report_error("Enum member name collision with base enum: '" + std::string(member.name.ptr(), member.name.len()) + "'");
            }
        }
        flattened_members.push_back(member);
    }
    
    decl.members = program_.allocate_array(flattened_members);

    add_symbol(decl.name, TypeHandle{0});
    visit_type(program_.types[decl.base_type.index], this);
    for (auto& member : decl.members) {
        if (member.value) visit_expr(program_.expressions[member.value.value().index], this);
    }
}
void SemanticAnalyzer::flatten_flag_bases(FlagDecl& decl, std::vector<EnumMember>& out_members) {
    if (!decl.extends) return;
    for (const auto& base_decl_variant : program_.declarations) {
        if (auto* base_flag = std::get_if<FlagDecl>(&base_decl_variant)) {
            if (base_flag->name.len() == decl.extends.value().len() && std::strncmp(base_flag->name.ptr(), decl.extends.value().ptr(), base_flag->name.len()) == 0) {
                flatten_flag_bases(const_cast<FlagDecl&>(*base_flag), out_members);
                for (const auto& member : base_flag->members) {
                    out_members.push_back(member);
                }
                break;
            }
        }
    }
}

void SemanticAnalyzer::visit(const FlagDecl& decl_const) {
    FlagDecl& decl = const_cast<FlagDecl&>(decl_const);
    add_symbol(decl.name, TypeHandle{0});
    
    std::vector<EnumMember> flattened_members;
    flatten_flag_bases(decl, flattened_members);

    int64_t current_value = 1;
    int64_t max_val = 0;
    push_scope();
    for (auto& base_mem : flattened_members) {
        add_symbol(base_mem.name, TypeHandle{0});
        if (base_mem.value) {
             if (auto* lit = std::get_if<LiteralExpr>(&program_.expressions[base_mem.value.value().index])) {
                 if (lit->kind == LiteralExpr::Kind::INTEGER) {
                     if (lit->value.int_value > max_val) max_val = lit->value.int_value;
                 }
             }
        }
    }
    if (max_val > 0) current_value = max_val << 1;

    for (auto& member : decl.members) {
        for (const auto& base_mem : flattened_members) {
            if (base_mem.name.len() == member.name.len() && std::strncmp(base_mem.name.ptr(), member.name.ptr(), base_mem.name.len()) == 0) {
                report_error("Flag member name collision with base flag: '" + std::string(member.name.ptr(), member.name.len()) + "'");
            }
        }

        if (!member.value) {
            LiteralExpr lit{LiteralExpr::Kind::INTEGER};
            lit.value.int_value = current_value;
            
            ExprHandle handle;
            handle.index = static_cast<uint32_t>(program_.expressions.size());
            program_.expressions.push_back(lit);
            
            member.value = handle;
            current_value <<= 1;
        } else {
            if (auto* lit = std::get_if<LiteralExpr>(&program_.expressions[member.value.value().index])) {
                if (lit->kind == LiteralExpr::Kind::INTEGER) {
                    if (lit->value.int_value < 0) {
                        report_error("Negative values are not supported in flags");
                    } else {
                        current_value = lit->value.int_value << 1;
                    }
                }
            }
            visit_expr(program_.expressions[member.value.value().index], this);
        }
        add_symbol(member.name, TypeHandle{0});
        flattened_members.push_back(member);
    }
    pop_scope();
    
    decl.members = program_.allocate_array(flattened_members);

    uint64_t final_max_val = 0;
    for (auto& m : decl.members) {
        if (m.value) {
             if (auto* lit = std::get_if<LiteralExpr>(&program_.expressions[m.value.value().index])) {
                 if (lit->kind == LiteralExpr::Kind::INTEGER) {
                     if (lit->value.int_value > (int64_t)final_max_val) final_max_val = lit->value.int_value;
                 }
             }
        }
    }

    if (decl.base_type.is_null()) {
        TokenType target = TokenType::U16;
        if (final_max_val > 0xFFFFFFFF) target = TokenType::U64;
        else if (final_max_val > 0xFFFF) target = TokenType::U32;

        PrimitiveType pt;
        pt.primitive = target;
        TypeHandle th;
        th.index = static_cast<uint32_t>(program_.types.size());
        program_.types.push_back(pt);
        decl.base_type = th;
    }

    if (!decl.base_type.is_null()) {
        visit_type(program_.types[decl.base_type.index], this);
    }
}
void SemanticAnalyzer::visit(const FunctionBindingDecl& decl) {
    for (auto& arg : decl.bound_args) {
        if (arg) visit_expr(program_.expressions[arg.value().index], this);
    }
    add_symbol(decl.name, TypeHandle{0});
}
void SemanticAnalyzer::visit(const AllocDecl& decl) {
    visit_type(program_.types[decl.element_type.index], this);
    if (decl.size) visit_expr(program_.expressions[decl.size.value().index], this);
    add_symbol(decl.name, TypeHandle{0});
}

} // namespace ibex
