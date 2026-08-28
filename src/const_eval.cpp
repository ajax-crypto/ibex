// Copyright (c) 2026 Akash Pradhan
// SPDX-License-Identifier: MIT

#include "const_eval.h"
#include <cmath>
#include <climits>
#include <sstream>

namespace ibex {

void ConstExprEvaluator::set_const(const std::string& name, ConstValue value) {
    constants_[name] = std::move(value);
}

bool ConstExprEvaluator::is_truthy(const ConstValue& val) {
    if (auto* b = std::get_if<bool>(&val)) return *b;
    if (auto* i = std::get_if<int64_t>(&val)) return *i != 0;
    if (auto* d = std::get_if<double>(&val)) return *d != 0.0;
    if (auto* s = std::get_if<std::string>(&val)) return !s->empty();
    return false;
}

std::string ConstExprEvaluator::to_string(const ConstValue& val) {
    if (auto* b = std::get_if<bool>(&val)) return *b ? "true" : "false";
    if (auto* i = std::get_if<int64_t>(&val)) return std::to_string(*i);
    if (auto* d = std::get_if<double>(&val)) {
        std::ostringstream oss;
        oss << *d;
        return oss.str();
    }
    if (auto* s = std::get_if<std::string>(&val)) return "\"" + *s + "\"";
    if (auto* sv = std::get_if<std::shared_ptr<StructValue>>(&val)) {
        if (!*sv) return "null";
        std::string res = (*sv)->type_name + "{";
        bool first = true;
        for (const auto& [k, v] : (*sv)->fields) {
            if (!first) res += ", ";
            res += k + ": " + to_string(v);
            first = false;
        }
        res += "}";
        return res;
    }
    return "<unknown>";
}

std::optional<ConstValue> ConstExprEvaluator::evaluate(ExprHandle handle) {
    if (handle.is_null()) return std::nullopt;
    auto& expr = program_.expressions[handle.index];

    if (auto* lit = std::get_if<LiteralExpr>(&expr)) return eval_literal(*lit);
    if (auto* id = std::get_if<IdentifierExpr>(&expr)) return eval_identifier(*id);
    if (auto* mod_param = std::get_if<ModuleParamExpr>(&expr)) return eval_module_param(*mod_param);
    if (auto* struct_init = std::get_if<StructInitExpr>(&expr)) return eval_struct_init(*struct_init);
    if (auto* mem = std::get_if<MemberExpr>(&expr)) return eval_member_expr(*mem);
    if (auto* bin = std::get_if<BinaryExpr>(&expr)) return eval_binary(*bin);
    if (auto* un = std::get_if<UnaryExpr>(&expr)) return eval_unary(*un);

    return std::nullopt;
}

std::optional<ConstValue> ConstExprEvaluator::eval_module_param(const ModuleParamExpr& expr) {
    if (current_module_.empty() || !module_args_ || !module_params_) {
        report_error("Module parameter reference outside of a module context or evaluator not configured");
        return std::nullopt;
    }
    
    auto params_it = module_params_->find(current_module_);
    if (params_it == module_params_->end() || params_it->second.empty()) {
        report_error("Module '" + current_module_ + "' has no parameters");
        return std::nullopt;
    }
    
    std::string param_name(expr.name.ptr(), expr.name.len());
    int param_index = -1;
    for (size_t i = 0; i < params_it->second.size(); ++i) {
        std::string pname(params_it->second[i].name.ptr(), params_it->second[i].name.len());
        if (pname == param_name) {
            param_index = static_cast<int>(i);
            break;
        }
    }
    
    if (param_index == -1) {
        report_error("Invalid module parameter '$" + param_name + "'");
        return std::nullopt;
    }
    
    auto args_it = module_args_->find(current_module_);
    if (args_it == module_args_->end() || args_it->second.size() <= static_cast<size_t>(param_index)) {
        report_error("Module parameter '$" + param_name + "' has not been instantiated with a value yet");
        return std::nullopt;
    }
    
    return args_it->second[param_index];
}

std::optional<ConstValue> ConstExprEvaluator::eval_literal(const LiteralExpr& expr) {
    switch (expr.kind) {
        case LiteralExpr::Kind::INTEGER:
            return ConstValue{expr.value.int_value};
        case LiteralExpr::Kind::FLOAT:
            return ConstValue{expr.value.float_value};
        case LiteralExpr::Kind::BOOLEAN:
            return ConstValue{expr.value.bool_value};
        case LiteralExpr::Kind::STRING:
            return ConstValue{std::string(expr.value.string_value.value.ptr(),
                                          expr.value.string_value.value.len())};
        case LiteralExpr::Kind::NULL_VALUE:
            return ConstValue{int64_t(0)};
    }
    return std::nullopt;
}

std::optional<ConstValue> ConstExprEvaluator::eval_identifier(const IdentifierExpr& expr) {
    std::string name(expr.name.ptr(), expr.name.len());
    auto it = constants_.find(name);
    if (it != constants_.end()) return it->second;
    return std::nullopt;
}

std::optional<ConstValue> ConstExprEvaluator::eval_struct_init(const StructInitExpr& expr) {
    auto sv = std::make_shared<StructValue>();
    sv->type_name = std::string(expr.type_name.ptr(), expr.type_name.len());
    
    // Evaluate named fields
    for (const auto& arg : expr.field_values) {
        std::string fname(arg.name.ptr(), arg.name.len());
        auto val = evaluate(arg.value);
        if (!val) return std::nullopt;
        sv->fields[fname] = std::move(*val);
    }
    
    // For positional fields, we'd need the StructDecl to know names.
    // Let's look it up in program_.top_level_declarations
    if (!expr.positional_values.empty()) {
        const StructDecl* sdecl = nullptr;
        for (auto h : program_.top_level_declarations) {
            auto& d = program_.declarations[h.index];
            if (auto* m = std::get_if<ModuleDecl>(&d)) {
                for (auto mh : m->declarations) {
                    auto& md = program_.declarations[mh.index];
                    if (auto* sd = std::get_if<StructDecl>(&md)) {
                        if (std::string(sd->name.ptr(), sd->name.len()) == sv->type_name) {
                            sdecl = sd;
                            break;
                        }
                    }
                }
            }
            if (sdecl) break;
            if (auto* sd = std::get_if<StructDecl>(&d)) {
                if (std::string(sd->name.ptr(), sd->name.len()) == sv->type_name) {
                    sdecl = sd;
                    break;
                }
            }
        }
        
        if (sdecl) {
            for (size_t i = 0; i < expr.positional_values.size() && i < sdecl->members.size(); ++i) {
                std::string fname(sdecl->members[i].name.ptr(), sdecl->members[i].name.len());
                auto val = evaluate(expr.positional_values[i]);
                if (!val) return std::nullopt;
                sv->fields[fname] = std::move(*val);
            }
        }
    }
    
    return sv;
}

std::optional<ConstValue> ConstExprEvaluator::eval_member_expr(const MemberExpr& expr) {
    auto obj = evaluate(expr.object);
    if (!obj) return std::nullopt;
    
    if (auto* sv = std::get_if<std::shared_ptr<StructValue>>(&*obj)) {
        if (!*sv) return std::nullopt;
        std::string member_name(expr.member.ptr(), expr.member.len());
        auto it = (*sv)->fields.find(member_name);
        if (it != (*sv)->fields.end()) {
            return it->second;
        }
    }
    report_error("Cannot evaluate member access at compile-time for this object");
    return std::nullopt;
}

std::optional<ConstValue> ConstExprEvaluator::eval_binary(const BinaryExpr& expr) {
    auto lhs = evaluate(expr.left);
    auto rhs = evaluate(expr.right);
    if (!lhs || !rhs) return std::nullopt;

    // Integer binary operations
    if (auto* li = std::get_if<int64_t>(&*lhs)) {
        if (auto* ri = std::get_if<int64_t>(&*rhs)) {
            switch (expr.op) {
                case TokenType::PLUS: {
                    // Overflow check: if both positive and result negative, or both negative and result positive
                    int64_t result = *li + *ri;
                    if ((*ri > 0 && *li > INT64_MAX - *ri) || (*ri < 0 && *li < INT64_MIN - *ri)) {
                        report_error("Compile-time integer overflow in addition: " +
                                     std::to_string(*li) + " + " + std::to_string(*ri));
                        return std::nullopt;
                    }
                    return ConstValue{result};
                }
                case TokenType::MINUS: {
                    if ((*ri < 0 && *li > INT64_MAX + *ri) || (*ri > 0 && *li < INT64_MIN + *ri)) {
                        report_error("Compile-time integer overflow in subtraction: " +
                                     std::to_string(*li) + " - " + std::to_string(*ri));
                        return std::nullopt;
                    }
                    return ConstValue{*li - *ri};
                }
                case TokenType::STAR: {
                    // Simple overflow check for multiplication
                    if (*li != 0 && *ri != 0) {
                        if ((*li > 0 && *ri > 0 && *li > INT64_MAX / *ri) ||
                            (*li < 0 && *ri < 0 && *li < INT64_MAX / *ri) ||
                            (*li > 0 && *ri < 0 && *ri < INT64_MIN / *li) ||
                            (*li < 0 && *ri > 0 && *li < INT64_MIN / *ri)) {
                            report_error("Compile-time integer overflow in multiplication: " +
                                         std::to_string(*li) + " * " + std::to_string(*ri));
                            return std::nullopt;
                        }
                    }
                    return ConstValue{*li * *ri};
                }
                case TokenType::SLASH:
                    if (*ri == 0) {
                        report_error("Compile-time integer division by zero");
                        return std::nullopt;
                    }
                    return ConstValue{*li / *ri};
                case TokenType::PERCENT:
                    if (*ri == 0) {
                        report_error("Compile-time integer modulo by zero");
                        return std::nullopt;
                    }
                    return ConstValue{*li % *ri};
                case TokenType::EQ_EQ:      return ConstValue{*li == *ri};
                case TokenType::NOT_EQ:     return ConstValue{*li != *ri};
                case TokenType::LESS:       return ConstValue{*li < *ri};
                case TokenType::GREATER:    return ConstValue{*li > *ri};
                case TokenType::LESS_EQ:    return ConstValue{*li <= *ri};
                case TokenType::GREATER_EQ: return ConstValue{*li >= *ri};
                case TokenType::PIPE:       return ConstValue{*li | *ri};
                case TokenType::CARET:      return ConstValue{*li ^ *ri};
                case TokenType::AMPERSAND:  return ConstValue{*li & *ri};
                case TokenType::LSHIFT:     return ConstValue{*li << *ri};
                case TokenType::RSHIFT:     return ConstValue{*li >> *ri};
                default: return std::nullopt;
            }
        }
    }

    // Float binary operations
    if (auto* lf = std::get_if<double>(&*lhs)) {
        if (auto* rf = std::get_if<double>(&*rhs)) {
            switch (expr.op) {
                case TokenType::PLUS:       return ConstValue{*lf + *rf};
                case TokenType::MINUS:      return ConstValue{*lf - *rf};
                case TokenType::STAR:       return ConstValue{*lf * *rf};
                case TokenType::SLASH: {
                    if (*rf == 0.0) {
                        // IEEE 754: float / 0.0 = ±INF
                        double inf = (*lf >= 0.0) ? INFINITY : -INFINITY;
                        return ConstValue{inf};
                    }
                    return ConstValue{*lf / *rf};
                }
                case TokenType::EQ_EQ:      return ConstValue{*lf == *rf};
                case TokenType::NOT_EQ:     return ConstValue{*lf != *rf};
                case TokenType::LESS:       return ConstValue{*lf < *rf};
                case TokenType::GREATER:    return ConstValue{*lf > *rf};
                case TokenType::LESS_EQ:    return ConstValue{*lf <= *rf};
                case TokenType::GREATER_EQ: return ConstValue{*lf >= *rf};
                default: return std::nullopt;
            }
        }
    }

    // Boolean binary operations
    if (auto* lb = std::get_if<bool>(&*lhs)) {
        if (auto* rb = std::get_if<bool>(&*rhs)) {
            switch (expr.op) {
                case TokenType::AMPERSAND_AMPERSAND: return ConstValue{*lb && *rb};
                case TokenType::PIPE_PIPE:           return ConstValue{*lb || *rb};
                case TokenType::EQ_EQ:               return ConstValue{*lb == *rb};
                case TokenType::NOT_EQ:              return ConstValue{*lb != *rb};
                default: return std::nullopt;
            }
        }
    }

    // String concatenation
    if (auto* ls = std::get_if<std::string>(&*lhs)) {
        if (auto* rs = std::get_if<std::string>(&*rhs)) {
            if (expr.op == TokenType::PLUS) {
                return ConstValue{*ls + *rs};
            }
        }
    }

    return std::nullopt;
}

std::optional<ConstValue> ConstExprEvaluator::eval_unary(const UnaryExpr& expr) {
    auto operand = evaluate(expr.operand);
    if (!operand) return std::nullopt;

    switch (expr.op) {
        case TokenType::MINUS:
            if (auto* i = std::get_if<int64_t>(&*operand)) return ConstValue{-*i};
            if (auto* d = std::get_if<double>(&*operand)) return ConstValue{-*d};
            break;
        case TokenType::TILDE:
            if (auto* i = std::get_if<int64_t>(&*operand)) return ConstValue{~*i};
            break;
        case TokenType::BANG:
            if (auto* b = std::get_if<bool>(&*operand)) return ConstValue{!*b};
            break;
        default:
            break;
    }
    return std::nullopt;
}

} // namespace ibex
