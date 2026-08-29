// Copyright (c) 2026 Akash Pradhan
// SPDX-License-Identifier: MIT

#pragma once

#include "str.h"
#include "arena.h"
#include "lexer.h"

#include <variant>
#include <optional>
#include <span>
#include <cstdint>

namespace ibex {

// ============================================================================
// STRONG TYPEDEF FOR HANDLES - Type-safe indices instead of pointers
// ============================================================================

template<typename Tag>
struct Handle {
    uint32_t index;

    constexpr explicit Handle(uint32_t idx = 0xffffffff) : index(idx) {}
    constexpr bool is_null() const { return index == 0xffffffff; }
    constexpr bool operator==(Handle other) const { return index == other.index; }
    constexpr bool operator!=(Handle other) const { return index != other.index; }
};

// Type handles
struct TypeTag {};
using TypeHandle = Handle<TypeTag>;

// Expression handles
struct ExprTag {};
using ExprHandle = Handle<ExprTag>;

// Statement handles
struct StmtTag {};
using StmtHandle = Handle<StmtTag>;

// Declaration handles
struct DeclTag {};
using DeclHandle = Handle<DeclTag>;

// ============================================================================

struct Attribute {
    Str name;
    std::span<ExprHandle> args;
};

// ============================================================================
// TYPES - Discriminated union for type expressions
// ============================================================================

struct PrimitiveType {
    TokenType primitive;  // I8, I16, I32, I64, U8, U16, U32, U64, BYTE, F32, F64, BOOL
};

// Pointer type: *T (nullable, can be null)
struct PointerType {
    TypeHandle base;
};

// Reference type: &T (non-nullable, always points to valid data)
struct ReferenceType {
    TypeHandle base;
};

// Array type: [N]T (fixed-size array, decays to slice when needed)
struct ArrayType {
    TypeHandle element;
    uint32_t size;        // Fixed array size
};

// Slice type: [:T] (fat pointer - address + size combo)
// Represents both:
// - Decayed array ([N]T decays to [:T])
// - Result of vector allocation (allocates a vector, returns [:T])
// Different from scalar pointer *T
struct SliceType {
    TypeHandle element;
};

struct NamedType {
    Str name;
};

struct TypeofType {
    ExprHandle expr;
};

struct FunctionType {
    std::span<TypeHandle> param_types;
    TypeHandle return_type;
};

// Tuple type: (T1, T2, ...)
struct TupleType {
    std::span<TypeHandle> element_types;
};

// Optional type: T?
struct OptionalType {
    TypeHandle element_type;
};

// Variant (sum) type: (T1 + T2 + ...)
struct VariantType {
    std::vector<TypeHandle> types;
};

// Range type: start..end
struct RangeType {
    TypeHandle base_type;
};

// Variadic type: ...
struct VariadicType {
};

// Type discriminated union
using Type = std::variant<PrimitiveType, PointerType, ReferenceType, ArrayType, SliceType, NamedType, TypeofType, FunctionType, TupleType, OptionalType, VariantType, RangeType, VariadicType>;

// ============================================================================
// EXPRESSIONS - Discriminated union for expressions
// ============================================================================

struct BinaryExpr {
    ExprHandle left;
    TokenType op;
    ExprHandle right;
};

struct UnaryExpr {
    TokenType op;
    ExprHandle operand;
};

struct LiteralExpr {
    enum class Kind : uint8_t {
        INTEGER,
        FLOAT,
        STRING,
        BOOLEAN,
        NULL_VALUE,
    };
    Kind kind;
    union {
        int64_t int_value;
        double float_value;
        struct {
            Str value;
            Str prefix;
        } string_value;
        bool bool_value;
    } value;
    TokenType type_suffix = TokenType::EOF_TOKEN;  // Explicit suffix (EOF_TOKEN = none)
};

struct IdentifierExpr {
    Str name;
};

struct ModuleParamExpr {
    Str name;
};

// Represents a named argument: name = expr
struct NamedArg {
    Str name;
    ExprHandle value;
};

struct CallExpr {
    ExprHandle function;
    std::span<ExprHandle> arguments;  // Points to arena-allocated array
    // When calling with named arguments, arguments are reordered to match declaration order
    // and optional named_args maps parameter names to expressions
    std::span<NamedArg> named_args;   // Optional named arguments (empty if all positional)
};

struct CastExpr {
    ExprHandle operand;
    TypeHandle target_type;
};

struct IsExpr {
    ExprHandle operand;
    TypeHandle target_type;
};

struct TypeofExpr {
    ExprHandle expr;
};

struct MoveExpr {
    ExprHandle operand;
};

struct MemberExpr {
    ExprHandle object;
    Str member;
};

// FFI Access expression: accessing C functions (e.g., c::malloc)
struct FFIAccessExpr {
    Str language; // e.g., "c"
    Str function_name; // e.g., "malloc"
};

// Type member expression: Static members accessed on types (e.g., S.x where S is a struct)
// Can be used for enums values (Color.Red), static members (S.x), etc.
struct TypeMemberExpr {
    Str type_name;       // Name of the type
    Str member;          // Name of the member/value
};

struct IndexExpr {
    ExprHandle object;
    ExprHandle index;
};

// Slice expression: arr[start:end]
struct SliceExpr {
    ExprHandle object;    // Array or slice being sliced
    ExprHandle start;     // Start index
    ExprHandle end;       // End index
};

struct BlockExpr {
    std::span<StmtHandle> statements;  // Implicit return from last expr
    std::optional<ExprHandle> value;
};

// Address-of expression: @variable (gets pointer/reference to variable)
// Returns *T (pointer) for heap-allocated data or &T (reference) for stack data
struct AddressOfExpr {
    ExprHandle operand;
};

// Array literal: { 1, 2, 3, 4 }
struct ArrayLiteralExpr {
    std::span<ExprHandle> elements;
};

// Struct initializer with designated/named initializers: S { x: 10, y: 20 }
// Can use named fields or positional, supports nested initialization
struct StructInitExpr {
    Str type_name;                          // Name of the struct being initialized
    std::span<NamedArg> field_values;       // Named field initializers (empty means all positional)
    std::span<ExprHandle> positional_values; // Positional values for unnamed fields
};

// Dynamic allocation expression
struct AllocExpr {
    enum class Kind : uint8_t {
        SCALAR,   // Scalar allocation: @alloc(T) returns *T
        VECTOR,   // Vector allocation: @alloc_vec(T, size) returns [:T]
    };
    Kind kind;
    TypeHandle element_type;
    std::optional<ExprHandle> size;   // For vector allocation, None for scalar
};

// Deallocation expression
struct FreeExpr {
    ExprHandle pointer;
};

// Sizeof expression
struct SizeofExpr {
    std::optional<TypeHandle> type_operand;
    std::optional<ExprHandle> expr_operand;
};

// Dereference expression: *expr
struct DereferenceExpr {
    ExprHandle pointer;
};

// Ref expression: ref expr or ref(expr)
struct RefExpr {
    ExprHandle operand;
};

// Function binding expression: #func(named_arg=value, ...)
// Creates a partially-applied function
struct BindingExpr {
    Str target_function;
    std::span<NamedArg> bound_args;  // Named parameter bindings
};

// Lambda/anonymous function parameter
struct LambdaParam {
    Str name;
    TypeHandle type;
};

// Lambda/anonymous function expression: (params) -> RetType { body }
struct LambdaExpr {
    std::span<LambdaParam> parameters;
    TypeHandle return_type;       // null handle if omitted (void)
    StmtHandle body;              // Block statement
};

// Tuple expression: (expr, expr, ...)
struct TupleExpr {
    std::span<ExprHandle> elements;
};

// Unwrap expression: expr?
struct UnwrapExpr {
    ExprHandle operand;
};

// Range expression: start..end
struct RangeExpr {
    ExprHandle start;
    ExprHandle end;
};

// Expression discriminated union
using Expr = std::variant<
    BinaryExpr,
    UnaryExpr,
    LiteralExpr,
    IdentifierExpr,
    ModuleParamExpr,
    CallExpr,
    CastExpr,
    IsExpr,
    TypeofExpr,
    MoveExpr,
    MemberExpr,
    TypeMemberExpr,
    IndexExpr,
    SliceExpr,
    BlockExpr,
    AddressOfExpr,
    ArrayLiteralExpr,
    StructInitExpr,
    AllocExpr,
    FreeExpr,
    SizeofExpr,
    BindingExpr,
    LambdaExpr,
    TupleExpr,
    UnwrapExpr,
    DereferenceExpr,
    RefExpr,
    RangeExpr,
    FFIAccessExpr
>;

// ============================================================================
// STATEMENTS - Discriminated union for statements
// ============================================================================

struct BlockStmt {
    std::span<Attribute> attributes;
    std::span<StmtHandle> statements;
};

struct ReturnStmt {
    std::optional<ExprHandle> value;
};

struct IfStmt {
    std::span<Attribute> attributes;
    std::optional<ExprHandle> condition;  // Made optional for compile-time if
    StmtHandle then_branch;
    std::optional<StmtHandle> else_branch;
};

struct WhileStmt {
    std::span<Attribute> attributes;
    ExprHandle condition;
    StmtHandle body;
    std::optional<StmtHandle> else_branch;  // Executed if condition is false on first check
};

struct ForStmt {
    std::span<Attribute> attributes;
    
    // C-style loop fields
    bool is_c_style;
    std::optional<StmtHandle> init;
    std::optional<ExprHandle> condition;
    std::optional<StmtHandle> increment;
    
    // Range-style loop fields
    Str variable;         // Loop variable name
    std::optional<Str> index_variable;
    bool is_reverse;
    std::optional<ExprHandle> range;     // Range expression or source

    StmtHandle body;
    std::optional<StmtHandle> else_branch;  // Executed if range is empty
};

struct BreakStmt {};

struct ContinueStmt {};

struct ExprStmt {
    ExprHandle expression;
};

struct VarDeclStmt {
    std::span<Attribute> attributes;
    Str name;
    std::optional<TypeHandle> type;  // None means type deduction
    std::optional<ExprHandle> initializer;
    bool is_const;
    bool is_static;
};

struct ConstBlockStmt {
    std::span<Str> variables;
    StmtHandle body;
};

struct ConstModifierStmt {
    std::span<Str> variables;
};

struct CaseItem {
    std::optional<ExprHandle> value;     // null means default
    std::optional<ExprHandle> end_value; // for range, e.g. 1..3 (inclusive)
    StmtHandle body;
};

struct SwitchStmt {
    std::span<Attribute> attributes;
    ExprHandle target;
    std::span<CaseItem> cases;
};

// Statement discriminated union
using Stmt = std::variant<
    BlockStmt,
    ReturnStmt,
    IfStmt,
    WhileStmt,
    ForStmt,
    SwitchStmt,
    BreakStmt,
    ContinueStmt,
    ExprStmt,
    VarDeclStmt,
    ConstBlockStmt,
    ConstModifierStmt
>;

// ============================================================================
// DECLARATIONS - Discriminated union for declarations
// ============================================================================

struct FunctionParameter {
    std::span<Attribute> attributes;
    Str name;
    TypeHandle type;
    std::optional<ExprHandle> default_value;
    bool is_const;
};

struct FunctionDecl {
    std::span<Attribute> attributes;
    Str name;
    std::span<FunctionParameter> parameters;
    TypeHandle return_type;
    StmtHandle body;
};

struct VariableDecl {
    std::span<Attribute> attributes;
    Str name;
    std::optional<TypeHandle> type;  // None means type deduction (using :=)
    std::optional<ExprHandle> initializer;
    bool is_const;                    // const keyword (immutable)
    bool is_static;                   // static keyword
};

struct StructMember {
    std::span<Attribute> attributes;
    Str name;
    TypeHandle type;
    std::optional<ExprHandle> default_value;  // NSDMI
    std::optional<uint32_t> offset;            // Memory offset in bytes (from [[offset:N]] annotation), None for auto
};

struct StructDecl {
    std::span<Attribute> attributes;
    Str name;
    std::span<Str> bases;               // Base struct names for inheritance (can be multiple)
    std::span<StructMember> members;
};

struct EnumMember {
    std::span<Attribute> attributes;
    Str name;
    std::optional<ExprHandle> value;
};

struct EnumDecl {
    std::span<Attribute> attributes;
    Str name;
    TypeHandle base_type;           // Underlying primitive type (u8, i32, bool, etc.)
    std::optional<Str> extends;     // Optional base enum to extend
    std::span<EnumMember> members;
};

// Flag declaration - bitset enums with automatic power-of-two values
// flag S : i32 { first, second, third } => first=1, second=2, third=4
struct FlagDecl {
    std::span<Attribute> attributes;
    Str name;
    TypeHandle base_type;           // Underlying primitive type (u8, i32, i64, etc.)
    std::optional<Str> extends;     // Optional base flag to extend
    std::span<EnumMember> members;  // Members get auto-assigned power-of-two values
};

// Function binding declaration - compile-time function partial application
// using sum1 := #sum(, 3);  creates a new function with second param bound to 3
struct FunctionBindingDecl {
    std::span<Attribute> attributes;
    Str name;                          // New function name
    Str target_function;               // Function name to bind to (without type info)
    std::span<std::optional<ExprHandle>> bound_args;  // Bound arguments (None for parameters left open)
};

// Allocation declaration (for explicit control over memory)
struct AllocDecl {
    enum class Kind : uint8_t {
        SCALAR,   // Scalar: *T
        VECTOR,   // Vector: [:T]
    };
    Str name;
    Kind kind;
    TypeHandle element_type;
    std::optional<ExprHandle> size;   // Required for vector
};

struct PackageDecl {
    std::span<Attribute> attributes;
    Str name;
    std::vector<DeclHandle> declarations;
};

// Parameter for a parameterized module: module name(param: Type, ...);
struct ModuleParam {
    Str name;
    TypeHandle type;    // Must be a primitive type (bool, i32, etc.)
};

struct ModuleDecl {
    std::span<Attribute> attributes;
    Str name;
    std::vector<DeclHandle> declarations;
    std::vector<ModuleParam> parameters;  // Empty for non-parameterized modules
};

struct ImportDecl {
    std::span<Attribute> attributes;
    Str module_name;
    std::optional<Str> package_name;
    std::optional<Str> alias;
    bool is_wildcard;
    std::vector<ExprHandle> module_args;  // Compile-time constant arguments for parameterized imports
};

struct ExportPackagesDecl {
    std::span<Attribute> attributes;
    std::vector<Str> package_names;
};

struct TypeAliasDecl {
    std::span<Attribute> attributes;
    Str name;
    TypeHandle target_type;
    bool is_strong;
};

struct ForeignBlockDecl {
    std::span<Attribute> attributes;
    Str language;
    Str code;
};

// Declaration discriminated union
using Decl = std::variant<
    FunctionDecl,
    VariableDecl,
    StructDecl,
    EnumDecl,
    FlagDecl,
    FunctionBindingDecl,
    AllocDecl,
    PackageDecl,
    ModuleDecl,
    ImportDecl,
    ExportPackagesDecl,
    TypeAliasDecl,
    ForeignBlockDecl
>;

// ============================================================================
// PROGRAM - Complete AST representation
// ============================================================================

class Program {
public:
    explicit Program(Arena& arena) : arena_(arena) {}

    // Arena-based allocation for all nodes
    Arena& arena() { return arena_; }
    const Arena& arena() const { return arena_; }

    // Declare arrays are stored in vectors of variants
    std::vector<Decl> declarations;
    std::vector<Stmt> statements;
    std::vector<Expr> expressions;
    std::vector<Type> types;
    
    // Top-level declarations for semantic analysis
    std::vector<DeclHandle> top_level_declarations;

    // Helper to allocate arrays with span return
    template<typename T>
    std::span<T> allocate_array(const std::vector<T>& items) {
        T* ptr = arena_.allocate_array<T>(items.size());
        std::memcpy(ptr, items.data(), items.size() * sizeof(T));
        return std::span<T>(ptr, items.size());
    }

    // Helper to allocate string
    Str allocate_string(std::string_view sv) {
        char* ptr = static_cast<char*>(arena_.allocate(sv.size()));
        std::memcpy(ptr, sv.data(), sv.size());
        return Str(ptr, sv.size());
    }

private:
    Arena& arena_;
};

}  // namespace ibex
