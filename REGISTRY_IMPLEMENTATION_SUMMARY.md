# Type, Node, and Module Registry Implementation - Summary

Date: April 6, 2026  
Status: Complete

---

## What Was Implemented

### 1. Type Registry System

**File**: `include/type_registry.h`, `src/type_registry.cpp`

Classes and structures for managing type definitions:
- `TypeMember` - Struct/class member with name, type, offset, default value
- `TypeDefinition` - Complete type information with kind (STRUCT, CLASS, ENUM, FLAG, etc.)
- `TypeRegistry` - Central registry for all user-defined types

**Key Features**:
- ✅ Register and lookup types by ID or name
- ✅ Full type member management (add, remove, rename)
- ✅ Memory layout control with byte offsets
- ✅ Support for enum inheritance (`extends` field)
- ✅ Support for struct inheritance (`bases` field)
- ✅ Refactoring operations with impact tracking

### 2. Node and Module System

**File**: `include/node_registry.h`, `src/node_registry.cpp`

Classes for managing functions as computation nodes:
- `NodeParameter` - Function parameter with type reference
- `NodeOutput` - Function return type
- `NodeInvocation` - Represents a function call within a node
- `Node` - Function as a computation node (FUNCTION, EXTERNAL, BUILTIN, LAMBDA)
- `Module` - Collection of interconnected nodes
- `NodeRegistry` - Central registry managing modules and nodes

**Key Features**:
- ✅ Create and manage modules
- ✅ Create and manage function nodes within modules
- ✅ Track function invocations and data flow
- ✅ Node lookup by name and ID
- ✅ Refactoring operations for functions (rename, parameter changes)

### 3. Project Registry

**Class**: `ProjectRegistry`

Unified interface combining Type and Node registries:
- ✅ Global type and struct member renaming
- ✅ Cross-registry refactoring coordination
- ✅ Single file serialization for entire project

### 4. JSON Serialization with yyjson

**Integration**: yyjson 0.12.0 library

Comprehensive JSON support:
- ✅ Serialize type registry to JSON array
- ✅ Deserialize types from JSON
- ✅ Serialize node registry with module hierarchy
- ✅ Deserialize modules and nodes from JSON
- ✅ Combined project JSON with types + nodes
- ✅ File I/O operations (save/load)
- ✅ Automatic ID tracking during serialization

### 5. Refactoring Operations

Powerful refactoring with automatic impact tracking:

**Type Registry**:
- ✅ `rename_type()` - Rename type everywhere (updates references)
- ✅ `rename_member()` - Rename struct member
- ✅ `change_member_type()` - Update member's type
- ✅ `add_member()` - Add new member to struct
- ✅ `remove_member()` - Remove member from struct
- ✅ `update_member_offset()` - Change memory layout

**Node Registry**:
- ✅ `rename_function()` - Rename node with conflict detection
- ✅ `change_parameter_type()` - Update parameter type
- ✅ `change_return_type()` - Update return type
- ✅ `add_parameter()` - Add parameter to function
- ✅ `remove_parameter()` - Remove parameter from function

**Project Registry**:
- ✅ `rename_global_type()` - Rename type across type and node registries
- ✅ `rename_struct_member()` - Rename member with global impact

---

## Refactoring Example: rename_type()

```cpp
// Before
struct Point2D {
    x: f64;
    y: f64;
}

distance: (p: Point2D) -> f64 { ... }

// User wants to rename Point2D → Vector2
registry.rename_type(point_id, "Vector2");

// After (automatic):
// - Type name changed
// - All member.type_name references updated
// - All base type references in inheritance updated
// - All lookups use new name
// - Type registry internal mappings updated
```

When renaming a type:
1. Check for name conflicts
2. Update the type's name
3. Remove old name mapping
4. Add new name mapping
5. Update all references in other types' members
6. Update all base type references

---

## Refactoring Example: rename_member()

```cpp
// Before
struct Point {
    x: f64;  // Want to rename to "horizontal"
    y: f64;
}

// User renames member
registry.rename_member(point_id, "x", "horizontal");

// After:
// - Member renamed
// - Semantic analyzer will update code accessing point.x
```

---

## JSON Schema

### Type Registry

```json
[
  {
    "name": "Point",
    "kind": 0,  // STRUCT=0, CLASS=1, ENUM=2, FLAG=3...
    "id": 1,
    "members": [
      {
        "name": "x",
        "type": "f64",
        "offset": 0,
        "default": null
      }
    ],
    "bases": [],
    "underlying_type": "",
    "extends": null
  }
]
```

### Node Registry

```json
[
  {
    "name": "MainModule",
    "id": 1,
    "nodes": [
      {
        "name": "process",
        "id": 0,
        "kind": 0,  // FUNCTION=0, EXTERNAL=1...
        "unique_id": 1,
        "parameters": [
          {
            "name": "input",
            "type_id": 5,
            "default": null
          }
        ],
        "output": {
          "name": "result",
          "type_id": 6
        },
        "invocations": [
          {
            "target_node_id": 1,
            "argument_index": 0
          }
        ],
        "source_location": "game.ibex:42"
      }
    ]
  }
]
```

---

## CMake Integration

### yyjson Dependency

Updated `CMakeLists.txt`:
```cmake
include(FetchContent)

FetchContent_Declare(
    yyjson
    URL https://github.com/ibireme/yyjson/archive/refs/tags/0.12.0.tar.gz
)
FetchContent_MakeAvailable(yyjson)
```

### Compilation

Updated `src/CMakeLists.txt`:
```cmake
add_library(ibex_compiler
    lexer.cpp
    parser_new.cpp
    token.cpp
    type_registry.cpp      # NEW
    node_registry.cpp      # NEW
)

target_link_libraries(ibex_compiler
    PUBLIC
        yyjson             # NEW
)
```

---

## File Structure

```
include/
├── type_registry.h           # Type system classes
└── node_registry.h           # Node/Module/Project registry classes

src/
├── type_registry.cpp         # Type registry implementation
└── node_registry.cpp         # Node registry implementation

docs/
└── REGISTRY_SYSTEM.md        # Comprehensive documentation
```

---

## API Summary

### TypeRegistry

```cpp
// Registration
uint64_t register_type(const TypeDefinition& type_def);

// Lookup
std::optional<const TypeDefinition*> get_type(uint64_t type_id);
std::optional<const TypeDefinition*> get_type_by_name(string_view name);
std::optional<uint64_t> get_type_id_by_name(string_view name);
const vector<TypeDefinition>& get_all_types() const;

// Refactoring
bool rename_type(uint64_t type_id, string_view new_name);
bool rename_member(uint64_t type_id, string_view old_name, string_view new_name);
bool change_member_type(uint64_t type_id, string_view member_name, string_view new_type);
bool add_member(uint64_t type_id, const TypeMember& member);
bool remove_member(uint64_t type_id, string_view member_name);
bool update_member_offset(uint64_t type_id, string_view member_name, uint32_t new_offset);

// Serialization
string to_json() const;
bool from_json(string_view json_str);
bool save_to_file(string_view filename) const;
bool load_from_file(string_view filename);

// Utilities
size_t type_count() const;
void clear();
```

### Module

```cpp
optional<Node*> find_node(string_view node_name);
optional<const Node*> find_node(string_view node_name) const;
optional<Node*> find_node_by_id(uint32_t node_id);
optional<const Node*> find_node_by_id(uint32_t node_id) const;
```

### NodeRegistry

```cpp
// Module management
uint64_t create_module(string_view name);
optional<Module*> get_module(uint64_t module_id);
optional<Module*> get_module_by_name(string_view name);

// Node management
uint32_t create_node(uint64_t module_id, const Node& node);
optional<Node*> get_node(uint64_t module_id, uint32_t node_id);

// Refactoring
bool rename_function(uint64_t module_id, uint32_t node_id, string_view new_name);
bool change_parameter_type(uint64_t module_id, uint32_t node_id, string_view param_name, uint64_t new_type_id);
bool change_return_type(uint64_t module_id, uint32_t node_id, uint64_t new_type_id);
bool add_parameter(uint64_t module_id, uint32_t node_id, const NodeParameter& param);
bool remove_parameter(uint64_t module_id, uint32_t node_id, string_view param_name);

// Serialization
string to_json() const;
bool from_json(string_view json_str);
bool save_to_file(string_view filename) const;
bool load_from_file(string_view filename);

// Utilities
size_t module_count() const;
const vector<Module>& get_all_modules() const;
void clear();
```

### ProjectRegistry

```cpp
// Access sub-registries
TypeRegistry& types();
NodeRegistry& nodes();

// Global refactoring
bool rename_global_type(uint64_t type_id, string_view new_name);
bool rename_struct_member(uint64_t type_id, string_view old_member_name, string_view new_member_name);

// Serialization
string to_json() const;
bool from_json(string_view json_str);
bool save_to_file(string_view filename) const;
bool load_from_file(string_view filename);

// Utilities
void clear();
```

---

## Integration Points

### Parser Connection
- Parser creates `TypeDefinition` and `Node` objects from AST
- Register them with the registries during semantic analysis

### Code Generation Connection
- Query types and nodes from registries
- Generate C code, IR, or other representations

### IDE Integration
- Use registries for:
  - Code completion
  - Go-to-definition
  - Refactoring support
  - Type validation

### Serialization
- Save registry state between IDE sessions
- Enable project persistence
- Support version control

---

## Next Steps

1. **Semantic Analysis Integration**
   - Connect parser output to registry
   - Validate type references
   - Track type usage

2. **Code Generation**
   - Generate from registry data
   - Output C code, LLVM IR, or native code

3. **IDE Integration**
   - Expose registries through language server
   - Implement refactoring operations in IDE

4. **Testing**
   - Unit tests for registry operations
   - Refactoring operation testing
   - JSON serialization round-trip testing

5. **Performance Optimization**
   - Index by type properties
   - Lazy loading for large registries
   - Caching for frequent queries

6. **Enhanced Refactoring**
   - Detect breaking changes
   - Generate migration code
   - Track refactoring history

---

## Testing Example

```cpp
#include "node_registry.h"

void test_type_refactoring() {
    Arena arena;
    TypeRegistry types(arena);
    
    // Create a type
    TypeDefinition my_type(TypeDefinition::Kind::STRUCT, 
                           types.alloc_str("Point"), 1);
    my_type.members.push_back(TypeMember{
        .name = types.alloc_str("x"),
        .type_name = types.alloc_str("f64")
    });
    
    uint64_t point_id = types.register_type(my_type);
    
    // Rename type
    assert(types.rename_type(point_id, "Vector2"));
    
    // Verify
    auto renamed = types.get_type_by_name("Vector2");
    assert(renamed.has_value());
    assert(renamed.value()->name == "Vector2");
    
    // Old name no longer works
    assert(!types.get_type_by_name("Point").has_value());
}
```

---

## Conclusion

The Type Registry, Node Registry, and Module System provide a robust foundation for:
- Managing all type and function definitions
- Enabling powerful refactoring operations
- Persistent storage via JSON
- IDE integration and tooling support

Ready for integration with semantic analyzer and code generation.
