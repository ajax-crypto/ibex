# Ibex Language Specification

## Table of Contents
- [1. Lexical Elements](#1-lexical-elements)
  - [1.1 Keywords](#11-keywords)
  - [1.2 Operators and Precedence](#12-operators-and-precedence)
  - [1.3 Comments and Delimiters](#13-comments-and-delimiters)
- [2. Syntax & Grammar](#2-syntax--grammar)
  - [2.1 Declarations](#21-declarations)
  - [2.2 Statements and Expressions](#22-statements-and-expressions)
  - [2.3 Types](#23-types)
- [3. Semantics](#3-semantics)
  - [3.1 Variable Lifetime & Scoping](#31-variable-lifetime--scoping)
  - [3.2 Control Flow](#32-control-flow)
  - [3.3 Modifiers & Immutability](#33-modifiers--immutability)
  - [3.4 Implicit Structural Members](#34-implicit-structural-members)
  - [3.5 Attributes](#35-attributes)
  - [3.6 Packages & Modules](#36-packages--modules)
  - [3.7 Initialization and Function Calls](#37-initialization-and-function-calls)
  - [3.8 Inheritance and Composition](#38-inheritance-and-composition)
  - [3.9 Uniform Function Call Syntax (UFCS)](#39-uniform-function-call-syntax-ufcs)
  - [3.10 Circular Dependencies](#310-circular-dependencies)
  - [3.11 Parameterized Modules](#311-parameterized-modules)
  - [3.12 Compiler Flags & Environment Variables](#312-compiler-flags--environment-variables)
  - [3.13 Numeric Literals](#313-numeric-literals)
  - [3.14 Inline Functions (Lambda Expressions)](#314-inline-functions-lambda-expressions)

---

## 1. Lexical Elements

### 1.1 Keywords
The following keywords are reserved and cannot be used as identifiers:
`return`, `if`, `else`, `while`, `for`, `in`, `break`, `continue`, `struct`, `enum`, `flag`, `using`, `const`, `var`, `static`, `package`, `module`, `export`, `import`, `as`, `sizeof`, `typeof`, `true`, `false`, `null`

Primitive type keywords:
`i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `f32`, `f64`, `bool`, `text`, `byte` (alias for `u8`)

### 1.2 Operators and Precedence
Operators are listed from highest to lowest precedence.

| Precedence | Operator | Description | Arity |
| 1 | `()` `[]` `.` `?` | Grouping, Indexing, Member Access, Optional Unwrap | Binary/NA/Postfix |
| 2 | `!` `~` `-` `@` `&` `*` `sizeof` `typeof` | Logical NOT, Bitwise NOT, Negation, Address-of, Dereference, Size/Type | Unary |
| 3 | `as` | Type Cast | Binary |
| 4 | `*` `/` `%` | Multiplication, Division, Modulo | Binary |
| 5 | `+` `-` | Addition, Subtraction | Binary |
| 6 | `<<` `>>` | Bitwise Left/Right Shift | Binary |
| 7 | `<` `<=` `>` `>=` | Relational Comparisons | Binary |
| 8 | `==` `!=` | Equality Comparisons | Binary |
| 9 | `&` | Bitwise AND | Binary |
| 10 | `^` | Bitwise XOR | Binary |
| 11 | `\|` | Bitwise OR | Binary |
| 12 | `&&` | Logical AND | Binary |
| 13 | `\|\|` | Logical OR | Binary |
| 14 | `or` | Null Coalescing | Binary |
| 15 | `=` `:=` `+=` `-=` `*=` `/=` | Assignment and Compound Assignment | Binary |

### 1.3 Comments and Delimiters
Ibex uses strict structural delimiters to ensure unambiguous parsing:
- **Statement Delimiter**: All individual statements (declarations, assignments, expression evaluations, returns) must be terminated by a semicolon `;`.
- **Block Delimiters**: Logical scopes (e.g., function bodies, packages, structs, loops, conditionals) are wrapped in curly braces `{` and `}`. Control flow statements like `if`, `for`, or `while` *require* braces; single-statement un-braced bodies are not permitted.
- **Compile-Time Target Delimiter**: The hash/pound symbol `#` is a specialized delimiter used to denote compile-time symbol resolution, primarily utilized in function binding declarations.

**Comments**:
Comments are ignored by the parser and can be placed anywhere whitespace is valid.
- **Single-line**: `//` ignores all characters until a newline (`\n`) or EOF is reached.
- **Multi-line**: `/*` begins a block comment, and `*/` closes it. Multi-line comments can span multiple lines.

```ebnf
SingleLineComment ::= "//" { AnyCharExceptNewline } ( "\n" | EOF )
MultiLineComment  ::= "/*" { AnyChar } "*/"
```

## 2. Syntax & Grammar
*Note: Represented in pseudo-EBNF.*

### 2.1 Declarations
```ebnf
Program ::= { Decl }
Decl ::= VarDecl | FuncDecl | StructDecl | EnumDecl | FlagDecl 
       | PackageDecl | ModuleDecl | ImportDecl | TypeAliasDecl | UsingDecl

AttributeList ::= "[[" Attribute { "," Attribute } "]]"
Attribute ::= Identifier [ "(" Expr ")" ]

FuncDecl ::= [AttributeList] Identifier ":" "(" [ParamList] ")" [ "->" Type ] BlockStmt
ParamList ::= Param { "," Param }
Param ::= [AttributeList] Identifier ":" Type

StructDecl ::= [AttributeList] "struct" Identifier [ ":" BaseList ] "{" { StructMember } "}"
StructMember ::= Identifier ":" Type [ "=" Expr ] ";"
BaseList ::= Identifier { "," Identifier }

EnumDecl ::= [AttributeList] "enum" Identifier [ ":" Type ] "{" [ EnumMemberList ] "}"
EnumMemberList ::= EnumMember { "," EnumMember }
EnumMember ::= Identifier [ "=" Expr ]

FlagDecl ::= [AttributeList] "flag" Identifier [ ":" Type ] "{" [ FlagMemberList ] "}"
FlagMemberList ::= FlagMember { "," FlagMember }
FlagMember ::= Identifier [ "=" Expr ]

VarDecl ::= ["const" | "var"] Identifier [ ":" Type ] [ ( "=" | ":=" ) Expr ] ";"
PackageDecl ::= "package" Identifier "{" { Decl } "}"
ModuleDecl ::= "module" Identifier [ "(" ModuleParamList ")" ] ";"
ModuleParamList ::= ModuleParam { "," ModuleParam }
ModuleParam ::= Identifier ":" PrimitiveType
ExportDecl ::= "export" "package" Identifier { "," Identifier } ";"
ImportDecl ::= "import" Identifier [ "(" ExprList ")" ] "." ( Identifier | "*" ) [ "as" Identifier ] ";"

TypeAliasDecl ::= [AttributeList] "using" Identifier "=" Type ";"
UsingDecl ::= [AttributeList] "using" Identifier ":=" "#" Identifier "(" [BindingArgList] ")" ";"
BindingArgList ::= BindingArg { "," BindingArg }
BindingArg ::= [ Expr ]  /* Omitted Expr indicates an unbound parameter slot */

NumericSuffix ::= "i8" | "i16" | "i32" | "i64" | "u8" | "u16" | "u32" | "u64" | "u" | "f32" | "f64"
IntegerLiteral ::= Digits [ NumericSuffix ]
FloatLiteral ::= Digits "." Digits [ NumericSuffix ]
```

### 2.2 Statements and Expressions
```ebnf
Stmt ::= BlockStmt | IfStmt | WhileStmt | ForStmt | ReturnStmt | VarDeclStmt | ExprStmt | ConstModStmt | ConstBlockStmt
BlockStmt ::= "{" { Stmt } "}"

IfStmt ::= "if" Expr BlockStmt [ "else" ( IfStmt | BlockStmt ) ]
WhileStmt ::= "while" Expr BlockStmt [ "else" BlockStmt ]
ForStmt ::= "for" Identifier "in" Expr BlockStmt

ConstModStmt ::= "const" "(" IdentifierList ")" ";"
ConstBlockStmt ::= "const" "(" IdentifierList ")" BlockStmt
IdentifierList ::= Identifier { "," Identifier }

ReturnStmt ::= "return" [ Expr ] ";"
BreakStmt ::= "break" ";"
ContinueStmt ::= "continue" ";"

Expr ::= BinaryExpr | UnaryExpr | PrimaryExpr | CastExpr | MemberExpr | IndexExpr | CallExpr | SizeofExpr | StructInitExpr | ArrayLiteralExpr | UnwrapExpr
UnwrapExpr ::= Expr "?"
CallExpr ::= Expr "(" [ CallArgList ] ")"
CallArgList ::= CallArg { "," CallArg }
CallArg ::= [ Identifier "=" ] Expr

StructInitExpr ::= Identifier "{" [ StructInitList ] "}"
StructInitList ::= StructFieldInit { "," StructFieldInit }
StructFieldInit ::= [ Identifier ( ":" | "=" ) ] Expr

ArrayLiteralExpr ::= "[" [ExprList] "]"
TupleExpr ::= "(" [ ExprList ] ")"

SizeofExpr ::= "sizeof" "(" (Type | Expr) ")"
TypeofExpr ::= "typeof" "(" Expr ")"
```

### 2.3 Types
```ebnf
Type ::= PrimitiveType | NamedType | PointerType | ReferenceType | ArrayType | SliceType | TupleType | OptionalType | VariantType | TypeofExpr
ArrayType ::= "[" Expr "]" Type
SliceType ::= "[:" Type "]"
PointerType ::= "*" Type
ReferenceType ::= "&" Type
TupleType ::= "(" [ TypeList ] ")"
OptionalType ::= Type "?"
VariantType ::= "(" Type "+" [ VariantTypeList ] ")"
VariantTypeList ::= Type { "+" Type }
TypeList ::= Type { "," Type }
```

## 3. Semantics

### 3.1 Variable Lifetime & Scoping
- **Block Scope**: Variables declared inside `{}` are inaccessible outside.
- **Static Duration**: `static` variables persist for the duration of the program.

### 3.2 Control Flow
- **If Statements**: Parentheses around the condition are not required. Execution branches based on the expression.
- **Loops (`while`, `for`)**: 
  - Iteration blocks require braces `{}`. 
  - `break;` immediately terminates the innermost enclosing loop block.
  - `continue;` immediately halts the current iteration, skipping the remaining block, and proceeds to the next evaluation/iteration phase.
  - `while` loops support an optional `else` block which executes if the loop terminates normally (without hitting a `break`).

### 3.3 Modifiers & Immutability
- **`const` Keyword**: Variables marked const cannot be mutated. Compile-time literal assignments automatically infer `const`.
- **Const Modifiers**: You can mark existing variables as immutable dynamically using `const(var1, var2);`. They become immutable from that statement forward in the current scope.
- **Const Blocks**: You can mark existing variables as immutable strictly for the duration of a specific block using `const(var1, var2) { ... }`. Once the block exits, mutability returns to its prior state.

### 3.4 Implicit Structural Members
- **Arrays & Slices**: All arrays (`[N]T`) and slices (`[:T]`) intrinsically possess a read-only `.size` member which returns the element count. For explicit arrays, this is a compile-time constant.
- **Tuples**: Collections of values of potentially different types, e.g., `(i32, f64, text)`. Elements are accessed via 0-based compile-time constant indices (e.g., `x[1]`). The type of `x[1]` is deduced at compile time (`fp64`). Tuples have a read-only `.size` member which is a compile-time constant. Empty tuples `()` are valid.
- **`text` Type**: Strings are implicit UTF-8 byte arrays. The `text` type exposes intrinsic properties:
  - `.prefix`: A `text` slice pointing strictly to the prefix string literal (e.g. `html` in `html'''...'''`).
  - `.bytes`: Exposes the underlying `u8` array containing the raw string data.
  - `.bytes.size`: Returns the byte-count of the UTF-8 text string as a compile-time constant.

### 3.5 Attributes
Applied via `[[name(args)]]`.
- `[[strong]]`: Used on aliases. Creates a strict type boundary prohibiting implicit mix with base types. Requires `as` operator to cast.
- `[[unused]]`: Suppresses unused symbol warnings on functions or variables.
- `[[deprecated("reason")]]`: Triggers compiler warnings on reference/invocation.
- `[[platform("win32"|"linux"|"mac")]]`: Conditionally parses blocks or functions based on target architecture, skipping parse completely on un-matching platforms.
- `[[discard]]`: Discards the return value implicitly.

### 3.6 Packages & Modules
- **Packages** are block-level declarations (`package name { ... }`) that isolate namespaces. A single file can contain multiple package blocks with different names. If multiple package blocks share the same name within the same file, they are automatically merged into a single package scope (a warning is emitted). Packages can also be split across multiple files; they will be consolidated during compilation (a warning is emitted). Identical symbol names with the same signature inside a merged package result in a hard compilation error (name collision).
- **Modules** are single-file definitions (`.module.ibex`) that distribute Packages and can hold their own `const` fields, `using` aliases, and `[[attributes]]` like `[[deprecated]]`. Modules cannot contain logic or struct definitions. 
- **Directory Layout**: Each module and its associated package `.ibex` files must reside in their own subdirectory. The compiler assigns all non-module `.ibex` files in a module's directory to that module for dependency analysis. Consumer files that import the module must be placed outside the module's directory to avoid false circular dependency errors.
- **Exporting Packages**: A module explicitly exports a package using `export package name;`.
- **Importing**:
  - `import mod.pkg;` imports a specific package. It is accessed via `mod.pkg.symbol` or simply `pkg.symbol`.
  - `import mod.*;` imports all exported packages from a module.
  - `import mod.pkg as p;` binds the package namespace strictly to `p` (e.g., `p.symbol`).

### 3.7 Initialization and Function Calls
- **Named Arguments**: Function calls support explicit parameter binding via named arguments using the `=` operator (e.g., `func(name = val)`). This allows parameters to be passed out-of-order and provides self-documenting call sites.
- **Designated Initializers**: Struct instantiation supports designated initializers mapping fields to expressions. Both `:` and `=` operators are valid mapping delimiters (e.g., `Point { x: 10, y: 20 }` or `Point { x = 10, y = 20 }`). Positional initialization is also supported by omitting the field name.

### 3.8 Inheritance and Composition
- **Struct Composition**: `struct Derived : Base` embeds the fields of `Base` directly into `Derived` in memory, exactly as if they were declared manually in order. There are no virtual tables (v-tables) or dynamic dispatch overheads. Multiple bases (`struct Derived : B1, B2`) embed fields sequentially in declaration order.
- **Enum and Flag Extension**: An `enum` or `flag` declared with a named base type (e.g., `enum Ext : BaseEnum`) strictly inherits all enumerators from `BaseEnum` and explicitly shares its underlying primitive type footprint. 

### 3.9 Uniform Function Call Syntax (UFCS)
- **Syntactic Rewrite**: The member access call expression `obj.func(args...)` is strictly syntactically equivalent and intrinsically rewritten by the parser to `func(obj, args...)`. 
- **Object Resolution**: Because of UFCS, Ibex does not have traditional "methods" bound inside structs. Any standalone function can be called via method-syntax on an object so long as that function explicitly takes the object type as its first parameter.

### 3.10 Circular Dependencies
- The compiler detects circular module dependencies during the module scanning phase, before parsing begins.
- **Self-circular**: A module whose packages import from itself is detected and rejected.
- **Transitive cycles**: If module A depends on B, and B depends on A (directly or transitively), the compiler emits: `"Circular dependency detected: A -> B -> A"`.

### 3.11 Parameterized Modules
- Modules may declare typed parameters: `module config(debug: bool, max_size: i32);`
- All parameter types must be primitive types (`bool`, `i32`, `i64`, `f32`, `f64`, `u8`, `u16`, `u32`, `u64`, `text`).
- All arguments at the import site must be compile-time literals or constants. No runtime values.
- Parameterized module imports **must** use `as` alias: `import config(true, 256).* as cfg;` (error without alias).
- Parameters can control conditional `export package` statements inside `.module.ibex` files.
- Multiple imports of the same parameterized module with different arguments create separate module instances.

### 3.12 Compiler Flags & Environment Variables
- `--import <path>`: Add a non-recursive module search path.
- `--import-recursive <path>`: Add a recursive module search path.
- `IBEX_MODULE_PATH`: Environment variable with semicolon-separated module search paths.
- `IBEX_MODULE_RECURSE=1`: Environment variable to enable recursive search for paths from `IBEX_MODULE_PATH`.

### 3.13 Numeric Literals

#### Type Suffixes
Numeric literals may carry an explicit type suffix to specify their precision:

| Suffix | Type | Example |
|--------|------|---------|
| `i8` | 8-bit signed integer | `42i8` |
| `i16` | 16-bit signed integer | `1000i16` |
| `i32` | 32-bit signed integer | `42i32` (default for integers) |
| `i64` | 64-bit signed integer | `100000i64` |
| `u8` | 8-bit unsigned integer | `255u8` |
| `u16` | 16-bit unsigned integer | `65535u16` |
| `u32` | 32-bit unsigned integer | `1000u32` |
| `u64` | 64-bit unsigned integer | `1u64` |
| `u` | 64-bit unsigned (shorthand) | `1u` |
| `f32` | 32-bit float | `3.14f32` (default for floats) |
| `f64` | 64-bit float | `3.14f64` |

#### Default Inference
- Unsuffixed integer literals default to `i32`.
- Unsuffixed floating-point literals default to `f32`.

#### Overflow/Underflow
- The compiler validates that literal values fit within their declared (or inferred) type range.
- `256u8` → error: overflows u8 (range: 0 to 255)
- `128i8` → error: overflows i8 (range: -128 to 127)
- Compile-time constant evaluation also checks for overflow/underflow in arithmetic and division by zero.
- Integer division by zero is a compile-time error.
- Floating-point division by zero produces `±INF` per IEEE 754.

#### Implicit Type Properties
All numeric primitive types expose compile-time properties via dot syntax:

| Property | Available On | Description |
|----------|-------------|-------------|
| `.min` | All numeric types | Minimum representable value |
| `.max` | All numeric types | Maximum representable value |
| `.infinity` | `f32`, `f64` | Positive infinity |
| `.nan` | `f32`, `f64` | Quiet NaN |
| `.signaling_nan` | `f32`, `f64` | Signaling NaN |
| `.epsilon` | `f32`, `f64` | Machine epsilon |

Accessing float-only properties on integer types is a compilation error.

#### Reserved Types
The following type keywords are reserved for future use: `bf16`, `fp16`, `fp8`, `fp4`. Using them in declarations produces a compilation error.

### 3.14 Inline Functions (Lambda Expressions)

#### Grammar
```
LambdaExpr ::= "(" [ LambdaParam { "," LambdaParam } ] ")" [ "->" Type ] "{" Statement* "}"
LambdaParam ::= Identifier ":" Type
```

#### Semantics
- Lambda expressions create anonymous functions that can be immediately invoked (IIFE) or assigned to variables.
- Parameters are typed with the same syntax as function parameters: `name: Type`.
- The return type (`-> Type`) is optional. When omitted, the lambda returns `void`.
- Lambda body follows the same rules as function bodies.
- When immediately invoked, the lambda is wrapped in parentheses and called: `((params) -> RetType { body })(args)`.
- When assigned to a variable, the inferred type is a function type matching the lambda's signature.

