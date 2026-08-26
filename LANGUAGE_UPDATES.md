# Language Updates - April 6, 2026

## Changes Implemented

### 1. Scope Resolution Operator (`.` for static members)

**Requirement**: Use `.` for everything, including type-level properties for user-defined types.

**Implementation**:
- Added `TypeMemberExpr` to the AST for explicit type member access
- Existing `MemberExpr` already supports accessing members on expressions (which can be identifiers of types)
- Example: `S.x` for static member `x` on struct `S`, `Color.Red` for enum value `Red` on enum `Color`
- The parser creates an `IdentifierExpr` for the type name, then a `MemberExpr` accessing the member
- Semantic analyzer will determine if it's instance or type member access

**Files Modified**:
- `include/ast_new.h`: Added `TypeMemberExpr` struct and added it to `Expr` variant

### 2. For-Else and While-Else Statements  

**Requirement**: Support for `for-else` and `while-else` statements where the `else` block executes if the condition fails on the first evaluation.

**Semantics**:
- While-else: If the while condition is false on the first check, execute the else block
- For-else: If the range is empty (first iteration doesn't happen), execute the else block

**Implementation**:
- Updated `WhileStmt` to include `std::optional<StmtHandle> else_branch`
- Updated `ForStmt` to include `std::optional<StmtHandle> else_branch`
- Updated `parse_while_statement()` to parse optional `else` clause after the body
- Updated `parse_for_statement()` to parse optional `else` clause after the body

**Files Modified**:
- `include/ast_new.h`: Added `else_branch` field to both `WhileStmt` and `ForStmt`
- `src/parser_new.cpp`: Updated `parse_while_statement()` and `parse_for_statement()`

**Example Syntax**:
```ibex
while condition {
    doSomething();
} else {
    // Executed if condition was false on first check
}

for i in 0..10 {
    process(i);
} else {
    // Executed if range was empty
}
```

### 3. Extendable Enums

**Requirement**: Enums can be extended by users to create their own types with inheritance.

**Implementation**:
- Updated `EnumDecl` to include `std::optional<Str> extends` field
- Kept `base_type` for the underlying primitive type
- When `extends` is set, it references another enum to extend from
- Example: `enum tribool : boolean { maybe }` extends the `boolean` enum

**Parser Changes**:
- Updated `parse_enum_decl()` to support both primitive type declaration and enum extension syntax
- The parser checks if the token after `:` is a primitive type or an identifier (potential enum)
- If it's an identifier, it's treated as an extended enum

**Automatic Features** (to be implemented in semantic analyzer):
- Automatic deduction of minimum and maximum values including inherited members
- Check if a given value of underlying type is representable in the enum
- Automatic `to_text()` and `from_text()` functions for string conversion
- Enum values include all members from base enum plus new members

**Files Modified**:
- `include/ast_new.h`: Added `extends` field to `EnumDecl`
- `src/parser_new.cpp`: Updated `parse_enum_decl()` to handle enum extension

**Example Syntax**:
```ibex
enum boolean : bool { 
    no, 
    yes 
}

enum tribool : boolean { 
    maybe      // Maybe gets added after "yes"
}

// Access enum values with . operator
x: boolean = boolean.no;
y: tribool = tribool.maybe;
```

## Next Steps

1. **Semantic Analysis**: Implement type checking and validation for:
   - Static member access (determine if S.x is instance or type member)
   - Enum extension and inheritance
   - For-else and while-else semantics

2. **Code Generation**: 
   - Generate correct control flow for for-else and while-else
   - Implement enum hierarchy and automatic functions
   - Handle static member access in code generation

3. **Type Registry**:
   - Track enum inheritance relationships
   - Generate auto functions (to_text, from_text, min, max, is_valid)
   - Support polymorphic enum operations

## Testing

The following code should now parse successfully:
```ibex
// Enums with extension
enum Status : u8 {
    Idle = 0,
    Running = 1
}

enum ExtStatus : Status {
    Paused = 2,
    Done = 3
}

// For-else and while-else
for i in 0..10 {
    print(i);
} else {
    print("No items");
}

while x > 0 {
    x -= 1;
} else {
    print("Already zero");
}

// Static member access
struct Config {
    static VERSION: u32 = 1;
}
version: u32 = Config.VERSION;

// Enum value access
state: Status = Status.Running;
```
