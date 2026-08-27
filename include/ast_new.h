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

// Type discriminated union
using Type = std::variant<PrimitiveType, PointerType, ReferenceType, ArrayType, SliceType, NamedType>;

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
        Str string_value;
        bool bool_value;
    } value;
};

struct IdentifierExpr {
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

struct MemberExpr {
    ExprHandle object;
    Str member;
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

// Expression discriminated union
using Expr = std::variant<
    BinaryExpr,
    UnaryExpr,
    LiteralExpr,
    IdentifierExpr,
    CallExpr,
    CastExpr,
    MemberExpr,
    TypeMemberExpr,
    IndexExpr,
    SliceExpr,
    BlockExpr,
    AddressOfExpr,
    ArrayLiteralExpr,
    StructInitExpr,
    AllocExpr,
    FreeExpr
>;

// ============================================================================
// STATEMENTS - Discriminated union for statements
// ============================================================================

struct BlockStmt {
    std::span<StmtHandle> statements;
};

struct ReturnStmt {
    std::optional<ExprHandle> value;
};

struct IfStmt {
    ExprHandle condition;
    StmtHandle then_branch;
    std::optional<StmtHandle> else_branch;
};

struct WhileStmt {
    ExprHandle condition;
    StmtHandle body;
    std::optional<StmtHandle> else_branch;  // Executed if condition is false on first check
};

struct ForStmt {
    Str variable;         // Loop variable name
    ExprHandle range;     // Range expression (can be a literal range or call)
    StmtHandle body;
    std::optional<StmtHandle> else_branch;  // Executed if range is empty
};

struct BreakStmt {};

struct ContinueStmt {};

struct ExprStmt {
    ExprHandle expression;
};

struct VarDeclStmt {
    Str name;
    std::optional<TypeHandle> type;  // None means type deduction
    std::optional<ExprHandle> initializer;
    bool is_const;
};

struct ConstBlockStmt {
    std::span<Str> variables;
    StmtHandle body;
};

struct ConstModifierStmt {
    std::span<Str> variables;
};

// Statement discriminated union
using Stmt = std::variant<
    BlockStmt,
    ReturnStmt,
    IfStmt,
    WhileStmt,
    ForStmt,
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
    Str name;
    TypeHandle type;
    std::optional<ExprHandle> default_value;
    bool is_const;
};

struct FunctionDecl {
    Str name;
    std::span<FunctionParameter> parameters;
    TypeHandle return_type;
    StmtHandle body;
    std::span<Str> attributes;  // [[allocates]], [[deallocates]], etc.
};

struct VariableDecl {
    Str name;
    std::optional<TypeHandle> type;  // None means type deduction (using :=)
    std::optional<ExprHandle> initializer;
    bool is_const;                    // const keyword (immutable)
};

struct StructMember {
    Str name;
    TypeHandle type;
    std::optional<ExprHandle> default_value;  // NSDMI
    std::optional<uint32_t> offset;            // Memory offset in bytes (from [[offset:N]] annotation), None for auto
};

struct StructDecl {
    Str name;
    std::span<Str> bases;               // Base struct names for inheritance (can be multiple)
    std::span<StructMember> members;
};

struct ClassMember {
    Str name;
    TypeHandle type;
    bool is_public;
    std::optional<ExprHandle> default_value;
};

struct ClassDecl {
    Str name;
    std::span<ClassMember> members;
};

struct EnumMember {
    Str name;
    std::optional<ExprHandle> value;
};

struct EnumDecl {
    Str name;
    TypeHandle base_type;           // Underlying primitive type (u8, i32, bool, etc.)
    std::optional<Str> extends;     // Optional base enum to extend
    std::span<EnumMember> members;
};

// Flag declaration - bitset enums with automatic power-of-two values
// flag S : i32 { first, second, third } => first=1, second=2, third=4
struct FlagDecl {
    Str name;
    TypeHandle base_type;           // Underlying primitive type (u8, i32, i64, etc.)
    std::optional<Str> extends;     // Optional base flag to extend
    std::span<EnumMember> members;  // Members get auto-assigned power-of-two values
};

// Function binding declaration - compile-time function partial application
// using sum1 := #sum(, 3);  creates a new function with second param bound to 3
struct FunctionBindingDecl {
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

// Declaration discriminated union
using Decl = std::variant<
    FunctionDecl,
    VariableDecl,
    StructDecl,
    ClassDecl,
    EnumDecl,
    FlagDecl,
    FunctionBindingDecl,
    AllocDecl
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
