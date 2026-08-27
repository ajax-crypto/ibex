#include "semantic_analyzer.h"
#include <iostream>
#include <functional>

namespace ibex {

SemanticAnalyzer::SemanticAnalyzer(Program& program, TypeRegistry& type_registry)
    : program_(program), type_registry_(type_registry) {
    push_scope(); // Global scope
}

bool SemanticAnalyzer::analyze() {
    // Pre-pass: register all top-level declarations (functions, structs) so forward references work
    for (size_t i = 0; i < program_.declarations.size(); ++i) {
        auto& decl = program_.declarations[i];
        DeclHandle handle{static_cast<uint32_t>(i)};
        if (auto* func = std::get_if<FunctionDecl>(&decl)) {
            bool found = false;
            for (const auto& sym : scopes_.front().symbols) {
                if (sym.name == func->name) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                Symbol sym{func->name, TypeHandle{0}, handle, true, false, false};
                scopes_.front().symbols.push_back(sym);
            }
        } else if (auto* s = std::get_if<StructDecl>(&decl)) {
            bool found = false;
            for (const auto& sym : scopes_.front().symbols) {
                if (sym.name == s->name) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                Symbol sym{s->name, TypeHandle{0}, handle, true, false, false};
                scopes_.front().symbols.push_back(sym);
            }
        }
    }

    // Pass 1.5: Build Packages and Modules
    for (size_t i = 0; i < program_.declarations.size(); ++i) {
        auto& decl = program_.declarations[i];
        if (auto* pkg = std::get_if<PackageDecl>(&decl)) {
            std::string pkg_name = std::string(pkg->name.ptr(), pkg->name.len());
            if (packages_.find(pkg_name) == packages_.end()) {
                packages_[pkg_name] = Scope{};
            }
        } else if (auto* mod = std::get_if<ModuleDecl>(&decl)) {
            std::string mod_name = std::string(mod->name.ptr(), mod->name.len());
            modules_[mod_name] = {};
            for (auto inner_handle : mod->declarations) {
                if (inner_handle.is_null()) continue;
                auto& inner_decl = program_.declarations[inner_handle.index];
                if (auto* export_pkgs = std::get_if<ExportPackagesDecl>(&inner_decl)) {
                    for (Str p_name : export_pkgs->package_names) {
                        modules_[mod_name].push_back(std::string(p_name.ptr(), p_name.len()));
                    }
                }
            }
        }
    }

    // Main pass: visit all top-level declarations
    for (auto decl_handle : program_.top_level_declarations) {
        if (!decl_handle.is_null()) {
            visit_decl(program_.declarations[decl_handle.index], this);
        }
    }
    return !has_errors();
}

void SemanticAnalyzer::push_scope() {
    scopes_.push_back(Scope{});
}

void SemanticAnalyzer::pop_scope() {
    if (!scopes_.empty()) {
        for (const auto& sym : scopes_.back().symbols) {
            if (!sym.is_used && !sym.allow_unused && !sym.is_namespace) {
                // If it is a global scope (maybe scope.size() == 1?), wait, do we warn for unused globals?
                // For now, let's just warn for everything unused.
                report_warning("Unused variable: '" + std::string(sym.name.ptr(), sym.name.len()) + "'");
            }
        }
        scopes_.pop_back();
    }
}

void SemanticAnalyzer::add_symbol(Str name, TypeHandle type, DeclHandle decl_handle, bool is_const, bool allow_unused) {
    std::cout << "add_symbol: " << std::string(name.ptr(), name.len()) << "\n";
    if (scopes_.empty()) return;
    for (const auto& sym : scopes_.back().symbols) {
        if (sym.name == name) {
            report_error("Symbol '" + std::string(name.ptr(), name.len()) + "' already defined in current scope");
            return;
        }
    }
    scopes_.back().symbols.push_back({
        .name = name, 
        .type = type, 
        .decl_handle = decl_handle, 
        .is_const = is_const, 
        .is_used = false, 
        .allow_unused = allow_unused
    });
}

std::optional<Symbol> SemanticAnalyzer::find_symbol(Str name) {
    std::cout << "find_symbol: " << std::string(name.ptr(), name.len()) << "\n";
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        for (auto& sym : it->symbols) {
            if (sym.name == name) {
                sym.is_used = true;
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
    return TypeHandle{0};
}

void SemanticAnalyzer::validate_attributes(std::span<Attribute> attrs) {
    for (const auto& attr : attrs) {
        std::string_view name(attr.name.ptr(), attr.name.len());
        if (name == "strong" || name == "unused" || name == "deprecated" || 
            name == "platform" || name == "discard" || name == "offset" || name == "nodiscard") {
            
            if (name == "deprecated" && attr.args.size() != 1) {
                report_warning("Attribute 'deprecated' expects exactly 1 argument");
            }
            if (name == "platform" && attr.args.empty()) {
                report_error("Attribute 'platform' expects at least 1 argument");
            }
            if (name == "offset" && attr.args.size() != 1) {
                report_error("Attribute 'offset' expects exactly 1 argument");
            }
        } else {
            report_warning("Unknown attribute: '" + std::string(name) + "'");
        }
    }
}

bool SemanticAnalyzer::has_attribute(std::span<Attribute> attrs, std::string_view name) const {
    for (const auto& attr : attrs) {
        if (std::string_view(attr.name.ptr(), attr.name.len()) == name) {
            return true;
        }
    }
    return false;
}

void SemanticAnalyzer::check_deprecated(const Symbol& sym) {
    if (sym.decl_handle.is_null()) return;
    auto& decl = program_.declarations[sym.decl_handle.index];
    auto attrs = std::visit([](const auto& node) -> std::span<Attribute> {
        if constexpr (requires { node.attributes; }) {
            return node.attributes;
        } else {
            return {};
        }
    }, decl);
    
    for (const auto& attr : attrs) {
        if (std::string_view(attr.name.ptr(), attr.name.len()) == "deprecated") {
            std::string reason = "";
            if (attr.args.size() == 1) {
                if (auto* lit = std::get_if<LiteralExpr>(&program_.expressions[attr.args[0].index])) {
                    if (lit->kind == LiteralExpr::Kind::STRING) {
                        reason = std::string(lit->value.string_value.value.ptr(), lit->value.string_value.value.len());
                    }
                }
            }
            std::string msg = "Call to deprecated function '" + std::string(sym.name.ptr(), sym.name.len()) + "'";
            if (!reason.empty()) {
                msg = "Call to deprecated function '" + std::string(sym.name.ptr(), sym.name.len()) + "' (Reason: " + reason + ")";
            }
            report_warning(msg);
        }
    }
}

void SemanticAnalyzer::check_discard(ExprHandle expr_handle) {
    auto& expr = program_.expressions[expr_handle.index];
    if (auto* call = std::get_if<CallExpr>(&expr)) {
        if (auto* ident = std::get_if<IdentifierExpr>(&program_.expressions[call->function.index])) {
            if (auto sym_opt = find_symbol(ident->name)) {
                auto& sym = *sym_opt;
                if (!sym.decl_handle.is_null()) {
                    auto& decl = program_.declarations[sym.decl_handle.index];
                    if (auto* func = std::get_if<FunctionDecl>(&decl)) {
                        bool func_discard = has_attribute(func->attributes, "discard");
                        
                        bool type_nodiscard = false;
                        if (!func->return_type.is_null()) {
                            auto& ret_type = program_.types[func->return_type.index];
                            if (auto* named = std::get_if<NamedType>(&ret_type)) {
                                if (auto t_sym_opt = find_symbol(named->name)) {
                                    auto& t_sym = *t_sym_opt;
                                    if (!t_sym.decl_handle.is_null()) {
                                        auto& t_decl = program_.declarations[t_sym.decl_handle.index];
                                        auto t_attrs = std::visit([](const auto& node) -> std::span<Attribute> {
                                            if constexpr (requires { node.attributes; }) return node.attributes;
                                            else return {};
                                        }, t_decl);
                                        type_nodiscard = has_attribute(t_attrs, "nodiscard");
                                    }
                                }
                            }
                        }
                        
                        if (type_nodiscard) {
                            report_error("Discarding return value of function '" + std::string(ident->name.ptr(), ident->name.len()) + "' which returns a [[nodiscard]] type");
                        } else if (!func_discard) {
                            bool is_void = false;
                            if (!func->return_type.is_null()) {
                                auto& ret_type = program_.types[func->return_type.index];
                                if (auto* named = std::get_if<NamedType>(&ret_type)) {
                                    if (std::string_view(named->name.ptr(), named->name.len()) == "void") {
                                        is_void = true;
                                    }
                                }
                            } else {
                                is_void = true; // no return type implies void
                            }
                            
                            if (!is_void) {
                                report_warning("Discarding return value of function '" + std::string(ident->name.ptr(), ident->name.len()) + "' (missing [[discard]] attribute)");
                            }
                        }
                    }
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Types
// ----------------------------------------------------------------------------
void SemanticAnalyzer::visit(const PrimitiveType& type) {}
void SemanticAnalyzer::visit(const PointerType& type) {
    if (!type.base.is_null()) visit_type(program_.types[type.base.index], this);
}
void SemanticAnalyzer::visit(const ReferenceType& type) {
    if (!type.base.is_null()) visit_type(program_.types[type.base.index], this);
}
void SemanticAnalyzer::visit(const ArrayType& type) {
    if (!type.element.is_null()) visit_type(program_.types[type.element.index], this);
}
void SemanticAnalyzer::visit(const SliceType& type) {
    if (!type.element.is_null()) visit_type(program_.types[type.element.index], this);
}
void SemanticAnalyzer::visit(const NamedType& type) {}

void SemanticAnalyzer::visit(const TypeofType& type) {
    if (!type.expr.is_null()) visit_expr(program_.expressions[type.expr.index], this);
}

void SemanticAnalyzer::visit(const FunctionType& type) {
    for (auto handle : type.param_types) {
        if (!handle.is_null()) visit_type(program_.types[handle.index], this);
    }
    if (!type.return_type.is_null()) {
        visit_type(program_.types[type.return_type.index], this);
    }
}

// ----------------------------------------------------------------------------
// Helper: check mutation target
// ----------------------------------------------------------------------------
static void check_mutation(Program& program_, ExprHandle handle,
    const std::function<std::optional<Symbol>(Str)>& find_symbol,
    const std::function<void(const std::string&)>& report_error) {
    if (handle.is_null()) return;
    if (handle.index >= program_.expressions.size()) return;
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

// ----------------------------------------------------------------------------
// Expressions
// ----------------------------------------------------------------------------
void SemanticAnalyzer::visit(const BinaryExpr& expr) {
    if (expr.op == TokenType::EQUAL || expr.op == TokenType::PLUS_EQUAL || expr.op == TokenType::MINUS_EQUAL ||
        expr.op == TokenType::STAR_EQUAL || expr.op == TokenType::SLASH_EQUAL) {
        check_mutation(program_, expr.left,
            [this](Str name) { return find_symbol(name); },
            [this](const std::string& msg) { report_error(msg); });
    }

    if (!expr.left.is_null()) visit_expr(program_.expressions[expr.left.index], this);
    if (!expr.right.is_null()) visit_expr(program_.expressions[expr.right.index], this);
}

void SemanticAnalyzer::visit(const UnaryExpr& expr) {
    if (!expr.operand.is_null()) visit_expr(program_.expressions[expr.operand.index], this);
}

void SemanticAnalyzer::visit(const LiteralExpr& expr) {}

void SemanticAnalyzer::visit(const IdentifierExpr& expr) {
    if (auto sym = find_symbol(expr.name)) {
        check_deprecated(*sym);
    } else {
        report_error("Undefined symbol: '" + std::string(expr.name.ptr(), expr.name.len()) + "'");
    }
}

void SemanticAnalyzer::visit(const CallExpr& expr) {
    // Find the function declaration being called
    const FunctionDecl* func_decl = nullptr;
    if (!expr.function.is_null()) {
        if (auto* id_expr = std::get_if<IdentifierExpr>(&program_.expressions[expr.function.index])) {
            for (const auto& decl_variant : program_.declarations) {
                if (auto* f = std::get_if<FunctionDecl>(&decl_variant)) {
                    if (f->name == id_expr->name) {
                        func_decl = f;
                        break;
                    }
                }
            }
        }
    }

    if (func_decl) {
        // Check argument count
        if (expr.arguments.size() + expr.named_args.size() > func_decl->parameters.size()) {
            report_error("Too many arguments for function '" + std::string(func_decl->name.ptr(), func_decl->name.len()) + "'");
        }

        // Check named argument names
        for (const auto& named_arg : expr.named_args) {
            bool found = false;
            for (const auto& param : func_decl->parameters) {
                if (param.name == named_arg.name) {
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
    for (auto arg : expr.arguments) {
        if (!arg.is_null()) visit_expr(program_.expressions[arg.index], this);
    }
    for (auto arg : expr.named_args) {
        if (!arg.value.is_null()) visit_expr(program_.expressions[arg.value.index], this);
    }
}

void SemanticAnalyzer::visit(const CastExpr& expr) {
    if (!expr.target_type.is_null()) visit_type(program_.types[expr.target_type.index], this);
    if (!expr.operand.is_null()) visit_expr(program_.expressions[expr.operand.index], this);
}

void SemanticAnalyzer::visit(const MemberExpr& expr) {
    if (!expr.object.is_null()) {
        auto& obj_expr = program_.expressions[expr.object.index];
        
        // Custom logic to handle module/package namespaces
        if (auto* id_expr = std::get_if<IdentifierExpr>(&obj_expr)) {
            auto sym = find_symbol(id_expr->name);
            if (sym && sym->is_namespace) {
                std::string target = sym->namespace_target;
                if (target.ends_with(".*")) {
                    // This is a module wildcard import: e.g. 'mod'
                    std::string mod_name = target.substr(0, target.size() - 2);
                    std::string member_name = std::string(expr.member.ptr(), expr.member.len());
                    // Check if member is a package exported by this module
                    const auto& exports = modules_[mod_name];
                    if (std::find(exports.begin(), exports.end(), member_name) == exports.end()) {
                        report_error("Module '" + mod_name + "' does not export package '" + member_name + "'");
                    }
                    // We don't visit the object since it's a namespace
                    return;
                } else {
                    // This is a package alias: e.g. 'm' -> 'math'
                    auto pkg_it = packages_.find(target);
                    if (pkg_it != packages_.end()) {
                        bool found = false;
                        for (const auto& pkg_sym : pkg_it->second.symbols) {
                            if (pkg_sym.name == expr.member) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            report_error("Undefined member '" + std::string(expr.member.ptr(), expr.member.len()) + "' in package '" + target + "'");
                        }
                    }
                    return;
                }
            }
        } else if (auto* mem_obj = std::get_if<MemberExpr>(&obj_expr)) {
            // e.g. mod.math.PI
            // if mem_obj resolves to 'mod.math', then expr is 'PI'
            if (auto* base_id = std::get_if<IdentifierExpr>(&program_.expressions[mem_obj->object.index])) {
                auto sym = find_symbol(base_id->name);
                if (sym && sym->is_namespace && sym->namespace_target.ends_with(".*")) {
                    // This is `mod.math`. We just check if `math` is a valid package, then check `PI` inside `math`
                    std::string pkg_name = std::string(mem_obj->member.ptr(), mem_obj->member.len());
                    auto pkg_it = packages_.find(pkg_name);
                    if (pkg_it != packages_.end()) {
                        bool found = false;
                        for (const auto& pkg_sym : pkg_it->second.symbols) {
                            if (pkg_sym.name == expr.member) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            report_error("Undefined member '" + std::string(expr.member.ptr(), expr.member.len()) + "' in package '" + pkg_name + "'");
                        }
                    }
                    return;
                }
            }
        }
        
        visit_expr(obj_expr, this);
    }
}

void SemanticAnalyzer::visit(const TypeMemberExpr& expr) {}

void SemanticAnalyzer::visit(const IndexExpr& expr) {
    if (!expr.object.is_null()) visit_expr(program_.expressions[expr.object.index], this);
    if (!expr.index.is_null()) visit_expr(program_.expressions[expr.index.index], this);
}

void SemanticAnalyzer::visit(const SliceExpr& expr) {
    if (!expr.object.is_null()) visit_expr(program_.expressions[expr.object.index], this);
    if (!expr.start.is_null()) visit_expr(program_.expressions[expr.start.index], this);
    if (!expr.end.is_null()) visit_expr(program_.expressions[expr.end.index], this);
}

void SemanticAnalyzer::visit(const AddressOfExpr& expr) {
    if (!expr.operand.is_null()) visit_expr(program_.expressions[expr.operand.index], this);
}

void SemanticAnalyzer::visit(const ArrayLiteralExpr& expr) {
    for (auto el : expr.elements) {
        if (!el.is_null()) visit_expr(program_.expressions[el.index], this);
    }
}

void SemanticAnalyzer::visit(const StructInitExpr& expr) {
    const StructDecl* struct_decl = nullptr;
    for (const auto& decl_variant : program_.declarations) {
        if (auto* s = std::get_if<StructDecl>(&decl_variant)) {
            if (s->name == expr.type_name) {
                struct_decl = s;
                break;
            }
        }
    }

    if (struct_decl) {
        // Validate named field initializers against the struct's own members
        for (const auto& named_arg : expr.field_values) {
            bool found = false;
            for (const auto& mem : struct_decl->members) {
                if (mem.name == named_arg.name) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                report_error("No such field '" + std::string(named_arg.name.ptr(), named_arg.name.len()) + "' in struct '" +
                    std::string(expr.type_name.ptr(), expr.type_name.len()) + "'");
            }
        }
    } else {
        report_error("Unknown struct type '" + std::string(expr.type_name.ptr(), expr.type_name.len()) + "'");
    }

    for (auto arg : expr.positional_values) {
        if (!arg.is_null()) visit_expr(program_.expressions[arg.index], this);
    }
    for (auto arg : expr.field_values) {
        if (!arg.value.is_null()) visit_expr(program_.expressions[arg.value.index], this);
    }
}

void SemanticAnalyzer::visit(const AllocExpr& expr) {
    if (!expr.element_type.is_null()) visit_type(program_.types[expr.element_type.index], this);
    if (expr.size && !expr.size.value().is_null()) visit_expr(program_.expressions[expr.size.value().index], this);
}

void SemanticAnalyzer::visit(const FreeExpr& expr) {
    if (!expr.pointer.is_null()) visit_expr(program_.expressions[expr.pointer.index], this);
}

void SemanticAnalyzer::visit(const BlockExpr& expr) {
    push_scope();
    for (auto stmt : expr.statements) {
        if (!stmt.is_null()) visit_stmt(program_.statements[stmt.index], this);
    }
    if (expr.value && !expr.value.value().is_null()) {
        visit_expr(program_.expressions[expr.value.value().index], this);
    }
    pop_scope();
}

void SemanticAnalyzer::visit(const SizeofExpr& expr) {
    if (expr.type_operand) visit_type(program_.types[expr.type_operand->index], this);
    if (expr.expr_operand) visit_expr(program_.expressions[expr.expr_operand->index], this);
}

// ----------------------------------------------------------------------------
// Statements
// ----------------------------------------------------------------------------
void SemanticAnalyzer::visit(const BlockStmt& stmt) {
    validate_attributes(stmt.attributes);
    push_scope();
    for (auto s : stmt.statements) {
        if (!s.is_null()) visit_stmt(program_.statements[s.index], this);
    }
    pop_scope();
}

void SemanticAnalyzer::visit(const ReturnStmt& stmt) {
    if (stmt.value && !stmt.value.value().is_null()) {
        visit_expr(program_.expressions[stmt.value.value().index], this);
    }
}

void SemanticAnalyzer::visit(const IfStmt& stmt) {
    validate_attributes(stmt.attributes);
    if (stmt.condition && !stmt.condition.value().is_null()) {
        visit_expr(program_.expressions[stmt.condition.value().index], this);
    }
    if (!stmt.then_branch.is_null()) {
        visit_stmt(program_.statements[stmt.then_branch.index], this);
    }
    if (stmt.else_branch && !stmt.else_branch.value().is_null()) {
        visit_stmt(program_.statements[stmt.else_branch.value().index], this);
    }
}

void SemanticAnalyzer::visit(const WhileStmt& stmt) {
    validate_attributes(stmt.attributes);
    if (!stmt.condition.is_null()) visit_expr(program_.expressions[stmt.condition.index], this);
    if (!stmt.body.is_null()) visit_stmt(program_.statements[stmt.body.index], this);
    if (stmt.else_branch && !stmt.else_branch.value().is_null()) {
        visit_stmt(program_.statements[stmt.else_branch.value().index], this);
    }
}

void SemanticAnalyzer::visit(const ForStmt& stmt) {
    validate_attributes(stmt.attributes);
    if (!stmt.range.is_null()) visit_expr(program_.expressions[stmt.range.index], this);
    push_scope();
    add_symbol(stmt.variable, TypeHandle{0}, DeclHandle{}, true);
    if (!stmt.body.is_null()) visit_stmt(program_.statements[stmt.body.index], this);
    pop_scope();
    if (stmt.else_branch && !stmt.else_branch.value().is_null()) {
        visit_stmt(program_.statements[stmt.else_branch.value().index], this);
    }
}

void SemanticAnalyzer::visit(const BreakStmt& stmt) {}
void SemanticAnalyzer::visit(const ContinueStmt& stmt) {}

void SemanticAnalyzer::visit(const ExprStmt& stmt) {
    if (!stmt.expression.is_null()) {
        visit_expr(program_.expressions[stmt.expression.index], this);
        check_discard(stmt.expression);
    }
}

void SemanticAnalyzer::visit(const VarDeclStmt& stmt) {
    validate_attributes(stmt.attributes);
    if (stmt.type && !stmt.type.value().is_null()) visit_type(program_.types[stmt.type.value().index], this);
    if (stmt.initializer && !stmt.initializer.value().is_null()) {
        visit_expr(program_.expressions[stmt.initializer.value().index], this);
    }
    add_symbol(stmt.name, stmt.type.value_or(TypeHandle{0}), DeclHandle{}, stmt.is_const, has_attribute(stmt.attributes, "unused"));
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
    if (!stmt.body.is_null()) visit_stmt(program_.statements[stmt.body.index], this);
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
// Modules and Packages
// ----------------------------------------------------------------------------
void SemanticAnalyzer::visit(const PackageDecl& decl) {
    validate_attributes(decl.attributes);
    push_scope();
    for (auto d : decl.declarations) {
        if (!d.is_null()) visit_decl(program_.declarations[d.index], this);
    }
    std::string pkg_name = std::string(decl.name.ptr(), decl.name.len());
    auto& pkg_scope = packages_[pkg_name];
    for (const auto& sym : scopes_.back().symbols) {
        pkg_scope.symbols.push_back(sym);
    }
    pop_scope();
}

void SemanticAnalyzer::visit(const ModuleDecl& decl) {
    validate_attributes(decl.attributes);
    push_scope();
    for (auto d : decl.declarations) {
        if (!d.is_null()) visit_decl(program_.declarations[d.index], this);
    }
    pop_scope();
}

void SemanticAnalyzer::visit(const ImportDecl& decl) {
    std::string mod_name = std::string(decl.module_name.ptr(), decl.module_name.len());
    if (modules_.find(mod_name) == modules_.end()) {
        report_error("Module '" + mod_name + "' not found");
        return;
    }

    if (decl.alias) {
        // import mod.pkg as m
        if (!decl.package_name) {
            report_error("Alias requires a package name");
            return;
        }
        std::string pkg_name = std::string(decl.package_name->ptr(), decl.package_name->len());
        std::string alias_name = std::string(decl.alias->ptr(), decl.alias->len());
        
        // Verify module exports this package
        const auto& exports = modules_[mod_name];
        if (std::find(exports.begin(), exports.end(), pkg_name) == exports.end()) {
            report_error("Module '" + mod_name + "' does not export package '" + pkg_name + "'");
            return;
        }

        // Add namespace symbol
        Symbol sym{
            .name = decl.alias.value(),
            .type = TypeHandle{0},
            .decl_handle = DeclHandle{},
            .is_const = true,
            .is_used = false,
            .allow_unused = true,
            .is_namespace = true,
            .namespace_target = pkg_name
        };
        scopes_.back().symbols.push_back(sym);
    } else if (decl.is_wildcard) {
        // import mod.*
        const auto& exports = modules_[mod_name];
        for (const auto& pkg_name : exports) {
            // We just add a namespace symbol for each exported package under its own name
            // Note: We need a Str pointing to the package name... but Str requires a ptr and len.
            // For simplicity, we assume we can look up the original package name Str.
            // But we don't have the Str easily. Let's create a dummy Str or allocate one, but this is a semantic analyzer.
            // Actually, if we just want to import ALL MEMBERS of all packages into the global scope? 
            // "import <module>.* // import all packages, access via <module>.<package>.member"
            // Wait! The user says "access via <module>.<package>.member".
            // So `import mod.*` means we add `mod` as a namespace?
            // "import <module>.* // import all packages, access via <module>.<package>.member"
            // Let's add `mod` as a namespace pointing to a special multi-package scope?
            // This is tricky. Let's just add `mod` as a namespace and handle it in MemberExpr.
        }
        Symbol sym{
            .name = decl.module_name,
            .type = TypeHandle{0},
            .decl_handle = DeclHandle{},
            .is_const = true,
            .is_used = false,
            .allow_unused = true,
            .is_namespace = true,
            .namespace_target = mod_name + ".*" // Special wildcard namespace
        };
        scopes_.back().symbols.push_back(sym);
    }
}

void SemanticAnalyzer::visit(const ExportPackagesDecl& decl) {
    // Handled semantically during module exports validation in Pre-Pass
}

void SemanticAnalyzer::visit(const TypeAliasDecl& decl) {
    validate_attributes(decl.attributes);
    if (!decl.target_type.is_null()) visit_type(program_.types[decl.target_type.index], this);
    add_symbol(decl.name, decl.target_type, DeclHandle{}, false);
}

// ----------------------------------------------------------------------------
// Declarations
// ----------------------------------------------------------------------------
void SemanticAnalyzer::visit(const FunctionDecl& decl) {
    validate_attributes(decl.attributes);
    std::cout << "Visiting FunctionDecl: " << std::string(decl.name.ptr(), decl.name.len()) << "\n";
    push_scope();
    for (const auto& param : decl.parameters) {
        add_symbol(param.name, param.type, DeclHandle{}, param.is_const, has_attribute(param.attributes, "unused"));
    }
    if (!decl.body.is_null()) visit_stmt(program_.statements[decl.body.index], this);
    pop_scope();
}

void SemanticAnalyzer::visit(const VariableDecl& decl) {
    validate_attributes(decl.attributes);
    std::cout << "Visiting VariableDecl: " << std::string(decl.name.ptr(), decl.name.len()) << "\n";
    if (decl.type && !decl.type.value().is_null()) visit_type(program_.types[decl.type.value().index], this);
    if (decl.initializer && !decl.initializer.value().is_null()) {
        visit_expr(program_.expressions[decl.initializer.value().index], this);
    }
    add_symbol(decl.name, decl.type.value_or(TypeHandle{0}), DeclHandle{}, decl.is_const, has_attribute(decl.attributes, "unused"));
}

void SemanticAnalyzer::flatten_struct_bases(StructDecl& decl, std::vector<StructMember>& out_members, uint32_t& current_offset) {
    for (auto base_name : decl.bases) {
        for (const auto& base_decl_variant : program_.declarations) {
            if (auto* base_struct = std::get_if<StructDecl>(&base_decl_variant)) {
                if (base_struct->name == base_name) {
                    flatten_struct_bases(const_cast<StructDecl&>(*base_struct), out_members, current_offset);
                    for (const auto& member : base_struct->members) {
                        StructMember copied_member = member;
                        if (copied_member.offset.has_value()) {
                            copied_member.offset = current_offset + copied_member.offset.value();
                        } else {
                            copied_member.offset = current_offset;
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

    // Append own members
    for (auto& member : decl.members) {
        if (member.offset.has_value()) {
            if (member.offset.value() < current_offset) {
                report_error("Overlap or invalid explicit offset for member '" + std::string(member.name.ptr(), member.name.len()) + "'");
            }
            current_offset = member.offset.value();
        } else {
            member.offset = current_offset;
        }

        // Check for name collision with base members
        for (const auto& base_mem : flattened_members) {
            if (base_mem.name == member.name) {
                report_error("Member name collision with base struct: '" + std::string(member.name.ptr(), member.name.len()) + "'");
                break;
            }
        }

        flattened_members.push_back(member);
        current_offset += 4;
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

    // Don't re-add if already added in pre-pass
    if (!find_symbol(decl.name)) {
        add_symbol(decl.name, TypeHandle{0});
    }
}

void SemanticAnalyzer::flatten_enum_bases(EnumDecl& decl, std::vector<EnumMember>& out_members) {
    if (!decl.extends) return;

    for (const auto& base_decl_variant : program_.declarations) {
        if (auto* base_enum = std::get_if<EnumDecl>(&base_decl_variant)) {
            if (base_enum->name == decl.extends.value()) {
                flatten_enum_bases(const_cast<EnumDecl&>(*base_enum), out_members);
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
        for (const auto& base_mem : flattened_members) {
            if (base_mem.name == member.name) {
                report_error("Enum member name collision with base enum: '" + std::string(member.name.ptr(), member.name.len()) + "'");
            }
        }
        flattened_members.push_back(member);
    }

    decl.members = program_.allocate_array(flattened_members);

    add_symbol(decl.name, TypeHandle{0});
    if (!decl.base_type.is_null()) visit_type(program_.types[decl.base_type.index], this);
    for (auto& member : decl.members) {
        if (member.value && !member.value.value().is_null()) {
            visit_expr(program_.expressions[member.value.value().index], this);
        }
    }
}

void SemanticAnalyzer::flatten_flag_bases(FlagDecl& decl, std::vector<EnumMember>& out_members) {
    if (!decl.extends) return;
    for (const auto& base_decl_variant : program_.declarations) {
        if (auto* base_flag = std::get_if<FlagDecl>(&base_decl_variant)) {
            if (base_flag->name == decl.extends.value()) {
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
        if (base_mem.value && !base_mem.value.value().is_null()) {
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
            if (base_mem.name == member.name) {
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
            if (!member.value.value().is_null()) {
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
        }
        add_symbol(member.name, TypeHandle{0});
        flattened_members.push_back(member);
    }
    pop_scope();

    decl.members = program_.allocate_array(flattened_members);

    uint64_t final_max_val = 0;
    for (auto& m : decl.members) {
        if (m.value && !m.value.value().is_null()) {
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
        if (arg && !arg.value().is_null()) visit_expr(program_.expressions[arg.value().index], this);
    }
    add_symbol(decl.name, TypeHandle{0});
}

void SemanticAnalyzer::visit(const AllocDecl& decl) {
    if (!decl.element_type.is_null()) visit_type(program_.types[decl.element_type.index], this);
    if (decl.size && !decl.size.value().is_null()) visit_expr(program_.expressions[decl.size.value().index], this);
    add_symbol(decl.name, TypeHandle{0});
}

} // namespace ibex
