# Ibex Language Specification

## Table of Contents
1. [Overview](#overview)
2. [Core Language Design](#core-language-design)
3. [Syntax](#syntax)
4. [Type System](#type-system)
5. [Grammar Specification](#grammar-specification)
6. [Memory Management](#memory-management)
7. [Advanced Features](#advanced-features)
8. [C Interoperability](#c-interoperability)
9. [Implementation Model](#implementation-model)

## Overview

Ibex is a native, non-garbage-collected programming language designed for systems programming. It compiles to object code and can be linked directly with C code. The language emphasizes:

- **Memory Safety**: Explicit memory management without garbage collection overhead
- **Performance**: Zero-cost abstractions and inline compilation
- **Interoperability**: Seamless integration with C libraries
- **Developer Experience**: Visual and textual code representation with integrated IDE

## Core Language Design

## Core Language Design

### Design Principles

1. **Simplicity**: Minimal syntax, low parser complexity
2. **Explicitness**: Intent is always clear from syntax
3. **Linearity**: No exceptions, no hidden control flow
4. **Safety**: Explicit memory management with annotations
5. **Expressiveness**: UFCS enables fluent, composable code
6. **Zero-Cost**: No runtime overhead beyond explicit allocations

### Node-Based Architecture

Unlike traditional file-based languages, Ibex uses a **node-based system**:

- **Types**: Stored in global type registry (not in files)
- **Functions**: Represented as computation nodes (like in game engines)
- **Linking**: Based on matching input/output types
- **IDE-Native**: Designed for visual node editors
- **No source files**: Code is provided through the IDE

This approach enables:
- Automatic function discovery by type signature
- Visual function composition
- Type-safe node linking
- Program-wide type refactoring

## Syntax

### Formal Grammar

```
program            := declaration*

declaration        := var_decl | func_decl | type_decl | enum_decl

var_decl           := identifier ':' type_expr ['=' expr] ';'
                   | identifier ':=' expr ';'  // type deduction

func_decl          := identifier ':' param_list '->' type_expr '{' statement* '}'

param_list         := '(' [param (',' param)*] ')'

param              := identifier ':' type_expr ['{' expr '}']  // default value

type_decl          := 'struct' identifier '{' field_list '}'
                   | 'class' identifier '{' visibility field_list method_list '}'

field_list         := (identifier ':' type_expr ';')*

enum_decl          := 'enum' identifier ':' type_expr '{' 
                        identifier ['=' expr] (',' identifier ['=' expr])* 
                     '}'

statement          := var_decl
                   | assignment
                   | expr ';'
                   | '{' statement* [return_expr] '}'
                   | 'if' expr '{' statement* '}' ['else' statement]
                   | 'for' identifier 'in' expr '{' statement* '}'
                   | 'while' expr '{' statement* '}'

assignment         := identifier ':=' expr ';'

expr               := primary (infix_op primary)*
                   | expr 'as' type_expr
                   | expr '.' identifier ['(' args ')']
                   | lambda_expr

lambda_expr        := '[' ']' '(' [param_list] ')' '->' type_expr '{' statement* '}'

primary            := identifier
                   | literal
                   | identifier '(' args ')'
                   | '{' statement* [return_expr] '}'
                   | '(' expr ')'

return_expr        := 'return' expr ';'
                   | expr ';'  // implicit return in blocks

args               := [expr (',' expr)*]

type_expr          := base_type ['*']
                   | base_type '[' expr ']'
                   | base_type '[' expr ':' ']'

base_type          := 'i8' | 'i16' | 'i32' | 'i64'
                   | 'u8' | 'u16' | 'u32' | 'u64'
                   | 'byte'
                   | 'f32' | 'f64'
                   | identifier
```

### Variable Declaration

```ibex
// Simple variable
x: i32 = 10;

// Type deduction
y := 20;            // inferred as i32

// With explicit type
name: [256]byte;    // array of bytes
```

### Function Declaration

```ibex
// Simple function
add: (i32, i32) -> i32 { return a + b; }

// With default parameters
sum: (i32, i32{3}) -> i64 {
    return (a + b) as i64;
}

// No return type annotation needed, inferred from last expr/return
process: (x: i32) -> i32 {
    y := x * 2;
    y         // implicit return
}
```

### Compound Types

```ibex
// Struct with inline initialization (NSDMI)
struct Point {
    x: i32 = 0;
    y: i32 = 0;
};

// Class with public/private
class Vector {
    private:
        capacity: u32 = 0;
        size: u32 = 0;
    public:
        init: (cap: u32) -> void;
        push: (value: i32) -> void;
};

// Enum (always scoped like C++ enum class)
enum Color: u8 {
    Red = 0,
    Green = 1,
    Blue = 2
};
```

### Universal Function Call Syntax (UFCS)

```ibex
// Define function with receiver type as first parameter
len: (v: Vector) -> u32 { return v.size; }

// Can be called as regular function
length := len(my_vector);

// Or with UFCS syntax
length := my_vector.len();
```

### Type Casting

```ibex
x: i32 = 42;
y: i8 = x as i8;           // explicit cast
z: f64 = x as f64;         // int to float

// Cast works in expressions
result := (value as i32) + 10;
```

### Blocks as Expressions

```ibex
// Block with return value
result: i32 = {
    x := 10;
    y := 20;
    return x + y;
};

// Equivalent to
result := {
    x := 10;
    y := 20;
    x + y              // implicit return
};
```

### Non-Capturing Lambdas

```ibex
// Lambda stored in variable
transform: (x: i32) -> i32 = [](x: i32) -> i32 {
    return x * 2;
};

// Used as function
result := transform(5);

// As function parameter
map: (arr: [10]i32, f: (i32) -> i32) -> void {
    for i in 0..10 {
        arr[i] = f(arr[i]);
    }
};
```

### Memory Annotations

```ibex
// Allocation annotation
create_buffer: () -> *byte [[allocates]] {
    return alloc(byte, 1024);
}

// Deallocation annotation
destroy_buffer: (buf: *byte) -> void [[deallocates]] {
    free(buf);
}

// These enable static analysis and validation
```

## Type System

### Primitive Types

| Type | Size | Description |
|------|------|-------------|
| `i8`, `i16`, `i32`, `i64` | 1, 2, 4, 8 bytes | Signed integers |
| `u8`, `u16`, `u32`, `u64` | 1, 2, 4, 8 bytes | Unsigned integers |
| `byte` | 1 byte | Unsigned 8-bit (alias to u8) |
| `f32`, `f64` | 4, 8 bytes | Floating point |
| `bool` | 1 byte | Boolean (true/false) |

**Note**: No `void` type. Functions with no return value simply omit the return type or use IIFE returns.

### Compound Types

#### Structs
```ibex
struct Person {
    name: [256]byte;
    age: u8 = 0;          // NSDMI (in-class initialization)
    active: bool = true;
};
```

#### Classes
```ibex
class Container {
    private:
        data: *i32;
        capacity: u32;
    public:
        init: (cap: u32) -> void;
        push: (value: i32) -> void;
};
```

#### Enums
```ibex
enum Status: u8 {
    Idle = 0,
    Running = 1,
    Paused = 2,
    Done = 3
};

// Enums are scoped (like C++ enum class)
// Automatic decay to underlying type:
s: Status = Status.Running;
val: u8 = s;  // implicit conversion
```

### Type Deduction

```ibex
// Automatic type deduction with :=
x := 42;              // i32
y := 3.14;            // f64
z := true;            // bool

// In function parameters (not supported, must be explicit)
add: (i32, i32) -> i32 {}  // types required

// In blocks
process: (x: i32) -> i32 {
    y := x * 2;       // y is i32 (deduced from x)
    return y;
}
```

### Type Compatibility

- **Implicit conversions**: Only within numeric type families
- **Explicit casts**: Using `as` keyword
  ```ibex
  x: i32 = 42;
  y: i8 = x as i8;
  f: f64 = x as f64;
  ```

### Arrays, Pointers, and Slices

(To be specified)

## Grammar Specification

### Statement Delimiters

Semicolon (`;`) terminates statements:
```ibex
x: i32 = 10;
y := 20;
result := process(x);
```

### Expression Statements

Last expression in a scope can be returned implicitly:
```ibex
compute: (x: i32) -> i32 {
    a := x + 10;
    b := a * 2;
    b                  // implicit return (no semicolon)
}
```

### Control Flow

#### If-Else
```ibex
if x > 10 {
    result := process(x);
} else {
    result := 0;
}
```

#### Loops
```ibex
// For-in loop
for i in 0..10 {
    print(i);
}

// While loop
while condition {
    doSomething();
}
```

#### Blocks as Expressions
```ibex
value: i32 = {
    x := 10;
    y := 20;
    x + y              // implicit return
};
```

## Memory Management

### Manual Memory Management

Ibex uses explicit manual memory management with built-in safety annotations:

```ibex
// Allocation
buffer: *byte = alloc(byte, 1024);

// Deallocate
free(buffer);
```

### Memory Annotations

Functions performing memory operations are marked with explicit annotations:

```ibex
// This function allocates memory
create_buffer: (size: u32) -> *byte [[allocates]] {
    return alloc(byte, size);
}

// This function deallocates memory
destroy_buffer: (buf: *byte) -> void [[deallocates]] {
    free(buf);
}

// Multiple annotations
acquire_resource: () -> *Resource [[allocates, nodiscard]] {
    return alloc(Resource, 1);
}
```

### Annotation Types

- `[[allocates]]` - Function allocates memory (declares new heap objects)
- `[[deallocates]]` - Function deallocates memory
- `[[nodiscard]]` - Return value should not be discarded
- `[[unsafe]]` - Function contains unsafe operations requiring programmer responsibility

### Pointer Operations

```ibex
// Pointers are declared with *
ptr: *i32;

// Dereferencing (if structures support it)
value := *ptr;

// Memory annotations guide safe usage
allocate: (count: u32) -> *i32 [[allocates]] {}
release: (ptr: *i32) -> void [[deallocates]] {}
```

### No RAII or Exception Handling

- Functions manage resources manually using explicit allocation/deallocation
- No exception throwing or catch blocks
- Error handling is explicit (optional return values or error codes)

## Advanced Features

### Universal Function Call Syntax (UFCS)

Any function with type `T` as first parameter can use method-like syntax:

```ibex
// Define function with receiver type
len: (v: Vector) -> u32 {
    return v.size;
}

// Both forms are equivalent:
length1 := len(my_vector);        // traditional function call
length2 := my_vector.len();       // UFCS method call

// Enables fluent chaining
result := data
    .transform()
    .filter()
    .count();
```

### Non-Capturing Lambdas

Only non-capturing lambdas are supported (captures would affect type identity):

```ibex
// Lambda stored in variable
square: (i32) -> i32 = [](x: i32) -> i32 {
    return x * x;
};

// Used like any function
result := square(5);        // 25

// As function parameter
apply: (f: (i32) -> i32, x: i32) -> i32 {
    return f(x);
};

val := apply(square, 5);    // 25
```

Lambdas:
- Capture no state (no closure variables)
- Are typed by signature alone
- Can be stored in variables
- Can be passed as parameters
- Are equivalent to regular functions of same signature

### Blocks as Expression Values

Blocks can be expressions with values:

```ibex
// With explicit return
value1: i32 = {
    x := 10;
    y := 20;
    return x + y;
};

// With implicit return (last expression has no semicolon)
value2: i32 = {
    x := 10;
    y := 20;
    x + y              // implicit return
};

// Block for side effects (no return value)
process: () -> void {
    {
        x := computeExpensive();
        doSomething(x);
    }
}
```

### Type Casting

Type conversion uses the `as` keyword:

```ibex
// Numeric conversions
x: i32 = 42;
y: i8 = x as i8;
f: f64 = x as f64;

// In expressions
result: i64 = ((value as i32) + 10) as i64;

// Enum conversions
status: Status = Status.Running;
code: u8 = status as u8;          // decay to underlying type
restored: Status = code as Status; // explicit conversion
```

### Automatic Type Deduction

Variables with `:=` have types inferred from initialization:

```ibex
x := 42;                           // i32
y := 3.14;                         // f64  
z := true;                         // bool
arr := [1, 2, 3];                  // [3]i32

sum := x + y;                      // type of expression
result := computeValue();          // return type of function
```

**Note**: Function parameters always require explicit types. Type deduction `:=` is only for variable initialization.

## C Interoperability

### C Function Declarations

Ibex can declare and call C functions with C linkage:

```ibex
// Declare external C function
printf: (*byte, ...) -> i32 [[ c_extern ]];

// Call like normal function
printf("Hello from Ibex\n");
```

### C Data Types

C types map to Ibex types:

```ibex
// C int -> i32
// C unsigned int -> u32
// C long -> i64 (on 64-bit systems)
// C void* -> *byte
// C char* -> *byte
// C arrays -> [n]T arrays
```

### Calling C Libraries

```ibex
// Declare C standard library function
malloc: (u32) -> *byte [[ c_extern ]];
free: (*byte) -> void [[ c_extern ]];
memcpy: (*byte, *byte, u32) -> *byte [[ c_extern ]];

// Use them
buffer: *byte = malloc(1024);
memcpy(buffer, other_buffer, 1024);
free(buffer);
```

### Linkage Annotations

- `[[c_extern]]` - Function is defined in C (external linkage)
- `[[c_link]]` - Function uses C calling convention
- `[[packed]]` - Struct/class layout matches C struct (no padding)

## Implementation Model

### Compiler Architecture - High Performance Design

The Ibex compiler is architected for maximum performance using modern low-level design patterns:

#### Core Design Principles

1. **Zero-Copy Data Structures**: All strings use views and spans
2. **No Virtual Functions**: Discriminated unions + switch statements instead of inheritance
3. **Arena Allocation**: Single bump allocator for all AST nodes
4. **Handle-Based References**: 32-bit indices instead of pointers (cache-friendly)
5. **Minimal Allocations**: Typical compilation allocates 2-3 chunks maximum
6. **UTF-8Native**: All strings are UTF-8 with proper Unicode handling via ICU
7. **Standard Library Minimal**: Only use essential C++ features

#### Memory Model

**Arena Allocator**:
- Bump allocator with 64 KB chunks
- All allocations are contiguous (cache locality)
- Zero fragmentation (no free list overhead)
- Reset-based cleanup between compilation units

**String Representation**:
- `Str` type: UTF-8 string view (const char* + length)
- No null termination required
- Stored in arena to avoid string copies
- Spans of strings: `std::span<const Str>`

**Type-Safe Handles**:
- `TypeHandle`, `ExprHandle`, `StmtHandle`, `DeclHandle`
- 32-bit indices instead of pointers
- Automatically bounds-checked in debug builds
- No pointer arithmetic or raw downcasts

#### AST Representation - Discriminated Unions

All AST nodes use `std::variant` (tagged unions) instead of inheritance:

```cpp
// Type expressions
using Type = std::variant<
    PrimitiveType,  // i32, f64, bool, etc.
    PointerType,    // *T
    ArrayType,      // [n]T or [:T] (void)
    NamedType       // Custom types
>;

// Expressions
using Expr = std::variant<
    BinaryExpr,     // a + b
    UnaryExpr,      // -a, !a
    LiteralExpr,    // 42, 3.14, "str", true
    IdentifierExpr, // variable names
    CallExpr,       // func(a, b)
    CastExpr,       // a as type
    MemberExpr,     // obj.field (UFCS)
    IndexExpr,      // arr[i]
    BlockExpr       // { ... } (expression value)
>;

// Statements
using Stmt = std::variant<
    BlockStmt,      // { ... }
    ReturnStmt,     // return expr;
    IfStmt,         // if cond { ... }
    WhileStmt,      // while cond { ... }
    ForStmt,        // for x in range { ... }
    BreakStmt,      // break;
    ContinueStmt,   // continue;
    ExprStmt,       // expr;
    VarDeclStmt     // name: type = value;
>;

// Declarations
using Decl = std::variant<
    FunctionDecl,   // name: (params) -> type { }
    VariableDecl,   // name: type = value;
    StructDecl,     // struct { ... }
    ClassDecl,      // class { ... }
    EnumDecl        // enum: type { ... }
>;
```

#### Pattern Matching

Process AST nodes using `std::visit` with lambda closures:

```cpp
visit_expr(expr, [&](const auto& e) {
    if constexpr (std::is_same_v<decltype(e), const BinaryExpr&>) {
        // Handle binary expression
        process_binary(e.left, e.op, e.right);
    } else if constexpr (std::is_same_v<decltype(e), const CallExpr&>) {
        // Handle call expression
        process_call(e.function, e.arguments);
    }
    // ... other cases
});
```

Or use visitor pattern for complex traversals (see `ast_visitor.h`).

#### Compilation Pipeline

1. **Lexing**: Tokenize source → emit tokens (no string copies)
2. **Parsing**: Parse tokens → build AST in arena
   - Returns handles to nodes in arena
   - All allocations bump-allocated
3. **Semantic Analysis**: Walk AST with visitor pattern
   - Type checking and inference
   - Symbol resolution
   - No intermediate representations
4. **Code Generation**: Emit LLVM IR or machine code
   - Direct from AST via visitors
   - Minimal intermediate data structures
5. **Linking**: Link object files with C/C++ libraries

#### Type Registry

Global type registry (hash map):
- Maps fully qualified names to type definitions
- Enables:
  - Type-safe API bindings
  - Runtime type queries for IDE
  - Program-wide refactoring
  - Function discovery by signature

#### Performance Characteristics

- **Lexing**: O(n) single pass, no string allocations
- **Parsing**: O(n) recursive descent, 2-3 arena chunks typical
- **Compilation**: Linear in AST size, no tree rebalancing
- **Memory Footprint**: Typically 1-3 MB for large programs
- **Compilation Speed**: 100k+ LOC/sec on modern hardware

### Node-Based Computation

Functions are represented as computation nodes in the IDE:

```
Function Node: (i32, i32) -> i32
┌─────────────────────────┐
│  add: (i32, i32) -> i32 │
├─────────────────────────┤
│ Input Pins: [0] i32, [1] i32
│ Output Pins: [0] i32
│ Body: { return a + b; }
└─────────────────────────┘
```

Functions can be linked visually based on type matching.

### Target Output

- **Object Code**: Generates .obj/.o files
- **Linkable**: Can be linked with C/C++ object files
- **ABI**: Follows platform ABI (MSVC/GCC/Clang)
- **Optimization**: LLVM provides O0/O2/O3 optimizations
- **Runtime Support**: Minimal needed (just allocator for Ibex code)
