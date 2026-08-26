#include "type_registry.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>

// Include yyjson
#include <yyjson.h>

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

std::string TypeRegistry::to_json() const {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
    yyjson_mut_val *root = yyjson_mut_arr(doc);
    yyjson_mut_doc_set_root(doc, root);
    
    for (const auto& type_def : types_) {
        yyjson_mut_val *type_obj = yyjson_mut_obj(doc);
        
        // Add type info
        yyjson_mut_obj_add_str(doc, type_obj, "name", type_def.name.data(), type_def.name.size());
        yyjson_mut_obj_add_uint(doc, type_obj, "kind", static_cast<uint32_t>(type_def.kind));
        yyjson_mut_obj_add_uint(doc, type_obj, "id", type_def.unique_id);
        
        // Add members
        yyjson_mut_val *members_arr = yyjson_mut_arr(doc);
        for (const auto& member : type_def.members) {
            yyjson_mut_val *member_obj = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_str(doc, member_obj, "name", member.name.data(), member.name.size());
            yyjson_mut_obj_add_str(doc, member_obj, "type", member.type_name.data(), member.type_name.size());
            
            if (member.offset.has_value()) {
                yyjson_mut_obj_add_uint(doc, member_obj, "offset", member.offset.value());
            }
            
            if (member.default_value.has_value()) {
                yyjson_mut_obj_add_str(doc, member_obj, "default", 
                    member.default_value.value().data(), member.default_value.value().size());
            }
            
            yyjson_mut_arr_append(members_arr, member_obj);
        }
        yyjson_mut_obj_add_val(doc, type_obj, "members", members_arr);
        
        // Add bases
        yyjson_mut_val *bases_arr = yyjson_mut_arr(doc);
        for (const auto& base : type_def.bases) {
            yyjson_mut_arr_append(bases_arr, yyjson_mut_str(doc, base.data(), base.size()));
        }
        yyjson_mut_obj_add_val(doc, type_obj, "bases", bases_arr);
        
        // Add other fields
        if (!type_def.underlying_type.is_empty()) {
            yyjson_mut_obj_add_str(doc, type_obj, "underlying_type", 
                type_def.underlying_type.data(), type_def.underlying_type.size());
        }
        
        if (type_def.extends.has_value()) {
            yyjson_mut_obj_add_str(doc, type_obj, "extends", 
                type_def.extends.value().data(), type_def.extends.value().size());
        }
        
        yyjson_mut_arr_append(root, type_obj);
    }
    
    // Convert to string
    size_t json_size = 0;
    char *json_str = yyjson_mut_write(doc, 0, &json_size);
    std::string result(json_str, json_size);
    
    free(json_str);
    yyjson_mut_doc_free(doc);
    
    return result;
}

bool TypeRegistry::from_json(std::string_view json_str) {
    yyjson_doc *doc = yyjson_read(json_str.data(), json_str.size(), nullptr);
    if (!doc) return false;
    
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_arr(root)) {
        yyjson_doc_free(doc);
        return false;
    }
    
    clear();
    
    yyjson_arr_iter iter = yyjson_arr_iter_with(root);
    yyjson_val *type_obj;
    while ((type_obj = yyjson_arr_iter_next(&iter))) {
        TypeDefinition type_def;
        
        // Read type info
        yyjson_val *name_val = yyjson_obj_get(type_obj, "name");
        if (name_val && yyjson_is_str(name_val)) {
            type_def.name = alloc_str(yyjson_get_str(name_val));
        }
        
        yyjson_val *kind_val = yyjson_obj_get(type_obj, "kind");
        if (kind_val && yyjson_is_uint(kind_val)) {
            type_def.kind = static_cast<TypeDefinition::Kind>(yyjson_get_uint(kind_val));
        }
        
        yyjson_val *id_val = yyjson_obj_get(type_obj, "id");
        if (id_val && yyjson_is_uint(id_val)) {
            type_def.unique_id = yyjson_get_uint(id_val);
            next_type_id_ = std::max(next_type_id_, type_def.unique_id + 1);
        }
        
        // Read members
        yyjson_val *members_arr = yyjson_obj_get(type_obj, "members");
        if (members_arr && yyjson_is_arr(members_arr)) {
            yyjson_arr_iter members_iter = yyjson_arr_iter_with(members_arr);
            yyjson_val *member_obj;
            while ((member_obj = yyjson_arr_iter_next(&members_iter))) {
                TypeMember member;
                
                yyjson_val *mem_name = yyjson_obj_get(member_obj, "name");
                if (mem_name) member.name = alloc_str(yyjson_get_str(mem_name));
                
                yyjson_val *mem_type = yyjson_obj_get(member_obj, "type");
                if (mem_type) member.type_name = alloc_str(yyjson_get_str(mem_type));
                
                yyjson_val *mem_offset = yyjson_obj_get(member_obj, "offset");
                if (mem_offset && yyjson_is_uint(mem_offset)) {
                    member.offset = static_cast<uint32_t>(yyjson_get_uint(mem_offset));
                }
                
                yyjson_val *mem_default = yyjson_obj_get(member_obj, "default");
                if (mem_default && yyjson_is_str(mem_default)) {
                    member.default_value = alloc_str(yyjson_get_str(mem_default));
                }
                
                type_def.members.push_back(member);
            }
        }
        
        // Read bases
        yyjson_val *bases_arr = yyjson_obj_get(type_obj, "bases");
        if (bases_arr && yyjson_is_arr(bases_arr)) {
            yyjson_arr_iter bases_iter = yyjson_arr_iter_with(bases_arr);
            yyjson_val *base_val;
            while ((base_val = yyjson_arr_iter_next(&bases_iter))) {
                if (yyjson_is_str(base_val)) {
                    type_def.bases.push_back(alloc_str(yyjson_get_str(base_val)));
                }
            }
        }
        
        // Read other fields
        yyjson_val *underlying = yyjson_obj_get(type_obj, "underlying_type");
        if (underlying && yyjson_is_str(underlying)) {
            type_def.underlying_type = alloc_str(yyjson_get_str(underlying));
        }
        
        yyjson_val *extends = yyjson_obj_get(type_obj, "extends");
        if (extends && yyjson_is_str(extends)) {
            type_def.extends = alloc_str(yyjson_get_str(extends));
        }
        
        // Register type
        std::string name_str(type_def.name.data(), type_def.name.size());
        name_to_id_[name_str] = type_def.unique_id;
        types_.push_back(type_def);
    }
    
    yyjson_doc_free(doc);
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
