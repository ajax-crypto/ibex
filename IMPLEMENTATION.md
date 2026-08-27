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
