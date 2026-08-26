# Ibex Programming Language & IDE

A modern systems programming language featuring an integrated development environment with unique visual and textual code representation.

## Project Overview

Ibex is a native, non-garbage-collected programming language that compiles to object code and links with C. Key features include:

1. **Native Compilation**: Compiles to object code, no runtime overhead
2. **C Interoperability**: Direct linking with C libraries and object files
3. **Integrated IDE**: Blend of textual and visual code representation
4. **Type Registry**: Centralized type management enabling program-wide refactoring
5. **Parser-Friendly Syntax**: Avoids the C++ vexing parse problem through explicit keyword-based declarations
6. **No Garbage Collection**: Manual memory management for predictable performance

## Architecture

### Core Components

#### 1. Compiler (`/compiler`, `/src`)
- **Lexer**: Tokenizes source code
- **Parser**: Builds Abstract Syntax Tree (AST)
- **Type Registry**: Manages all program types
- **Semantic Analyzer**: Type checking and validation
- **Code Generator**: Produces object code or intermediate representation

#### 2. IDE (`/ide`)
- **Text Editor**: Function-based code editing
- **Visual Editor**: Call stack and data flow visualization
- **Type Browser**: Type registry interface
- **Integration**: Real-time compilation and refactoring

#### 3. Runtime (`/runtime`)
- **Memory Management**: Allocation/deallocation functions
- **Runtime Support**: Linkage with C runtime libraries

### Documentation

- **[LANGUAGE_DESIGN.md](docs/LANGUAGE_DESIGN.md)**: Complete language specification
- **[IDE_ARCHITECTURE.md](docs/IDE_ARCHITECTURE.md)**: IDE design and features

## Build System

The project uses CMake 3.22+ with support for:
- **MSVC** on Windows
- **GCC** on Linux

### Prerequisites

- C++23-capable compiler (MSVC 2022 or GCC 12+)
- CMake 3.22 or later

### Build Instructions

#### Windows (MSVC)

```bash
mkdir build
cd build
cmake -G "Visual Studio 17 2022" ..
cmake --build . --config Release
```

#### Linux (GCC)

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

### Build Options

```cmake
-DBUILD_IDE=ON          # Build IDE component (default: ON)
-DBUILD_TESTS=ON        # Build test suite (default: ON)
-DBUILD_EXAMPLES=ON     # Build example programs (default: ON)
-DBUILD_SHARED_LIBS=OFF # Link libraries statically (default)
```

## Language Features

### Basic Syntax

```ibex
// Comments start with //

// Function definition with explicit types
func add(i32 a, i32 b) -> i32 {
    return a + b;
}

// Variable declaration
var x: i32 = 10;          // Mutable variable
let y: i32 = 20;          // Constant variable

// Struct definition
struct Point {
    i32 x;
    i32 y;
};

// Calling functions
func main() -> i32 {
    let sum: i32 = add(5, 3);
    return sum;
}
```

### Primitive Types

| Type | Description |
|------|-------------|
| `void` | No value |
| `bool` | Boolean value |
| `i8`, `i16`, `i32`, `i64` | Signed integers |
| `u8`, `u16`, `u32`, `u64` | Unsigned integers |
| `f32`, `f64` | Floating point |

### Pointers & Arrays

```ibex
var ptr: *i32 = &x;              // Pointer to i32
var arr: i32[10];                // Array of 10 i32
var dynArr: *i32 = alloc(i32, 10); // Dynamic array
free(dynArr);                     // Explicit deallocation
```

### C Interoperability

```ibex
// Declare C function
extern "C" func printf(*u8 format, ...) -> i32;

func main() -> i32 {
    printf("Hello, %d\n", 42);
    return 0;
}
```

## Type Registry

The type registry is central to Ibex's design, enabling:

- **Global Type Management**: All types stored in one place
- **Program-Wide Refactoring**: Rename types across entire codebase
- **Type Metadata**: Complete type information available during compilation
- **IDE Integration**: Type browser and refactoring tools

### Type Registry API

```cpp
class TypeRegistry {
  public:
    // Register a new type
    TypeId register_type(const TypeDefinition& def);
    
    // Lookup type information
    const TypeDefinition* lookup(TypeId id) const;
    TypeId lookup_by_name(std::string_view name) const;
    
    // Refactoring support
    void rename_type(const std::string& old_name, const std::string& new_name);
    void update_type(TypeId id, const TypeDefinition& new_def);
    
    // Query types
    std::vector<TypeId> get_all_types() const;
    std::vector<TypeId> get_types_by_category(TypeCategory category) const;
};
```

## IDE Features (Planned)

### Text Editor
- Function-based code editing
- Real-time syntax highlighting
- Type hints and refactoring

### Visual Modes

#### Call Stack View
- Hierarchical function call graph
- Visual function composition
- Parameter flow visualization

#### Data Flow View
- Variable usage tracking
- Memory allocation visualization
- Type dependency analysis

### Type Registry Browser
- Browse all program types
- Refactor types program-wide
- Memory layout visualization

## Avoiding the Vexing Parse Problem

Ibex solves the C++ vexing parse problem through:

1. **Explicit Keywords**: Every declaration starts with a keyword
   ```ibex
   func f() -> i32;     // Function
   var x: i32;          // Variable  
   let c: i32;          // Constant
   typedef T = i32;     // Type alias
   ```

2. **Clear Syntax**: No ambiguous declaration/expression syntax

3. **Consistent Patterns**: Type annotations always follow this form: `name: type`

4. **No Implicit Conversions**: Constructor syntax is explicit

## Project Structure

```
ibex/
├── CMakeLists.txt          # Main build configuration
├── include/                # Public header files
│   ├── lexer.h
│   ├── parser.h
│   ├── ast.h
│   ├── type_registry.h
│   └── ...
├── src/                    # Compiler implementation
│   ├── lexer.cpp
│   ├── parser.cpp
│   ├── ast.cpp
│   ├── type_registry.cpp
│   └── ...
├── compiler/               # Compiler executable
│   ├── CMakeLists.txt
│   └── main.cpp
├── ide/                    # IDE implementation
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── editor.cpp
│   ├── visual_editor.cpp
│   ├── callstack_view.cpp
│   └── dataflow_view.cpp
├── runtime/                # Runtime library
│   ├── CMakeLists.txt
│   ├── runtime.cpp
│   └── memory.cpp
├── tests/                  # Test suite
│   ├── CMakeLists.txt
│   ├── test_lexer.cpp
│   ├── test_parser.cpp
│   └── test_type_registry.cpp
├── examples/               # Example programs
│   └── hello.ibex
└── docs/                   # Documentation
    ├── LANGUAGE_DESIGN.md
    └── IDE_ARCHITECTURE.md
```

## Compilation Pipeline

```
Source Code (.ibex)
    ↓
Lexer → Tokens
    ↓
Parser → AST
    ↓
Type Registry (collect & register types)
    ↓
Semantic Analyzer (type checking)
    ↓
Code Generator (C code or LLVM IR)
    ↓
C Compiler (MSVC/GCC)
    ↓
Object Files (.obj/.o)
    ↓
Linker (with C runtime)
    ↓
Executable
```

## Development Status

### Completed
- [x] Project structure and CMake build system
- [x] Language specification document
- [x] IDE architecture design
- [x] Lexer implementation
- [x] Parser framework
- [x] AST definition
- [x] Type registry system
- [x] Basic header files

### In Progress
- [ ] Complete parser implementation
- [ ] Semantic analyzer
- [ ] Code generator
- [ ] IDE implementation
- [ ] Test suite

### Future
- [ ] Debugger integration
- [ ] Performance profiler
- [ ] Collaborative editing
- [ ] Advanced IDE features

## Getting Started

1. Clone or extract the project
2. Create a build directory: `mkdir build && cd build`
3. Generate build files: `cmake ..`
4. Build the project: `cmake --build . --config Release`
5. Try the compiler: `./bin/ibexc -h`
6. Check out examples in `examples/`

## Contributing

The project is structured to support incremental development:

1. **Core Compiler**: Lexer ✓ → Parser ✓ → Semantic Analysis → Code Gen
2. **Type System**: Registry ✓ → Type Checking → Refactoring
3. **IDE**: Editor → Visual Views → Integration
4. **Runtime**: Memory Management → C Interop

Each component has its own header files and implementation, making it easy to:
- Focus on specific areas
- Write comprehensive tests
- Integrate features incrementally

## Design Philosophy

1. **Simplicity**: Languages that are easy to parse are easier to understand
2. **Explicitness**: Make intent clear through explicit syntax
3. **Zero-Cost**: Abstractions with no runtime overhead
4. **Composability**: Small, reusable units of code
5. **Toolability**: Rich IDE support through type registry

## License

[To be determined]

## References

- **Vexing Parse**: https://en.wikipedia.org/wiki/Most_vexing_parse
- **Type Registry**: Common in modern IDE designs (Visual Studio, IntelliJ, etc.)
- **Call Graph Visualization**: Used in profilers and code analysis tools
- **C Interoperability**: Similar to Rust's FFI

## Contact & Questions

For more information, see the documentation in `/docs` directory.
