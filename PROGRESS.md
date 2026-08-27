# Project Progress & Roadmap

## Implemented Features
- **Lexical Analysis & Parsing**: Robust recursive descent parser utilizing handle-based Arena AST.
- **Multi-File Compilation**: Native support for compiling multiple `.ibex` files simultaneously via combined token streams.
- **Data Structures**: Structs, Enums, and Flags (with inheritance and base types).
- **Control Flow**: `if`, `while`, `for`, `break`, `continue`. Const-blocks and Const-modifiers.
- **Strings**: Raw UTF-8 text literals (`html'''<html>'''`) with constant type inference and intrinsic prefix/bytes properties.
- **Type Aliases**: Weak aliases (`using T = i32;`) and `[[strong]]` typedefs.
- **Compile-Time Ops**: `typeof` and `sizeof` operators, and compile-time function bindings (`:= #func()`).
- **Module Architecture**: Fully functional namespace isolation (`package`), export encapsulation (`module`), and resolution (`import mod.pkg as p`).
- **Semantic Analysis Scaffold**: Two-pass scope resolution (`Pre-Pass 1.0` and `Module Resolution 1.5`), immutability enforcement, undefined symbol checking.

## Current Development Roadmap
1. **Semantic Type System Hardening**:
   - Implement deep type equality checks.
   - Enforce `[[strong]]` type barriers on binary operations (requiring explicit `as` casts).
2. **Code Generation (Backend)**:
   - Begin lowering the validated AST into LLVM IR or generating C/C++ backend code.
3. **Standard Library Setup**:
   - Create the fundamental `core` and `std` packages (I/O, basic math, memory allocators).
4. **Attribute Enforcement**:
   - Tie `[[platform("...")]]` directly to parser skipping logic dynamically.
   - Connect `[[deprecated]]` and `[[unused]]` to compiler warning outputs within the Semantic Analyzer pass.

## Upcoming Language Features
1. **Rich Built-in Types**: Native support for tuples, variants, `optional`, and `any`.
2. **Operator Overloading**: Safely limited to comparison, arithmetic, bitwise operations, and the explicit `as` cast operator.
3. **Advanced Branching**: `switch-case` for value-based branching and `match-case` for pattern/type-based matching.
4. **Compile-Time Execution**: Capability to run arbitrary user code securely during the compilation phase.
5. **Reflection & Metaprogramming**: Robust reflection system paired with programmatic code generation.
6. **Literal Suffixes**: Built-in suffixes for numeric/text literals alongside support for user-defined suffixes.
7. **`[[allocator]]` Attribute**: Explicit marking of functions that may allocate or free memory for strict safety auditing.
8. **Enhanced Iteration**: `for-each` loop support featuring built-in indexing and stride controls.
9. **C-Style Variadic Arguments**: For seamless C interoperability and variable-length argument passing.
10. **Versioning System**: First-class support for module versioning and language versioning boundaries.
11. **Inline Assembly**: Direct hardware and register control.
12. **Destructive Move**: Secure, default destructive-move semantics.
13. **Parameterized Types**: Generics/parameterized structural types.

## Broader Goals
- Establish Ibex as a fast, explicit, C-alternative for engine and systems programming.
- Deliver a native IDE Language Server (LSP) leveraging the fast, multi-pass Semantic Analyzer.
