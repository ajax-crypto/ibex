#pragma once

#include "ast_new.h"
#include <variant>
#include <optional>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace ibex {

// A compile-time constant value
using ConstValue = std::variant<int64_t, double, bool, std::string>;

// Evaluate AST expressions to compile-time constant values.
// Used for module parameters, conditional exports, and const folding.
class ConstExprEvaluator {
public:
    explicit ConstExprEvaluator(const Program& program) : program_(program) {}

    // Evaluate an expression handle to a compile-time constant.
    // Returns std::nullopt if the expression is not const-evaluable.
    std::optional<ConstValue> evaluate(ExprHandle handle);

    // Register a named constant (e.g. module parameter binding)
    void set_const(const std::string& name, ConstValue value);

    // Check if a ConstValue is truthy (for conditional export evaluation)
    static bool is_truthy(const ConstValue& val);

    // Convert a ConstValue to string for error messages
    static std::string to_string(const ConstValue& val);

    // Error reporting
    const std::vector<std::string>& get_errors() const { return errors_; }
    bool has_errors() const { return !errors_.empty(); }

private:
    const Program& program_;
    std::unordered_map<std::string, ConstValue> constants_;
    std::vector<std::string> errors_;

    void report_error(const std::string& msg) { errors_.push_back(msg); }

    std::optional<ConstValue> eval_literal(const LiteralExpr& expr);
    std::optional<ConstValue> eval_identifier(const IdentifierExpr& expr);
    std::optional<ConstValue> eval_binary(const BinaryExpr& expr);
    std::optional<ConstValue> eval_unary(const UnaryExpr& expr);
};

} // namespace ibex
