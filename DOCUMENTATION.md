# Ibex Language Guide

Ibex is a statically typed, compiled systems language. This document details its features and usage.

## Table of Contents
- [1. Basic Types](#1-basic-types)
- [2. Variables and Constants](#2-variables-and-constants)
- [3. Functions & Bindings](#3-functions--bindings)
- [4. Structs, Enums, and Extensions](#4-structs-enums-and-extensions)
- [5. Uniform Function Call Syntax (UFCS)](#5-uniform-function-call-syntax-ufcs)
- [6. Modules and Packages](#6-modules-and-packages)
- [7. Type Aliasing](#7-type-aliasing)
- [8. Intrinsic Operators](#8-intrinsic-operators)
- [9. Comparison with C++](#9-comparison-with-c)
- [10. Inline Functions (Lambdas)](#10-inline-functions-lambdas)

---

## 1. Basic Types

- **Primitives**: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `f32`, `f64`, `bool`.
- **Text / Strings**: Represented by the `text` type. Implicitly UTF-8 array of `u8` bytes. 
  - Raw literals: `html'''<html>'''`. Exposes `.prefix` and `.bytes` properties. `.bytes.size` is a compile-time constant. Compile-time string declarations infer `const text`.
- **Composite**: 
  - Arrays: `[N]T` e.g., `x: [8]i32 = [1, 2, 3, 4, 5, 6, 7, 8];`. Accessed via `x[index]`. Fixed size, checked at compile time.
  - Slices: `[:]T` (Pointer + Size) e.g., `y := x[1:3];` creates a slice from index 1 to 3. The slice type is denoted by `[:]T` for consistency with arrays.
  - Pointers: `*T` (Nullable)
  - References: `&T` (Non-nullable)
  - Tuples: `(T1, T2, ...)` e.g. `(i32, f64, text) = (1, 2.0, "hello")`. Elements accessed via `x[index]`. Tuple size is available via `.size`. Empty tuples `()` are valid.
  - Optionals: `T?` specifies a type that can hold a value of type `T` or `null`. e.g. `x: i32? = 2;` or `x: i32? = null;`. Use postfix `?` to unwrap the value (`x?`), which traps if null. Use the `or` operator to coalesce (`x? or 0` or just `x or 0`). Calling members on an optional (like `a.member`) is an error—unwrap it first (`a?.member`). Trailing optional parameters in functions can be omitted (defaulting to `null`).
  - Variants: `(T1 + T2 + ...)` specifies a sum type that stores exactly one of the listed candidate types. e.g. `x: (i32 + f64) = 5;`. Must be explicitly initialized. Has implicit compile-time member `.count` and runtime member `.which` indicating the active type (0-indexed). Elements are accessed by manual casting using the `as` operator (e.g., `x as i32`). Single-choice variants are supported via trailing `+` syntax, e.g. `(i32+)`.

## 2. Variables and Constants
Variables are mutable by default unless marked `const`.
```ibex
var x: i32 = 10;
const y := 20; // Type inferred as i32
```
Compile-time strings and constants are implicitly inferred as `const`.

## 3. Operators
- **Arithmetic**: `+`, `-`, `*`, `/`, `%`
- **Bitwise**: `|` (OR), `&` (AND), `^` (XOR), `~` (COMPLEMENT), `<<` (Left Shift), `>>` (Right Shift). Restricted purely to integer types (`u8`, `i32`, etc.).
- **Logical**: `&&`, `||`, `!`
- **Null Coalescing**: `or` (used for unwrapping Optionals)
- **Unwrap**: `?` (postfix unwrapping of Optionals)

## 4. Functions & Bindings
Standard functions:
```ibex
add : (a: i32, b: i32) -> i32 { return a + b; }
```

**Named Arguments**:
Function calls support named arguments out of order using the `=` delimiter.
```ibex
add(b = 20, a = 10);
```

Compile-time partial application (Function Bindings):
```ibex
using add_five := #add(, 5);
```

**Variadic Parameters (C-Style)**:
Ibex supports C-style variadic parameters utilizing the `...` type syntax. They must be the last parameter.
```ibex
print_all: (fmt: text, args:...) -> void {
    if typeof(args[0]) is i32 {
        // ...
    }
}
```
*Note: If the variadic parameter is the only parameter (e.g. `foo: (p:...)`), the compiler automatically injects a hidden `_hidden_va_start` parameter to properly hook into C-style `va_args` ABIs.*


## 4. Pointers and References

Ibex supports both pointers and references for memory access.

### References (`&T`)
- References are created using the `ref` keyword: `y := ref x;` or `y := ref(x);`
- They must always be initialized and there is no implicit conversion from value to reference.
- References are **always const by default** concerning the underlying data. You cannot modify the underlying variable through a reference (`y.mem = 5` is illegal if `y` is a reference).
- However, since const-ness is a property of the variable, the reference itself can be re-bound to another location: `y = ref z;`
- References are auto-dereferenced when their members are accessed.

### Pointers (`*T`)
- Pointers are created using the address-of operator `@`: `p := @x;`
- Pointer dereferencing uses the `*` operator: `*p = 5;`
- Pointers can read and write the underlying variable.
- Const-ness is a property of the variable, not the type. A const pointer `const p := @x;` means `p` cannot point to something else (`p = @y` is illegal), but you CAN mutate the underlying data (`*p = 5` or `p.mem = 5` are legal).
- For pointers to structs, there is no special `->` operator. The standard `.` operator accesses members directly: `p.member`.
- A pointer to an array (`@arr`) evaluates to a pointer to its first element (`*T`).

## 4. Structs, Enums, and Extensions
No OOP classes, virtual tables, or complex class hierarchies. Structs strictly hold data and support structural composition via a colon syntax `:`.
```ibex
struct Point2D { x: i32, y: i32 }
struct Point3D : Point2D { z: i32 } // Inherits x and y structurally
```

Ibex supports designated initializers using either `:` or `=` to map fields to values.
```ibex
const p1 := Point3D{ x: 10, y: 20, z: 30 };
const p2 := Point3D{ z = 30, x = 10, y = 20 }; // Out of order mapping
const p3 := Point3D{ 10, 20, 30 };             // Positional (in order)
```

Enums and Flags support specific underlying primitive types, and can directly extend existing enums or flags.
```ibex
enum BaseColor : u8 { Red = 1, Green = 2 }
enum ExtColor : BaseColor { Blue = 3 } // Inherits Red and Green from BaseColor
```

## 5. Memory Management and Moves

Ibex uses a strict, safe memory model featuring destructive moves, compile-time pinning, and implicit Return Value Optimization (RVO).

- **Copyable Types**: All primitive types, slices, strings (non-owning), and structs comprised of copyable types are implicitly copyable on assignment or function parameter passing.
- **Non-Copyable Types**: Arrays (`[N]T`), and structs explicitly marked with `[[nocopy]]` or containing non-copyable members are not implicitly copyable.

If you want to transfer ownership of a non-copyable type, you must use the `move()` operator. 
```ibex
arr1: [4]i32;
// arr2 := arr1; // Compiler Error: Implicit copy of non-copyable type
arr2 := move(arr1); // OK
// var3 := arr1; // Compiler Error: Use of moved variable
```

### Pinning and Revival
- **Revival**: Moved variables can be revived by re-assigning them to a new value (`arr1 = ...`). 
- **Pinning**: A variable or type can be marked with `[[nomove]]` to prevent it from ever being moved. Attempting to apply `move()` to a pinned variable is a compile-time error. Additionally, `const` variables are automatically pinned.

### Return Value Optimization (RVO)
When returning a local non-copyable variable from a function, the compiler automatically performs RVO. The local variable acts as an implicit move and is evaluated directly into the call-site's memory.
```ibex
make_array: () -> [4]i32 {
    arr: [4]i32;
    arr[0] = 1;
    return arr; // Implicit move (RVO), legal
}
```

## 5. Uniform Function Call Syntax (UFCS)
Any free-standing function can be called as a method on its first argument via the dot `.` syntax. This allows for a fluent, object-oriented-like API without requiring methods to be forcefully bound inside structs.
```ibex
expand : (rect: &Rect, amount: i32) { /* ... */ }

// These are completely identical to the compiler:
expand(&my_rect, 5);
my_rect.expand(5);
```

## 6. Modules and Packages
A **Package** is a namespace containing types, functions, and variables. A **Module** is a collection of exported packages.
```ibex
package networking { 
    export const PORT := 8080;
}
module server_core { 
    export package networking; 
}
// Usage:
import server_core.networking as net;
import server_core.* // Wildcard import
```

## 7. Type Aliasing
```ibex
using WeakAlias = i32;            // Interchangeable with i32
[[strong]] using StrictId = i32;  // Distinct type, requires 'as' cast to mix with i32
```

## 8. Intrinsic Operators
- `sizeof(type_or_expr)`: Returns size in bytes.
- `typeof(expr)`: Returns the type of the expression, usable in declarations.
- `as`: Explicit type casting. Ibex enforces strong structural typing for casts:
  - **Compatible Types**: Casting is allowed between compatible primitive sets: integer to floating point, integer to integer, integer to enum, and integer to flag.
  - **Structural Identity**: User-defined types (such as `struct`, `enum`, or `flag`) can be cast to an entirely unrelated type *only if* both types are structurally identical (i.e. they share the exact same memory layout, field names, and sequence).
  - **Strong Aliases**: Aliases defined with `[[strong]]` require `as` to be mixed with their underlying base type (as they are structurally identical).
  - **Illegal Casts**: Irregular and opaque casts (such as arrays to text, or between structurally mismatched structs) are strictly rejected at compile-time.

## 9. Comparison with C++
| Feature | Ibex | C++ |
|---------|------|-----|
| Classes/OOP | Removed entirely | `class`, `virtual`, inheritance |
| Namespaces | `package` & `module` system | `namespace` |
| References | `&T` (never null) | `T&` |
| Aliasing | `using T = X` with `[[strong]]` support | `using T = X`, requires wrapper structs for strong types |
| Metaprogramming| Attributes `[[attr]]`, Function Bindings | Templates, Macros, `constexpr` |


## 10. Modules and Packages

Ibex organizes code into packages and modules. Packages are defined as blocks, allowing you to scope declarations. A single file can contain multiple packages. If a package with the same name is declared multiple times in the same file or across different files, the compiler merges them into one namespace (emitting a warning to consolidate them).

```ibex
// math.ibex
package geom {
    struct Point { x: f32; y: f32; }
}

package calc {
    add: (a: i32, b: i32) -> i32 { return a + b; }
}
```

Modules are defined in a separate `.module.ibex` file to dictate how packages are exported.

```ibex
// math.module.ibex
module math;

export package geom, calc;
```

You can then import them elsewhere:
```ibex
// main.ibex
package main {
    import math.geom;
    import math.calc;

    main: () -> void {
        p: geom.Point = geom.Point {x: 0, y: 0};
        res: i32 = calc.add(1, 2);
    }
}
```

### Directory Layout

Each module must reside in its own subdirectory alongside its package `.ibex` files. The compiler treats all `.ibex` files in a module's directory as belonging to that module. Files that *consume* the module (i.e., import it) must be placed **outside** the module's directory.

Nested modules are strictly prohibited. You cannot define a `.module.ibex` inside a subdirectory of another module, nor can you define multiple modules in the same directory.

```
project/
├── math/                    # Module directory
│   ├── math.module.ibex     # module math; export package geom, calc;
│   ├── geom.ibex            # package geom { ... }
│   └── calc.ibex            # package calc { ... }
├── main.ibex                # import math.geom; import math.calc;
└── ...
```

### Parameterized Modules

Modules can take compile-time parameters. This is useful for configuring libraries or conditional compilation.

```ibex
// config.module.ibex
module config(debug: bool, max_size: i32);

export package utils;
```

When importing a parameterized module, you must provide constant literal arguments and assign it an alias:

```ibex
package app {
    import config(true, 256).* as cfg;
}
```

#### Struct Parameters
Module parameters can also be user-defined structs, provided that the struct is declared directly inside the `.module.ibex` file. When importing the module, the struct can be initialized directly without needing to import it first:

```ibex
// network.module.ibex
module network(config: NetConfig);

struct NetConfig {
    port: i32;
    secure: bool;
}

export package tcp;
```

```ibex
// client.ibex
package client {
    import network(NetConfig{port: 8080, secure: true}).* as net;
}
```

Inside the module's packages, you can reference the parameters (and their fields) using the `#` prefix. These are evaluated as compile-time constants:

```ibex
// utils.ibex
package utils {
    clamp: (val: i32) -> i32 {
        if val > #max_size { return #max_size; }
        return val;
    }
}
```

## 11. Numeric Literals & Type Properties

### Type Suffixes
Append a type suffix to any numeric literal to explicitly control its precision:

```ibex
package examples {
    main: () -> void {
        a: i8 = 42i8;           // 8-bit signed
        b: u64 = 1u << 56;      // u shorthand for unsigned 64-bit
        c: f64 = 3.14159f64;    // 64-bit float
        d := 100;               // defaults to i32
        e := 3.14;              // defaults to f32
    }
}
```

The compiler checks for overflow:
```ibex
package overflow {
    bad: () -> void {
        x := 256u8;    // Error: overflows u8 (range: 0 to 255)
        y := 128i8;    // Error: overflows i8 (range: -128 to 127)
    }
}
```

### Type Properties
All numeric types expose compile-time properties:

```ibex
package props {
    limits: () -> void {
        max_i32 := i32.max;        // 2147483647
        min_u8  := u8.min;         // 0
        inf     := f64.infinity;   // +Infinity
        eps     := f32.epsilon;    // ~1.19e-7
    }
}
```

## 12. Circular Dependencies

The compiler detects circular module dependencies and emits clear error messages:

```
Circular dependency detected: a -> b -> a
```


This occurs when module `a`'s packages import from module `b`, and module `b`'s packages import from module `a`.

## 13. Compiler Flags

| Flag | Description |
|------|-------------|
| `--import <path>` | Add a module search directory (non-recursive) |
| `--import-recursive <path>` | Add a module search directory (recursive) |
| `IBEX_MODULE_PATH` | Environment variable: semicolon-separated search paths |
| `IBEX_MODULE_RECURSE=1` | Environment variable: enable recursive search for env paths |

## 10. Inline Functions (Lambdas)

Ibex supports anonymous inline functions (lambda expressions). They can be immediately invoked or assigned to variables:

```ibex
package examples {
    demo: () -> void {
        // Immediately Invoked Function Expression (IIFE)
        x := ((y: i32) -> i32 { return y + 1; })(3);  // x is 4

        // Multi-parameter lambda
        sum := ((a: i32, b: i32) -> i32 { return a + b; })(10, 20);  // sum is 30

        // Lambda assigned to a variable
        inc := (x: i32) -> i32 { return x + 1; };
        result := inc(10);  // result is 11

        // Void return type can be omitted
        printer := (msg: text) { return; };
        printer("hello");
    }
}
```
## 14. Control Flow

### Switch Statements
Ibex supports `switch` statements for value-based branching. You can switch on integers as well as string (`text`) literals.

```ibex
package control {
    parse_command: (cmd: text) -> i32 {
        switch cmd {
            case "start": return 1;
            case "stop": return 0;
            case "pause": return 2;
            default: return -1;
        }
    }
}
```

### For Loops
The `for` loop is used to iterate over collections and ranges. The syntax relies on `for item in source { ... }`.
Iterating over a scalar value is a semantic error; the source must be an array, a slice, or a range (e.g., `0..count`).

```ibex
package control {
    loop_example: () -> void {
        // Range iteration
        for i in 0..10 {
            // Iterates from 0 to 9
        }

        // Collection iteration (assuming arr is an array or slice)
        // for item in arr { ... }
    }
}
```


## 9. C FFI (Foreign Function Interface)

Ibex currently targets the C11 standard for its runtime layer, offering zero-cost, explicit native interoperability.

### C Language Blocks
Include C headers dynamically inside Ibex code via the [[language="c"]] block:

`ibex
[[language="c"]] {
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
}
`

### Native Operator (::)
To invoke standard C functions, simply call them with the language prefix and double colon :: operator:
`ibex
var memory = c::malloc(1024);
c::free(memory);
`

### String Interpolation and Pointers
While Ibex maps primitives directly to their C equivalents, 	ext literals require an explicit conversion step to interop with char*.

Use the implicit .c_str() method to spawn a C-compatible null-terminated c_string, and .bytes to access the raw pointer:
`ibex
var greeting = "Hello C";
var c_greeting = greeting.c_str();

c::printf("%s\n", c_greeting); // Implicitly degrades to char*
var raw_pointer = c_greeting.bytes; // const byte*
`
