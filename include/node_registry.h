#pragma once

#include "str.h"
#include "arena.h"
#include "type_registry.h"
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <string>
#include <memory>

namespace ibex {

// Forward declare
struct Node;
struct Module;

// ============================================================================
// NODE - Represents a function as a computation node
// ============================================================================

struct NodeParameter {
    Str name;                          // Parameter name
    uint64_t type_id;                  // Reference to type in TypeRegistry
    std::optional<Str> default_value;  // Default value as string
};

struct NodeOutput {
    Str name;                  // Output name
    uint64_t type_id;          // Return type in TypeRegistry
};

struct NodeInvocation {
    uint32_t target_node_id;   // ID of invoked node in the module
    uint32_t argument_index;   // Which argument this provides
};

struct Node {
    enum class Kind : uint8_t {
        FUNCTION,       // Regular function
        EXTERNAL,       // External C function
        BUILTIN,        // Built-in function (print, alloc, etc.)
        LAMBDA,         // Anonymous function
    };
    
    uint32_t id;                           // Node ID within its module
    Kind kind;
    Str name;                              // Function name
    std::vector<NodeParameter> parameters; // Input parameters
    NodeOutput output;                     // Return type
    std::vector<NodeInvocation> invocations; // Function invocations within this node
    std::optional<Str> source_location;    // File path and line number
    uint64_t unique_id;                    // Globally unique ID
    
    Node() 
        : id(0), kind(Kind::FUNCTION), unique_id(0) {}
    
    Node(uint32_t node_id, Str n, Kind k, uint64_t uid)
        : id(node_id), kind(k), name(n), unique_id(uid) {}
};

// ============================================================================
// MODULE - Collection of connected nodes (functions)
// ============================================================================

struct Module {
    Str name;                              // Module name
    std::vector<Node> nodes;               // All nodes in this module
    std::unordered_map<std::string, uint32_t> name_to_node_id;  // Name lookup
    uint64_t unique_id;                    // Globally unique ID
    
    Module() : unique_id(0) {}
    explicit Module(Str n, uint64_t id) : name(n), unique_id(id) {}
    
    // Find node by name
    std::optional<Node*> find_node(std::string_view node_name);
    std::optional<const Node*> find_node(std::string_view node_name) const;
    
    // Find node by ID
    std::optional<Node*> find_node_by_id(uint32_t node_id);
    std::optional<const Node*> find_node_by_id(uint32_t node_id) const;
};

// ============================================================================
// NODE REGISTRY - Manages nodes across all modules
// ============================================================================

class NodeRegistry {
public:
    explicit NodeRegistry(Arena& arena, TypeRegistry& type_registry);
    
    // Module management
    uint64_t create_module(std::string_view name);
    std::optional<Module*> get_module(uint64_t module_id);
    std::optional<const Module*> get_module(uint64_t module_id) const;
    std::optional<Module*> get_module_by_name(std::string_view name);
    
    // Node management
    uint32_t create_node(uint64_t module_id, const Node& node);
    std::optional<Node*> get_node(uint64_t module_id, uint32_t node_id);
    std::optional<const Node*> get_node(uint64_t module_id, uint32_t node_id) const;
    
    // Refactoring operations
    
    /// Rename a function node across the entire registry (including type names)
    /// Returns true if successful, false if conflicts exist
    bool rename_function(uint64_t module_id, uint32_t node_id, std::string_view new_name);
    
    /// Update parameter type for a function
    /// Returns true if successful
    bool change_parameter_type(uint64_t module_id, uint32_t node_id, 
                              std::string_view param_name, uint64_t new_type_id);
    
    /// Update return type for a function
    bool change_return_type(uint64_t module_id, uint32_t node_id, uint64_t new_type_id);
    
    /// Add a parameter to a function
    /// Returns true if successful
    bool add_parameter(uint64_t module_id, uint32_t node_id, const NodeParameter& param);
    
    /// Remove a parameter from a function
    /// Returns true if successful
    bool remove_parameter(uint64_t module_id, uint32_t node_id, std::string_view param_name);
    
    // JSON serialization/deserialization
    
    /// Serialize all modules to JSON
    std::string to_json() const;
    
    /// Load from JSON
    bool from_json(std::string_view json_str);
    
    /// Save to file
    bool save_to_file(std::string_view filename) const;
    
    /// Load from file
    bool load_from_file(std::string_view filename);
    
    // Utilities
    size_t module_count() const { return modules_.size(); }
    const std::vector<Module>& get_all_modules() const { return modules_; }
    void clear();
    
private:
    Arena& arena_;
    TypeRegistry& type_registry_;
    std::vector<Module> modules_;
    std::unordered_map<std::string, uint64_t> name_to_module_id_;
    uint64_t next_module_id_ = 1;
    uint64_t next_unique_id_ = 1;
    
    // Helper methods
    Str alloc_str(std::string_view sv);
    
    // JSON helpers
    void serialize_module_to_json(std::string& output, const Module& module) const;
    void serialize_node_to_json(std::string& output, const Node& node) const;
    bool deserialize_module_from_json(const std::string& json_str, Module& module);
    bool deserialize_node_from_json(const std::string& json_str, Node& node);
};

// ============================================================================
// COMBINED REGISTRY - Type registry + Node registry together
// ============================================================================

class ProjectRegistry {
public:
    explicit ProjectRegistry(Arena& arena);
    
    TypeRegistry& types() { return type_registry_; }
    const TypeRegistry& types() const { return type_registry_; }
    
    NodeRegistry& nodes() { return node_registry_; }
    const NodeRegistry& nodes() const { return node_registry_; }
    
    // Unified refactoring operations
    
    /// Rename a type everywhere (including in nodes and modules)
    /// Returns true if successful
    bool rename_global_type(uint64_t type_id, std::string_view new_name);
    
    /// Rename a struct member everywhere (including in nodes that use that type)
    /// Returns true if successful
    bool rename_struct_member(uint64_t type_id, std::string_view old_member_name, 
                             std::string_view new_member_name);
    
    // Serialization
    std::string to_json() const;
    bool from_json(std::string_view json_str);
    bool save_to_file(std::string_view filename) const;
    bool load_from_file(std::string_view filename);
    
    void clear();
    
private:
    Arena& arena_;
    TypeRegistry type_registry_;
    NodeRegistry node_registry_;
};

} // namespace ibex
