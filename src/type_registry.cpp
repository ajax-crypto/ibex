// Copyright (c) 2026 Akash Pradhan
// SPDX-License-Identifier: MIT

#include "type_registry.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>

// Using glaze for serialization
#include <type_traits>
#include <utility>
#include <ranges>
#include <glaze/glaze.hpp>

namespace ibex {

TypeRegistry::TypeRegistry(Arena& arena)
    : arena_(arena) {}

Str TypeRegistry::alloc_str(std::string_view sv) {
    char* ptr = static_cast<char*>(arena_.allocate(sv.size()));
    std::memcpy(ptr, sv.data(), sv.size());
    return Str(ptr, sv.size());
}

uint64_t TypeRegistry::register_type(const TypeDefinition& type_def) {
    TypeDefinition def_copy = type_def;
    def_copy.unique_id = next_type_id_;
    
    // Store in map for quick lookup by name
    std::string name_str(def_copy.name.data(), def_copy.name.size());
    name_to_id_[name_str] = next_type_id_;
    
    types_.push_back(def_copy);
    return next_type_id_++;
}

std::optional<const TypeDefinition*> TypeRegistry::get_type(uint64_t type_id) const {
    for (const auto& type_def : types_) {
        if (type_def.unique_id == type_id) {
            return &type_def;
        }
    }
    return std::nullopt;
}

std::optional<const TypeDefinition*> TypeRegistry::get_type_by_name(std::string_view name) const {
    std::string name_str(name.data(), name.size());
    auto it = name_to_id_.find(name_str);
    if (it != name_to_id_.end()) {
        return get_type(it->second);
    }
    return std::nullopt;
}

std::optional<uint64_t> TypeRegistry::get_type_id_by_name(std::string_view name) const {
    std::string name_str(name.data(), name.size());
    auto it = name_to_id_.find(name_str);
    if (it != name_to_id_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool TypeRegistry::rename_type(uint64_t type_id, std::string_view new_name) {
    auto type_opt = get_type(type_id);
    if (!type_opt) return false;
    
    // Check for name conflicts
    std::string new_name_str(new_name.data(), new_name.size());
    if (name_to_id_.find(new_name_str) != name_to_id_.end()) {
        return false;  // Name already exists
    }
    
    // Find and update in types_ vector
    for (auto& type_def : types_) {
        if (type_def.unique_id == type_id) {
            // Remove old name mapping
            std::string old_name_str(type_def.name.data(), type_def.name.size());
            name_to_id_.erase(old_name_str);
            
            // Add new name mapping
            type_def.name = alloc_str(new_name);
            name_to_id_[new_name_str] = type_id;
            
            // Update references in all member types
            for (auto& other_type : types_) {
                for (auto& member : other_type.members) {
                    std::string member_type_str(member.type_name.data(), member.type_name.size());
                    if (member_type_str == old_name_str) {
                        member.type_name = alloc_str(new_name);
                    }
                }
                
                // Update base types
                for (auto& base : other_type.bases) {
                    std::string base_str(base.data(), base.size());
                    if (base_str == old_name_str) {
                        base = alloc_str(new_name);
                    }
                }
            }
            
            return true;
        }
    }
    
    return false;
}

uint64_t TypeRegistry::find_member_index(const TypeDefinition& type_def, std::string_view member_name) const {
    for (uint64_t i = 0; i < type_def.members.size(); ++i) {
        std::string mem_name(type_def.members[i].name.data(), type_def.members[i].name.size());
        if (mem_name == member_name) {
            return i;
        }
    }
    return std::string::npos;
}

bool TypeRegistry::rename_member(uint64_t type_id, std::string_view old_name, std::string_view new_name) {
    for (auto& type_def : types_) {
        if (type_def.unique_id == type_id) {
            uint64_t idx = find_member_index(type_def, old_name);
            if (idx != std::string::npos && idx < type_def.members.size()) {
                type_def.members[idx].name = alloc_str(new_name);
                return true;
            }
            return false;
        }
    }
    return false;
}

bool TypeRegistry::change_member_type(uint64_t type_id, std::string_view member_name, std::string_view new_type) {
    for (auto& type_def : types_) {
        if (type_def.unique_id == type_id) {
            uint64_t idx = find_member_index(type_def, member_name);
            if (idx != std::string::npos && idx < type_def.members.size()) {
                type_def.members[idx].type_name = alloc_str(new_type);
                return true;
            }
            return false;
        }
    }
    return false;
}

bool TypeRegistry::add_member(uint64_t type_id, const TypeMember& member) {
    for (auto& type_def : types_) {
        if (type_def.unique_id == type_id) {
            // Check for duplicate
            if (find_member_index(type_def, std::string_view(member.name.data(), member.name.size())) != std::string::npos) {
                return false;
            }
            TypeMember new_member = member;
            new_member.name = alloc_str(std::string_view(member.name.data(), member.name.size()));
            new_member.type_name = alloc_str(std::string_view(member.type_name.data(), member.type_name.size()));
            type_def.members.push_back(new_member);
            return true;
        }
    }
    return false;
}

bool TypeRegistry::remove_member(uint64_t type_id, std::string_view member_name) {
    for (auto& type_def : types_) {
        if (type_def.unique_id == type_id) {
            auto it = std::find_if(type_def.members.begin(), type_def.members.end(),
                [member_name](const TypeMember& m) {
                    return std::string_view(m.name.data(), m.name.size()) == member_name;
                }
            );
            if (it != type_def.members.end()) {
                type_def.members.erase(it);
                return true;
            }
            return false;
        }
    }
    return false;
}

bool TypeRegistry::update_member_offset(uint64_t type_id, std::string_view member_name, uint32_t new_offset) {
    for (auto& type_def : types_) {
        if (type_def.unique_id == type_id) {
            uint64_t idx = find_member_index(type_def, member_name);
            if (idx != std::string::npos && idx < type_def.members.size()) {
                type_def.members[idx].offset = new_offset;
                return true;
            }
            return false;
        }
    }
    return false;
}

// Namespace continues

// Mirror structs for Glaze serialization
struct GlazeTypeMember {
    std::string name;
    std::string type_name;
    std::optional<uint32_t> offset;
    std::optional<std::string> default_value;
};

struct GlazeTypeDefinition {
    TypeDefinition::Kind kind;
    std::string name;
    std::vector<GlazeTypeMember> members;
    std::vector<std::string> bases;
    std::string underlying_type;
    std::optional<std::string> extends;
    uint64_t unique_id;
};

} // namespace ibex

template <>
struct glz::meta<ibex::GlazeTypeMember> {
    using T = ibex::GlazeTypeMember;
    static constexpr auto value = object(
        "name", &T::name,
        "type_name", &T::type_name,
        "offset", &T::offset,
        "default_value", &T::default_value
    );
};

template <>
struct glz::meta<ibex::GlazeTypeDefinition> {
    using T = ibex::GlazeTypeDefinition;
    static constexpr auto value = object(
        "kind", &T::kind,
        "name", &T::name,
        "members", &T::members,
        "bases", &T::bases,
        "underlying_type", &T::underlying_type,
        "extends", &T::extends,
        "unique_id", &T::unique_id
    );
};

namespace ibex {

std::string TypeRegistry::to_json() const {
    std::vector<GlazeTypeDefinition> glaze_types;
    glaze_types.reserve(types_.size());
    for (const auto& t : types_) {
        GlazeTypeDefinition g;
        g.kind = t.kind;
        g.name = std::string(t.name.ptr(), t.name.len());
        g.unique_id = t.unique_id;
        g.underlying_type = std::string(t.underlying_type.ptr(), t.underlying_type.len());
        if (t.extends) g.extends = std::string(t.extends->ptr(), t.extends->len());
        
        for (const auto& m : t.members) {
            GlazeTypeMember gm;
            gm.name = std::string(m.name.ptr(), m.name.len());
            gm.type_name = std::string(m.type_name.ptr(), m.type_name.len());
            gm.offset = m.offset;
            if (m.default_value) gm.default_value = std::string(m.default_value->ptr(), m.default_value->len());
            g.members.push_back(std::move(gm));
        }
        
        for (const auto& b : t.bases) {
            g.bases.push_back(std::string(b.ptr(), b.len()));
        }
        
        glaze_types.push_back(std::move(g));
    }
    
    std::string json;
    glz::write_json(glaze_types, json);
    return json;
}

bool TypeRegistry::from_json(std::string_view json_str) {
    std::vector<GlazeTypeDefinition> glaze_types;
    auto ec = glz::read_json(glaze_types, json_str);
    if (ec) return false;
    
    types_.clear();
    name_to_id_.clear();
    next_type_id_ = 1;
    
    for (const auto& g : glaze_types) {
        TypeDefinition t;
        t.kind = g.kind;
        t.name = alloc_str(g.name);
        t.unique_id = g.unique_id;
        next_type_id_ = std::max(next_type_id_, t.unique_id + 1);
        t.underlying_type = alloc_str(g.underlying_type);
        if (g.extends) t.extends = alloc_str(*g.extends);
        
        for (const auto& gm : g.members) {
            TypeMember m;
            m.name = alloc_str(gm.name);
            m.type_name = alloc_str(gm.type_name);
            m.offset = gm.offset;
            if (gm.default_value) m.default_value = alloc_str(*gm.default_value);
            t.members.push_back(m);
        }
        
        for (const auto& b : g.bases) {
            t.bases.push_back(alloc_str(b));
        }
        
        name_to_id_[g.name] = t.unique_id;
        types_.push_back(t);
    }
    
    return true;
}

bool TypeRegistry::save_to_file(std::string_view filename) const {
    std::string json = to_json();
    std::ofstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) return false;
    file.write(json.c_str(), json.size());
    return file.good();
}

bool TypeRegistry::load_from_file(std::string_view filename) {
    std::ifstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) return false;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return from_json(buffer.str());
}

void TypeRegistry::clear() {
    types_.clear();
    name_to_id_.clear();
    next_type_id_ = 1;
}

} // namespace ibex
