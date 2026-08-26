#pragma once

#include "str.h"
#include "arena.h"
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <string>
#include <memory>

// Forward declare yyjson types
struct yyjson_doc;
struct yyjson_val;

namespace ibex {

// ============================================================================
// TYPE DEFINITIONS FOR TYPE REGISTRY
// ============================================================================

/// Represents a member of a struct or class
struct TypeMember {
    Str name;                              // Member name
    Str type_name;                         // Type name (e.g., "i32", "Point")
    std::optional<uint32_t> offset;        // Byte offset in struct (-1 for auto)
    std::optional<Str> default_value;      // Default value expression as string
    
    bool operator==(const TypeMember& other) const {
        return name == other.name && type_name == other.type_name;
    }
};

/// Represents a struct or class type
struct TypeDefinition {
    enum class Kind : uint8_t {
        STRUCT,
        CLASS,
        ENUM,
        FLAG,
        PRIMITIVE,
        POINTER,
        REFERENCE,
        ARRAY,
        SLICE,
    };
    
    Kind kind;
    Str name;                                    // Type name
    std::vector<TypeMember> members;             // Members (for struct/class/enum)
    std::vector<Str> bases;                      // Base types for inheritance
    Str underlying_type;                         // For enum/flag: underlying type
    std::optional<Str> extends;                  // For enum: extends another enum
    uint64_t unique_id;                          // Unique identifier for type
    
    TypeDefinition() 
        : kind(Kind::STRUCT), unique_id(0) {}
    
    TypeDefinition(Kind k, Str n, uint64_t id) 
        : kind(k), name(n), unique_id(id) {}
};

// ============================================================================
// TYPE REGISTRY - Stores all types and enables lookup/manipulation
// ============================================================================

class TypeRegistry {
public:
    explicit TypeRegistry(Arena& arena);
    
    // Register a new type
    uint64_t register_type(const TypeDefinition& type_def);
    
    // Lookup functions
    std::optional<const TypeDefinition*> get_type(uint64_t type_id) const;
    std::optional<const TypeDefinition*> get_type_by_name(std::string_view name) const;
    std::optional<uint64_t> get_type_id_by_name(std::string_view name) const;
    
    // Iterator support
    const std::vector<TypeDefinition>& get_all_types() const { return types_; }
    
    // Refactoring operations
    
    /// Rename a type everywhere in the registry
    /// Returns true if successful, false if type not found or rename conflicts exist
    bool rename_type(uint64_t type_id, std::string_view new_name);
    
    /// Rename a member within a struct type
    /// Returns true if successful, false if type/member not found
    bool rename_member(uint64_t type_id, std::string_view old_name, std::string_view new_name);
    
    /// Change a member's type within a struct
    /// Returns true if successful
    bool change_member_type(uint64_t type_id, std::string_view member_name, std::string_view new_type);
    
    /// Add a new member to a struct type
    /// Returns true if successful, false if duplicate member name
    bool add_member(uint64_t type_id, const TypeMember& member);
    
    /// Remove a member from a struct type
    /// Returns true if successful, false if member not found
    bool remove_member(uint64_t type_id, std::string_view member_name);
    
    /// Update a member's offset (for memory layout changes)
    bool update_member_offset(uint64_t type_id, std::string_view member_name, uint32_t new_offset);
    
    // JSON serialization/deserialization
    
    /// Serialize the entire type registry to JSON
    std::string to_json() const;
    
    /// Load type registry from JSON
    bool from_json(std::string_view json_str);
    
    /// Save to file
    bool save_to_file(std::string_view filename) const;
    
    /// Load from file
    bool load_from_file(std::string_view filename);
    
    // Utilities
    size_t type_count() const { return types_.size(); }
    void clear();
    
private:
    Arena& arena_;
    std::vector<TypeDefinition> types_;
    std::unordered_map<std::string, uint64_t> name_to_id_;
    uint64_t next_type_id_ = 1;
    
    // Helper methods
    Str alloc_str(std::string_view sv);
    uint64_t find_member_index(const TypeDefinition& type_def, std::string_view member_name) const;
    
    // JSON helpers
    void serialize_type_to_json(yyjson_val* json_arr, const TypeDefinition& type_def) const;
    bool deserialize_type_from_json(const yyjson_val* json_obj, TypeDefinition& type_def);
};

} // namespace ibex
