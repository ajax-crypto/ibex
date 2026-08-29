# Project Progress & Roadmap

## Implemented Features
- [x] **Lexical Analysis & Parsing**: Robust recursive descent parser utilizing handle-based Arena AST.
- [x] **Multi-File Compilation**: Native support for compiling multiple `.ibex` files simultaneously via combined token streams.
- [x] **Data Structures**: Structs, Enums, and Flags (with inheritance and base types).
- [x] **Control Flow**: `if`, `while`, `for` (with range and collection iteration, rejects scalars), `break`, `continue`. `switch-case` supports integer and string literal matching. Const-blocks and Const-modifiers.
- [x] **Strings**: Raw UTF-8 text literals (`html'''<html>'''`) with constant type inference and intrinsic prefix/bytes properties.
- [x] **Type Aliases**: Weak aliases (`using T = i32;`) and `[[strong]]` typedefs.
- [x] **Compile-Time Ops**: `typeof` and `sizeof` operators, and compile-time function bindings (`:= #func()`).
- [x] **Module Architecture**: Fully functional namespace isolation (`package`), export encapsulation (`module`), and resolution (`import mod.pkg as p`).
- [x] **Parameterized Modules**: Support for passing configuration arguments to modules at compile-time, with `#param_name` access inside packages.
- [x] **Semantic Analysis Scaffold**: Two-pass scope resolution (`Pre-Pass 1.0` and `Module Resolution 1.5`), immutability enforcement, undefined symbol checking.
- [x] **C Foreign Function Interface (FFI)**: Complete FFI integration with automatic C type mapping, header preprocessing, and compile-time validation. Language blocks (`[[language="c"]]`), FFI calls (`c::function()`), and zero-cost string interoperability via `c_str()` method.

## Current Development Roadmap
- [ ] **Semantic Type System Hardening**:
   - [ ] Implement deep type equality checks.
   - [ ] Enforce `[[strong]]` type barriers on binary operations (requiring explicit `as` casts).
- [ ] **Code Generation (Backend)**:
   - [ ] Begin lowering the validated AST into LLVM IR or generating C/C++ backend code.
- [ ] **Standard Library Setup**:
   - [ ] Create the fundamental `core` and `std` packages (I/O, basic math, memory allocators).
- [x] **Attribute Enforcement**:
   - [ ] Tie `[[platform("...")]]` directly to parser skipping logic dynamically.
   - [x] Connect `[[deprecated]]` and `[[unused]]` to compiler warning outputs within the Semantic Analyzer pass.

## Upcoming Language Features
- [x] **Rich Built-in Types**: Native support for tuples, variants, `optional`, and `any`. (Partially implemented in AST & Semantics)
- [ ] **Operator Overloading**: Safely limited to comparison, arithmetic, bitwise operations, and the explicit `as` cast operator.
- [ ] **Advanced Branching**: `match-case` for pattern/type-based matching.
- [x] **Compile-Time Execution**: Capability to run arbitrary user code securely during the compilation phase. (Partially via `ConstExprEvaluator`)
- [ ] **Reflection & Metaprogramming**: Robust reflection system paired with programmatic code generation.
- [x] **Literal Suffixes**: Built-in suffixes for numeric/text literals alongside support for user-defined suffixes. (Numeric suffixes implemented)
- [x] **`[[allocator]]` Attribute**: Explicit marking of functions that may allocate or free memory for strict safety auditing. (Basic FFI support via C headers)
- [x] **Enhanced Iteration**: `for-each` loop support featuring built-in indexing and stride controls. (Iterating over collections/ranges implemented)
- [x] **C-Style Variadic Arguments**: For seamless C interoperability and variable-length argument passing. (Supported via FFI and C header parsing)
- [ ] **Versioning System**: First-class support for module versioning and language versioning boundaries.
- [ ] **Inline Assembly**: Direct hardware and register control.
- [x] **Destructive Move**: Secure, default destructive-move semantics.
- [ ] **Parameterized Types**: Generics/parameterized structural types.

## Tooling & Ecosystem
- [x] **Tree-sitter Grammar**: Complete language grammar (`grammar.js`) for editor integrations.
- [x] **VS Code Syntax Highlighting**: TextMate grammar and basic extension.
- [ ] **Language Server Protocol (LSP)**: `ibex-lsp` executable providing live semantic diagnostics (like auto-highlighting "use after move"), hover info, and auto-completion in VS Code.

## Broader Goals
- [ ] Establish Ibex as a fast, explicit, C-alternative for engine and systems programming.
- [ ] Deliver a robust package manager for dependency resolution and distribution.
