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
