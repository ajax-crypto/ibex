# Compiler Architecture & Internal Details

The Ibex compiler (`ibexc`) is written in C++20. It uses a custom-built, arena-allocated AST and a multi-pass analysis pipeline to ensure memory safety and compilation speed.

## Table of Contents
- [1. Core Components](#1-core-components)
  - [1.1 Lexer (lexer.cpp)](#11-lexer-lexercpp)
  - [1.2 Parser (parser_new.cpp)](#12-parser-parser_newcpp)
  - [1.3 Abstract Syntax Tree (ast_new.h)](#13-abstract-syntax-tree-ast_newh)
  - [1.4 Semantic Analyzer (semantic_analyzer.cpp)](#14-semantic-analyzer-semantic_analyzercpp)
  - [1.5 Type Registry (type_registry.h)](#15-type-registry-type_registryh)
- [2. C++ Coding Standards](#2-c-coding-standards)
  - [2.1 Allowed Features (USE)](#21-allowed-features-use)
  - [2.2 Banned Features (DO NOT USE)](#22-banned-features-do-not-use)
- [Circular Dependency Detection](#circular-dependency-detection)
- [Constant Expression Evaluator](#constant-expression-evaluator)
- [Parameterized Modules](#parameterized-modules)

---

## 1. Core Components

### 1.1 Lexer (`lexer.cpp`)
Scans raw source text into a token stream (`Token`). It handles keywords, operators, and primitive literals (integers, booleans, and raw strings like `html'''...'''`). 

### 1.2 Parser (`parser_new.cpp`)
A hand-written recursive descent parser that constructs the Abstract Syntax Tree (AST).
- **Arena Allocation**: AST nodes are allocated linearly in an `Arena` to maximize cache locality and prevent memory leaks.
- **Handle System**: Nodes reference each other via strongly-typed handles (`ExprHandle`, `StmtHandle`, `DeclHandle`, `TypeHandle`) rather than raw pointers.
- **Multi-File Parsing**: The compiler driver dynamically concatenates token streams from multiple files, stripping intermediate EOFs, allowing the parser to construct a single unified `Program` tree seamlessly.

### 1.3 Abstract Syntax Tree (`ast_new.h`)
The AST utilizes `std::variant` based discriminated unions to represent:
- **Decl**: Functions, Variables, Structs, Enums, Packages, Modules, Type Aliases.
- **Stmt**: Blocks, If/While/For loops, Returns, Variable Declarations.
- **Expr**: Binary/Unary ops, Function calls, Member access, `sizeof`, `typeof`.
- **Type**: Primitives, Pointers, References, Arrays, Slices.

### 1.4 Semantic Analyzer (`semantic_analyzer.cpp`)
Performs scope resolution, symbol tracking, and mutability checks. It uses a Multi-Pass architecture:
- **Pass 1.0 (Pre-Pass)**: Scans all declarations, building the global scope and resolving top-level structures (Functions, Structs).
- **Pass 1.5 (Module Resolution)**: Catalogues `PackageDecl` namespaces and maps `ModuleDecl` exports, building the global package and module registry.
- **Pass 2.0 (Main Pass)**: Recursively visits AST nodes (`ExprVisitor`, `StmtVisitor`, `DeclVisitor`), enforcing mutability rules (e.g. `const`), resolving aliased imports, and verifying undefined symbols.

### 1.5 Type Registry (`type_registry.h`)
Maintains a runtime catalog of user-defined types (Structs, Enums, Flags, Aliases) mapping them to unique identifiers, managing memory offsets and bases.

## 2. C++ Coding Standards

The compiler is engineered for **blazing-fast compilation speed** (both for parsing Ibex code and building the compiler itself) and strict runtime performance. To achieve this, we adhere to a very specific subset of C++20.

### 2.1 Allowed Features (USE)
- **Structs and Classes**: Standard flat records without inheritance.
- **Discriminated Unions**: Leverage `std::variant` for polymorphism instead of virtual base classes.
- **Tuples**: Utilize `std::tuple` for cohesive multi-value representations.
- **Trivially Copyable Types**: Design types to be easily movable and copyable (`memcpy` friendly).
- **Modern Views**: `std::string_view` and `std::span` are mandatory for non-owning string and array views to eliminate copying overhead.
- **Arena Allocation**: Prefer linear monotonic arenas for bulk memory management (e.g., AST nodes, Strings).
- **Data-Oriented Design**: Data must be stored in CPU cache-friendly formats (flat, contiguous arrays/vectors).

### 2.2 Banned Features (DO NOT USE)
- **Inheritance & Polymorphism**: No deep inheritance hierarchies. No `virtual` functions. 
- **Complex Metaprogramming**: No SFINAE tricks or excessively heavy template instantiations that degrade compiler build times.
- **Manual/Heap Allocations**: Avoid local small allocations. Do not use raw `new`/`delete`.
- **Smart Pointers**: No `std::shared_ptr` or reference-counting paradigms.
- **Pointer-Chasing**: Avoid fragmented structures (e.g., linked lists, trees built with distributed node pointers). Use Handles/Indices referencing contiguous arrays.
- **Heavy Standard Libraries**: Avoid the `<ranges>` library as it heavily inflates compile times.

## Circular Dependency Detection

After `ModuleScanner` discovers all `.module.ibex` files, it performs a lightweight lex of each module's `.ibex` package files to extract `import` token patterns. These are assembled into a directed dependency graph (`module -> {dependencies}`). A DFS with 3-color marking (white/gray/black) detects back edges, which indicate cycles. Self-cycles (a module importing itself) are caught during graph construction.

## Constant Expression Evaluator

`ConstExprEvaluator` (`const_eval.h/.cpp`) evaluates AST `ExprHandle` nodes to `ConstValue` (variant of `int64_t`, `double`, `bool`, `std::string`). Supports literal evaluation, named constant lookup, binary operations (`+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`), unary operations (`-`, `!`), and string concatenation. Used for parameterized module argument validation and conditional export evaluation.

## Parameterized Modules

`ModuleDecl` carries a `std::vector<ModuleParam>` for typed parameters. `ImportDecl` carries `std::vector<ExprHandle>` for compile-time arguments. The parser enforces that parameterized imports use `as` aliases. `ModuleScanner::evaluate_conditional_exports()` binds parameter values and re-evaluates export declarations to determine which packages are available for a given parameterization. `SemanticAnalyzer` tracks parameterized argument states and folds `#param_name` expressions inside packages into their concrete literal values via `ConstExprEvaluator`.

## C Foreign Function Interface (FFI)

The Ibex compiler integrates C interoperability through a three-stage pipeline: parsing foreign blocks, preprocessing C code, and validating FFI calls.

### Architecture

#### Stage 1: Foreign Block Parsing

The parser (`parser.cpp`) recognizes `[[language="c"]]` attribute blocks:

```ibex
[[language("c")]] {
    #include <stdlib.h>
    int add(int a, int b);
}
```

- **AST Node**: `ForeignBlockDecl` contains the language tag and raw C code
- **Parsing**: Code between `{` and `}` is captured preserving line/column information
- **Multi-block Support**: Multiple foreign blocks are supported in a single file

#### Stage 2: C Header Preprocessing (FFIParser)

`FFIParser` (`ffi_parser.h/cpp`) processes foreign blocks in semantic analysis:

1. **Concatenation**: All `[[language="c"]]` blocks are concatenated into a single C source file
2. **Preprocessing**: The C compiler is invoked with `-E` flag to expand macros and includes:
   ```bash
   <c-compiler> -E -I<include> -D<macro> ibex_ffi_temp.c
   ```
3. **Tree-Sitter Parsing**: The preprocessed output is parsed using `tree-sitter-c` grammar
4. **Type Extraction**: Function declarators are traversed to extract:
   - Function name
   - Parameter types (with pointer detection)
   - Return type (with pointer detection)

#### Stage 3: Type Mapping

The `map_c_type()` function translates C types to Ibex `TypeHandle`s:

| C Type | Ibex Type | Represented As |
|--------|-----------|----------------|
| `int`, `int32_t` | `i32` | `PrimitiveType{I32}` |
| `float` | `f32` | `PrimitiveType{F32}` |
| `double` | `f64` | `PrimitiveType{F64}` |
| `char`, `void` | `byte` | `PrimitiveType{BYTE}` |
| `T*` | `*T` | `PointerType{base_handle}` |

Limitations:
- Struct types are not yet supported (mapped to `byte*` fallback)
- Variadic signatures are detected but not fully validated
- Function pointers are not currently supported

#### Stage 4: FFI Call Validation

During semantic analysis (`SemanticAnalyzer::visit(const CallExpr&)`), when an `FFIAccessExpr` is encountered in a call:

1. **Function Lookup**: Look up the C function name in `ffi_functions_` map
2. **Argument Validation**: 
   - Check argument count matches signature
   - For each argument, verify type compatibility with `are_types_compatible()`
3. **Return Type Assignment**: Set `current_expr_type_` to the mapped return type
4. **Error Reporting**: Type mismatches generate compilation errors

Example validation flow:
```ibex
c::malloc(1024u64)  // ✓ OK: u64 matches malloc's size_t parameter
c::malloc(1024i32)  // ✗ ERROR: i32 incompatible with size_t
c::malloc;          // ✗ ERROR: FFI functions must be called
```

### Configuration (FFIConfig)

CLI arguments populate `FFIConfig` passed to `SemanticAnalyzer`:

```cpp
struct FFIConfig {
    std::string c_compiler_path;           // e.g., "gcc", "msvc"
    std::vector<std::string> include_paths; // -I flags
    std::vector<std::string> definitions;   // -D flags
};
```

Fallback behavior:
- If no compiler is configured, foreign blocks are silently ignored
- If C header parsing fails, a warning is issued and all FFI calls assume `i32` return

### String Interoperability

The `c_str()` method on Ibex `text` values returns a `c_string` struct:

```ibex
str := "Hello";
c_str := str.c_str();      // Returns c_string
ptr := c_str.bytes;         // Extract byte* (const byte*)
c::printf("%s\n", c_str);  // c_string can be passed where char* expected
```

Implemented via semantic analysis pattern matching in `CallExpr` visitor (method call on text type).

### Build System Integration

**CMakeLists.txt**:
- Fetches `tree-sitter` and `tree-sitter-c` via `FetchContent`
- Exposes `tree_sitter_SOURCE_DIR` and `tree_sitter_c_SOURCE_DIR`

**src/CMakeLists.txt**:
- Creates `tree_sitter_lib` static library from `lib/src/lib.c` and `src/parser.c`
- Links `tree_sitter_lib` into `ibex_compiler`
- Includes `ffi_parser.cpp` in compiler sources

### Debugging

Enable verbose FFI output by uncommenting debug prints in `ffi_parser.cpp`:
- "Parsing C code with tree-sitter\n"
- Print tree-sitter parsing results

