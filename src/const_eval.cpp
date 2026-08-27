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
    return "<unknown>";
}

std::optional<ConstValue> ConstExprEvaluator::evaluate(ExprHandle handle) {
    if (handle.is_null()) return std::nullopt;
    auto& expr = program_.expressions[handle.index];

    if (auto* lit = std::get_if<LiteralExpr>(&expr)) return eval_literal(*lit);
    if (auto* id = std::get_if<IdentifierExpr>(&expr)) return eval_identifier(*id);
    if (auto* bin = std::get_if<BinaryExpr>(&expr)) return eval_binary(*bin);
    if (auto* un = std::get_if<UnaryExpr>(&expr)) return eval_unary(*un);

    return std::nullopt;
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
        case TokenType::BANG:
            if (auto* b = std::get_if<bool>(&*operand)) return ConstValue{!*b};
            break;
        default:
            break;
    }
    return std::nullopt;
}

} // namespace ibex
