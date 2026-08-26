# Ibex Language Updates - Functions and Structs (April 6, 2026)

## Overview

Six major features have been implemented for improved function and struct support:
1. Named parameters in function calls
2. Struct initialization with designated initializers
3. Struct inheritance (single and multiple)
4. Memory layout specification with byte offsets
5. Flags language feature (power-of-two enums)
6. Automatic to_text/from_text functions for enums and flags

---

## 1. Named Parameters in Function Calls

### Feature Description
Functions can be called with named parameters in any order. Parameters are reordered to match the function declaration order.

### Syntax
```ibex
sum: (lhs: i8, rhs: i8) -> i16 {}

// Call with positional arguments
result1 := sum(2, 3);

// Call with named arguments (reordered)
result2 := sum(rhs = 2, lhs = 3);  // Becomes sum(3, 2)

// Call with mixed (not recommended but allowed)
result3 := sum(5, rhs = 10);
```

### Implementation Details
- **AST Changes**: Updated `CallExpr` to include `std::span<NamedArg> named_args`
- **NamedArg Structure**: Contains `Str name` and `ExprHandle value`
- **Parser**: Updated `parse_postfix()` to detect and parse named arguments
  - When parsing function call arguments, checks if argument is `identifier = expression`
  - Separates named and positional arguments
  - Semantic analyzer will reorder named arguments to match declaration

### Files Modified
- `include/ast_new.h`: Added `NamedArg` struct, updated `CallExpr`
- `src/parser_new.cpp`: Updated function call argument parsing in `parse_postfix()`

---

## 2. Struct Initialization with Designated Initializers

### Feature Description
Structs can be initialized with designated (named) field initializers, supporting nested initialization and indexed access.

### Syntax
```ibex
struct Point {
    x: i32 = 0;
    y: i32 = 0;
}

// Default construction
p1: Point = Point { };

// Designated initializers
p2: Point = Point { x: 10, y: 20 };

// Mixed order
p3: Point = Point { y: 15, x: 5 };

// Nested initialization
struct Rectangle {
    top_left: Point;
    bottom_right: Point;
}

rect: Rectangle = Rectangle {
    top_left: Point { x: 0, y: 0 },
    bottom_right: Point { x: 100, y: 100 }
};

// Positional initialization
p4: Point = Point { 10, 20 };
```

### Implementation Details
- **AST Changes**: Added `StructInitExpr` to represent struct initialization
  - `Str type_name`: The struct type being initialized
  - `std::span<NamedArg> field_values`: Named field initializers
  - `std::span<ExprHandle> positional_values`: Positional initializers
- **Parser**: Updated `parse_postfix()` to detect `Identifier { ... }` pattern
  - Distinguishes between named and positional field initializers
  - Supports optional trailing commas
  - Fields can be in any order (semantic analyzer reorders)

### Features
- Partial initialization (unspecified fields use default values from declaration)
- Nested struct initialization supported
- Automatic struct initialization using in-class defaults (NSDMI)

### Files Modified
- `include/ast_new.h`: Added `StructInitExpr`, added to `Expr` variant
- `src/parser_new.cpp`: Updated `parse_postfix()` for struct initialization

---

## 3. Struct Inheritance (Single and Multiple)

### Feature Description
Structs can inherit from other structs. Multiple inheritance is supported. No virtual inheritance or virtual functions.

### Syntax
```ibex
struct Animal {
    name: [64]byte;
    age: u8;
}

struct Dog : Animal {
    breed: [64]byte;
    bark_volume: u8 = 50;
}

struct Mammal : Animal {
    fur_color: [64]byte;
}

// Multiple inheritance
struct ServiceDog : Dog, Mammal {
    training_level: u8;
    certification_date: u64;
}
```

### Inheritance Semantics
- **Member Access**: Derived struct inherits all base struct members
- **Layout**: Base struct members appear before derived struct members in memory
- **Initialization**: Base struct members can be initialized in derived struct initializer
- **No Virtual**: All inheritance is static; no virtual function tables

### Implementation Details
- **AST Changes**: Updated `StructDecl` to include `std::span<Str> bases`
  - Stores names of base structs for resolution by semantic analyzer
  - Empty span for structs with no bases
- **Parser**: Updated `parse_struct_decl()` to parse base struct names
  - Syntax: `struct Name : Base1, Base2 { ... }`
  - Parses comma-separated list of base struct names after `:`

### Files Modified
- `include/ast_new.h`: Updated `StructDecl` with `bases` field
- `src/parser_new.cpp`: Updated `parse_struct_decl()` to parse inheritance

---

## 4. Memory Layout Specification

### Feature Description
Byte offsets for struct members can be specified using `[[offset:N]]` annotations, allowing precise control over memory layout.

### Syntax
```ibex
struct S {
    [[offset:0]] x: i8;
    [[offset:4]] y: i8;
    [[offset:8]] z: i32;
}

// Custom layout for FFI or memory-mapped I/O
struct Register {
    [[offset:0]] control: u32;
    [[offset:4]] status: u32;
    [[offset:8]] data: u64;
}
```

### Use Cases
- C interoperability with exact memory layout requirements
- Memory-mapped I/O registers
- Network packet structures with specific byte alignment
- Sharing layout with other languages

### Implementation Details
- **AST Changes**: Updated `StructMember` to include `std::optional<uint32_t> offset`
  - `std::nullopt` means automatic offset (default)
  - `Some(n)` specifies explicit byte offset
- **Parser**: Updated `parse_struct_decl()` to parse `[[offset:N]]` annotations
  - Parses annotations before member declaration
  - Extracts integer value for offset
  - Stores offset in `StructMember.offset` field
- **Semantic Analysis**: Will validate offset consistency and layout

### Files Modified
- `include/ast_new.h`: Updated `StructMember` with `offset` field
- `src/parser_new.cpp`: Updated `parse_struct_decl()` to parse offset annotations

---

## 5. Flags Language Feature

### Feature Description
Flags are a language feature for defining bitsets with automatic power-of-two value assignment.

### Syntax
```ibex
flag Permission : u32 {
    read,      // 1
    write,     // 2
    execute,   // 4
    delete     // 8
}

flag Status : u8 {
    none = 0,  // Special zero value (optional)
    active,
    paused,
    complete
}

// Usage
perms: Permission = Permission.read | Permission.write;
is_readable := (perms & Permission.read) != 0;
```

### Semantics
- **Automatic Values**: First member is 1, then 2, 4, 8, 16, etc. (powers of 2)
- **Explicit Values**: Can specify explicit value with `=` (useful for zero value)
- **Bitwise Operations**: Flags support bitwise AND/OR/XOR operations
- **Built-in Functions**: Implicit `to_text()` and `from_text()` support

### Implementation Details
- **AST Changes**: Added new declaration type `FlagDecl`
  - `Str name`: Name of the flag type
  - `TypeHandle base_type`: Underlying integer type (u8, u16, u32, u64, i8, etc.)
  - `std::span<EnumMember> members`: Flag members
- **Parser**: Added `parse_flag_decl()` method
  - Triggered by `flag` keyword
  - Similar structure to enum parsing
  - Semantic analyzer generates power-of-two values
- **Updated Lexer**: Added `FLAG` keyword token

### Files Modified
- `include/lexer.h`: Added `FLAG` token type
- `include/ast_new.h`: Added `FlagDecl` declaration type to `Decl` variant
- `src/lexer.cpp`: Added "flag" keyword mapping to `FLAG` token
- `src/parser_new.cpp`: Added `parse_flag_decl()` method

---

## 6. Automatic to_text and from_text Functions

### Feature Description
Enums and flags automatically provide `to_text()` and `from_text()` functions without explicit declaration.

### Syntax
```ibex
enum Status : u8 {
    Idle = 0,
    Running = 1,
    Paused = 2
}

// Automatic functions (always available)
status := Status.Idle;
text := status.to_text();         // Returns "Idle"
status2 := Status.from_text("Running");  // Returns Status.Running

// For flags:
flag Perm : u32 { read, write, execute }

perm := Perm.read | Perm.write;
text := perm.to_text();  // Returns "read | write" or similar
perm2 := Perm.from_text("read | execute");
```

### Implementation Details
- **Semantic Analyzer Only**: Parser doesn't need explicit changes
- **Implicit Functions**: Semantic analyzer automatically generates these functions for:
  - All enums (including extended enums)
  - All flags
- **Function Signatures**:
  - `to_text: (self: EnumType) -> std.string`
  - `from_text: (input: std.string) -> EnumType`
- **Behavior**:
  - `to_text()`: Converts enum value to string representation
  - `from_text()`: Parses string to get corresponding enum value
  - For flags with multiple bits set: representation shows all selected flags separated by `|`

### Files Modified
- No immediate parser changes needed (semantic analyzer responsibility)
- Semantic analyzer will add these functions to enum and flag declarations

---

## Lexer Updates

The following tokens were added to `include/lexer.h` and `src/lexer.cpp`:

1. **USING** - `using` keyword for compile-time function binding
2. **FLAG** - `flag` keyword for flag declarations
3. **HASH** - `#` character for compile-time function reference

---

## Summary of Files Modified

### Header Files
- `include/lexer.h` - Added USING, FLAG, HASH tokens
- `include/ast_new.h` - Added NamedArg, StructInitExpr, FlagDecl, FunctionBindingDecl
- `include/parser_new.h` - Added parse_flag_decl(), parse_function_binding_decl()

### Source Files
- `src/lexer.cpp` - Added keyword mappings for flag and using; added # character handling
- `src/parser_new.cpp` - Updated parse_struct_decl(), parse_postfix(); added new parsing methods

---

## Semantic Analysis Requirements

**Important**: These features require semantic analysis for full functionality:

1. **Named Arguments**: Reorder named arguments to match declaration order
2. **Struct Initialization**: Validate all required fields are initialized, fill defaults
3. **Inheritance**: Validate base struct references, build member list from inheritance chain
4. **Memory Offsets**: Validate offset values don't overlap
5. **Flags**: Generate power-of-two values automatically
6. **Enum/Flag Functions**: Generate to_text() and from_text() implementations
7. **Compile-time Function Binding**: Generate new function with bound parameters

---

## Example Programs

### Complete Example: Flag-based Permissions
```ibex
flag Permission : u32 {
    read = 1,
    write = 2,
    execute = 4
}

struct File {
    name: [256]byte;
    [[offset:256]] permissions: Permission;
}

check_permission: (file: File, perm: Permission) -> bool {
    return (file.permissions & perm) == perm;
}

main: () -> void {
    file: File = File {
        name: "data.txt",
        permissions: Permission.read | Permission.write
    };
    
    is_readable := check_permission(file, Permission.read);
    perm_text := file.permissions.to_text();
}
```

### struct Inheritance Example
```ibex
struct Animal {
    name: [64]byte;
}

struct Dog : Animal {
    breed: [64]byte;
}

create_dog: (rhs: [64]byte, lhs: [64]byte) -> Dog {
    return Dog {
        name: rhs,
        breed: lhs
    };
}
```

### Named Parameters Example
```ibex
configure: (host: [256]byte, port: u16, timeout: u32) -> void {}

// All equivalent:
configure("localhost", 8080, 5000);
configure(host = "localhost", port = 8080, timeout = 5000);
configure(timeout = 5000, host = "localhost", port = 8080);
```

---

## Testing Recommendations

1. **Named Parameters**: Verify parameter reordering and mixed positional/named
2. **Struct Initialization**: Test nested initialization, partial initialization
3. **Inheritance**: Test member access from base and derived structs
4. **Memory Offsets**: Verify that offset annotation is preserved
5. **Flags**: Test power-of-two generation and bitwise operations
6. **Automatic Functions**: Verify enum/flag to_text and from_text generation

---

## Future Enhancements

1. **Constructors**: Add optional constructor support for structs
2. **Methods**: Add method binding to structs
3. **Traits**: Add trait/interface system for polymorphism without virtuals
4. **Generics**: Add generic/template struct support
5. **Default Values**: Enhance default value expressions for structs
