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
  - Arrays: `[N]T`
  - Slices: `[:T]` (Pointer + Size)
  - Pointers: `*T` (Nullable)
  - References: `&T` (Non-nullable)

## 2. Variables and Constants
Variables are mutable by default unless marked `const`.
```ibex
var x: i32 = 10;
const y := 20; // Type inferred as i32
```
Compile-time strings and constants are implicitly inferred as `const`.

## 3. Functions & Bindings
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
- `as`: Explicit type casting.

## 9. Comparison with C++
| Feature | Ibex | C++ |
|---------|------|-----|
| Classes/OOP | Removed entirely | `class`, `virtual`, inheritance |
| Namespaces | `package` & `module` system | `namespace` |
| References | `&T` (never null) | `T&` |
| Aliasing | `using T = X` with `[[strong]]` support | `using T = X`, requires wrapper structs for strong types |
| Metaprogramming| Attributes `[[attr]]`, Function Bindings | Templates, Macros, `constexpr` |


## 5. Modules and Packages

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

```
project/
├── math/                    # Module directory
│   ├── math.module.ibex     # module math; export package geom, calc;
│   ├── geom.ibex            # package geom { ... }
│   └── calc.ibex            # package calc { ... }
├── main.ibex                # import math.geom; import math.calc;
└── ...
```


## 6. Numeric Literals & Type Properties

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

## 7. Circular Dependencies

The compiler detects circular module dependencies and emits clear error messages:

```
Circular dependency detected: a -> b -> a
```

This occurs when module `a`'s packages import from module `b`, and module `b`'s packages import from module `a`.

## 8. Parameterized Modules

Modules can accept compile-time parameters:

```ibex
// config.module.ibex
module config(debug: bool, max_size: i32);

export package utils;
// Conditional exports based on parameters (planned)
// if debug { export package debug_tools; }

const MAX_SIZE: i32 = max_size;
```

Importing a parameterized module requires providing arguments and an alias:

```ibex
package app {
    import config(true, 256).* as cfg;
    // Now use cfg.utils.symbol or cfg.MAX_SIZE
}
```

All arguments must be compile-time constants (literals or `const` variables). Omitting the `as` alias on a parameterized import is a compilation error.

## 9. Compiler Flags

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