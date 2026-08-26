# Ibex IDE Architecture

## Overview

The Ibex IDE provides an integrated development environment with a unique blend of textual and visual code representation. The IDE enables developers to write code both as traditional text and as visual function composition diagrams.

## Core Architecture

### 1. Editor Component

The editor displays individual functions as text code. Each function is an atomic unit in the IDE.

```cpp
// Editor.h - Main editor interface
class Editor {
public:
    // Open function for editing
    void open_function(FunctionId id);
    
    // Save function back to project
    void save_function(FunctionId id);
    
    // Get current function being edited
    FunctionId current_function() const;
    
    // Register callback for text changes
    void on_text_changed(TextChangeCallback callback);
};
```

### 2. Visual Editor Components

#### 2.1 Call Stack View

Shows the hierarchical call graph of functions. Displays:

- **Function Nodes**: Rectangles representing individual functions
- **Call Edges**: Directed edges showing which functions call which
- **Call Parameters**: Parameter flow indicator
- **Return Values**: Return value flow through the call stack

```cpp
// CallStackView.h
class CallStackView {
public:
    // Set root function to display
    void set_root_function(FunctionId root);
    
    // Get called functions at current level
    std::vector<FunctionId> get_callees() const;
    
    // Navigate to function (open in editor)
    void navigate_to_function(FunctionId id);
    
    // Highlight call path
    void highlight_path(const std::vector<FunctionId>& path);
};
```

Usage:
- Double-click a function to open it in the text editor
- Right-click to search for all callers
- Color coding indicates function type (user-defined, library, system)

#### 2.2 Data Flow View

Shows how data flows between functions. Displays:

- **Value Flow**: Data dependencies between function calls
- **Variable Lifetimes**: When variables are allocated/deallocated
- **Parameter Mapping**: Which variables are passed as parameters
- **Memory Layout**: Visual representation of memory organization

```cpp
// DataflowView.h
class DataflowView {
public:
    // Set function to analyze
    void analyze_function(FunctionId id);
    
    // Get data dependencies
    std::vector<FlowEdge> get_flow_edges() const;
    
    // Highlight variable usage
    void highlight_variable(VariableId var);
    
    // Show memory layout for types
    void show_memory_layout(TypeId type);
};
```

Features:
- Identify data bottlenecks
- Trace variable usage across function boundaries
- Visualize memory allocations and deallocations
- Detect unused variables

### 3. Type Registry Integration

The type registry is central to the IDE's refactoring capabilities.

```cpp
// TypeRegistryUI.h
class TypeRegistryUI {
public:
    // Display all registered types
    void show_type_browser();
    
    // Edit type definition
    void edit_type(TypeId id);
    
    // Refactor type globally
    void rename_type_globally(const std::string& old_name, const std::string& new_name);
    
    // Add/remove fields from struct
    void modify_struct_fields(TypeId struct_id, const std::vector<FieldInfo>& new_fields);
};
```

Benefits:
- Single location to manage all type definitions
- Type renaming/refactoring updates entire program automatically
- Visual type hierarchy display
- Field alignment and memory layout visualization

### 4. Project Structure

A project in Ibex IDE is organized as:

```
Project/
├── functions.ibex       // All function definitions
├── types.ireg          // Type registry (serialized)
├── project.json        // Project metadata
├── callgraph.json      // Cached call graph
└── .ibex_cache/        // Build artifacts and indices
```

### 5. View Switching

Users can toggle between View modes:

```
┌─────────────────────────────────────┐
│  [Editor] [Call Stack] [Data Flow]  │  Toggle buttons
├─────────────────────────────────────┤
│                                     │
│  Text Editor or Visual View         │
│                                     │
└─────────────────────────────────────┘
```

- **[Editor]**: Text-based function editing
- **[Call Stack]**: Hierarchical function call graph
- **[Data Flow]**: Variable and data dependency flow

### 6. Key Features

#### 6.1 Unified Type Management

All types are stored in a centralized registry accessible from:
- Type browser panel
- Inline type hints in editor
- Visual type definitions in diagrams
- Refactoring operations

#### 6.2 Navigation

- **Go to Definition**: Click function name → open function in editor
- **Find References**: Right-click function → show all callers
- **Type References**: Click type name → show all uses of that type
- **Breadcrumb Navigation**: Show function call stack at top

#### 6.3 Refactoring

- **Rename Function**: Update all call sites automatically
- **Rename Type**: Update all type definitions and usages
- **Move Function**: Update call graph and imports
- **Extract Function**: Select code → create new function
- **Change Function Signature**: Update all callers

### 7. Real-time Analysis

The IDE performs real-time analysis:

- **Type Checking**: Instant error highlighting
- **Memory Analysis**: Detect potential memory leaks
- **Call Graph**: Automatically maintained, updated on edit
- **Symbol Resolution**: Resolve all symbols in real-time
- **Dataflow Analysis**: Track variable flow through program

### 8. Export/Import

The IDE can export:

1. **Call Graph**: As DOT format for Graphviz
2. **Data Flow**: As flow diagram images
3. **Type Hierarchy**: As class/struct diagrams
4. **Memory Layout**: As ASCII diagrams

### 9. Integration with Compiler

The IDE maintains a persistent compilation state:

```cpp
// CompilationContext.h
class CompilationContext {
public:
    // Update incremental compilation
    void on_function_edited(FunctionId id, const std::string& source);
    
    // Get compilation errors
    std::vector<CompileError> get_errors();
    
    // Trigger full rebuild
    void rebuild();
    
    // Export object files
    void export_to_object_files(const std::string& output_dir);
};
```

## Example Workflow

### Create and Visualize a Program

1. **Create Function**: New → Function `calculate_total`
   - Editor opens with empty function
   - Text editor shows:`func calculate_total(i32 a, i32 b) -> i32 { }`

2. **Write Code**: Type function implementation
   ```ibex
   func calculate_total(i32 a, i32 b) -> i32 {
       let sum: i32 = add(a, b);
       return sum;
   }
   ```

3. **Switch to Call Stack View**: Click [Call Stack] button
   - Shows `calculate_total` calls `add`
   - Visual box for `calculate_total` with parameter inputs

4. **Add Type**: New → Type `Result`
   - Type registry shows new type
   - Can be used in function signatures

5. **Switch to Data Flow View**: Click [Data Flow]
   - Shows parameter `a` and `b` flowing to `add` call
   - Shows `sum` return value
   - Visualizes memory locations

6. **Refactor**: Type Registry → Rename `Result` to `ComputeResult`
   - All references automatically updated
   - No text search and replace needed

7. **Compile**: Build → Compile
   - IDE generates object files
   - Links with C runtime
   - Shows errors/warnings in editor inline

## Editor Keybindings (Proposed)

```
Ctrl+G   - Go to function by name
Ctrl+B   - Toggle between text/call-stack views
Ctrl+D   - Show data flow for current selection
Ctrl+R   - Show all references to symbol
Ctrl+H   - Show type hierarchy
Ctrl+T   - Open type registry
Alt+F12  - Find all callers
F2       - Rename symbol (updates all references)
```

## Future Enhancements

1. **Debugger Integration**: Step through code with visual call stack
2. **Performance Profiler**: Overlay execution time on call graph
3. **Memory Visualizer**: Real-time memory allocation tracking
4. **Collaborative Editing**: Multiple users editing same project
5. **Version Control**: Built-in Git integration with visual merging
