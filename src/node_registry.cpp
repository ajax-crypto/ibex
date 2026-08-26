#include "node_registry.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>
#include <yyjson.h>

namespace ibex {

// ============================================================================
// MODULE IMPLEMENTATION
// ============================================================================

std::optional<Node*> Module::find_node(std::string_view node_name) {
    std::string name_str(node_name.data(), node_name.size());
    auto it = name_to_node_id.find(name_str);
    if (it != name_to_node_id.end() && it->second < nodes.size()) {
        return &nodes[it->second];
    }
    return std::nullopt;
}

std::optional<const Node*> Module::find_node(std::string_view node_name) const {
    std::string name_str(node_name.data(), node_name.size());
    auto it = name_to_node_id.find(name_str);
    if (it != name_to_node_id.end() && it->second < nodes.size()) {
        return &nodes[it->second];
    }
    return std::nullopt;
}

std::optional<Node*> Module::find_node_by_id(uint32_t node_id) {
    if (node_id < nodes.size()) {
        return &nodes[node_id];
    }
    return std::nullopt;
}

std::optional<const Node*> Module::find_node_by_id(uint32_t node_id) const {
    if (node_id < nodes.size()) {
        return &nodes[node_id];
    }
    return std::nullopt;
}

// ============================================================================
// NODE REGISTRY IMPLEMENTATION
// ============================================================================

NodeRegistry::NodeRegistry(Arena& arena, TypeRegistry& type_registry)
    : arena_(arena), type_registry_(type_registry) {}

Str NodeRegistry::alloc_str(std::string_view sv) {
    char* ptr = static_cast<char*>(arena_.allocate(sv.size()));
    std::memcpy(ptr, sv.data(), sv.size());
    return Str(ptr, sv.size());
}

uint64_t NodeRegistry::create_module(std::string_view name) {
    Module module(alloc_str(name), next_module_id_);
    
    std::string name_str(name.data(), name.size());
    name_to_module_id_[name_str] = next_module_id_;
    
    modules_.push_back(module);
    return next_module_id_++;
}

std::optional<Module*> NodeRegistry::get_module(uint64_t module_id) {
    for (auto& module : modules_) {
        if (module.unique_id == module_id) {
            return &module;
        }
    }
    return std::nullopt;
}

std::optional<const Module*> NodeRegistry::get_module(uint64_t module_id) const {
    for (const auto& module : modules_) {
        if (module.unique_id == module_id) {
            return &module;
        }
    }
    return std::nullopt;
}

std::optional<Module*> NodeRegistry::get_module_by_name(std::string_view name) {
    std::string name_str(name.data(), name.size());
    auto it = name_to_module_id_.find(name_str);
    if (it != name_to_module_id_.end()) {
        return get_module(it->second);
    }
    return std::nullopt;
}

uint32_t NodeRegistry::create_node(uint64_t module_id, const Node& node) {
    auto module_opt = get_module(module_id);
    if (!module_opt) return 0;  // Invalid module ID
    
    Module& module = *module_opt;
    Node new_node = node;
    new_node.id = static_cast<uint32_t>(module.nodes.size());
    new_node.unique_id = next_unique_id_++;
    
    std::string name_str(new_node.name.data(), new_node.name.size());
    module.name_to_node_id[name_str] = new_node.id;
    
    module.nodes.push_back(new_node);
    return new_node.id;
}

std::optional<Node*> NodeRegistry::get_node(uint64_t module_id, uint32_t node_id) {
    auto module_opt = get_module(module_id);
    if (!module_opt) return std::nullopt;
    
    return module_opt.value()->find_node_by_id(node_id);
}

std::optional<const Node*> NodeRegistry::get_node(uint64_t module_id, uint32_t node_id) const {
    auto module_opt = get_module(module_id);
    if (!module_opt) return std::nullopt;
    
    return module_opt.value()->find_node_by_id(node_id);
}

bool NodeRegistry::rename_function(uint64_t module_id, uint32_t node_id, std::string_view new_name) {
    auto module_opt = get_module(module_id);
    if (!module_opt) return false;
    
    Module& module = *module_opt;
    auto node_opt = module.find_node_by_id(node_id);
    if (!node_opt) return false;
    
    Node& node = *node_opt;
    
    // Check for conflicts
    std::string new_name_str(new_name.data(), new_name.size());
    if (module.name_to_node_id.find(new_name_str) != module.name_to_node_id.end()) {
        return false;  // Name already exists in module
    }
    
    // Remove old name mapping
    std::string old_name_str(node.name.data(), node.name.size());
    module.name_to_node_id.erase(old_name_str);
    
    // Update name
    node.name = alloc_str(new_name);
    module.name_to_node_id[new_name_str] = node_id;
    
    return true;
}

bool NodeRegistry::change_parameter_type(uint64_t module_id, uint32_t node_id,
                                        std::string_view param_name, uint64_t new_type_id) {
    auto node_opt = get_node(module_id, node_id);
    if (!node_opt) return false;
    
    Node& node = *node_opt;
    
    for (auto& param : node.parameters) {
        std::string pname(param.name.data(), param.name.size());
        if (pname == param_name) {
            param.type_id = new_type_id;
            return true;
        }
    }
    
    return false;
}

bool NodeRegistry::change_return_type(uint64_t module_id, uint32_t node_id, uint64_t new_type_id) {
    auto node_opt = get_node(module_id, node_id);
    if (!node_opt) return false;
    
    Node& node = *node_opt;
    node.output.type_id = new_type_id;
    
    return true;
}

bool NodeRegistry::add_parameter(uint64_t module_id, uint32_t node_id, const NodeParameter& param) {
    auto node_opt = get_node(module_id, node_id);
    if (!node_opt) return false;
    
    Node& node = *node_opt;
    
    // Check for duplicate
    std::string pname(param.name.data(), param.name.size());
    for (const auto& existing : node.parameters) {
        std::string existing_name(existing.name.data(), existing.name.size());
        if (existing_name == pname) {
            return false;  // Duplicate parameter name
        }
    }
    
    NodeParameter new_param = param;
    new_param.name = alloc_str(pname);
    node.parameters.push_back(new_param);
    
    return true;
}

bool NodeRegistry::remove_parameter(uint64_t module_id, uint32_t node_id, std::string_view param_name) {
    auto node_opt = get_node(module_id, node_id);
    if (!node_opt) return false;
    
    Node& node = **node_opt;
    
    auto it = std::find_if(node.parameters.begin(), node.parameters.end(),
        [param_name](const NodeParameter& p) {
            return std::string_view(p.name.data(), p.name.size()) == param_name;
        }
    );
    
    if (it != node.parameters.end()) {
        node.parameters.erase(it);
        return true;
    }
    
    return false;
}

std::string NodeRegistry::to_json() const {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
    yyjson_mut_val *root = yyjson_mut_arr(doc);
    yyjson_mut_doc_set_root(doc, root);
    
    for (const auto& module : modules_) {
        yyjson_mut_val *module_obj = yyjson_mut_obj(doc);
        
        yyjson_mut_obj_add_str(doc, module_obj, "name", module.name.data(), module.name.size());
        yyjson_mut_obj_add_uint(doc, module_obj, "id", module.unique_id);
        
        // Add nodes
        yyjson_mut_val *nodes_arr = yyjson_mut_arr(doc);
        
        for (const auto& node : module.nodes) {
            yyjson_mut_val *node_obj = yyjson_mut_obj(doc);
            
            yyjson_mut_obj_add_str(doc, node_obj, "name", node.name.data(), node.name.size());
            yyjson_mut_obj_add_uint(doc, node_obj, "id", node.id);
            yyjson_mut_obj_add_uint(doc, node_obj, "kind", static_cast<uint32_t>(node.kind));
            yyjson_mut_obj_add_uint(doc, node_obj, "unique_id", node.unique_id);
            
            // Add parameters
            yyjson_mut_val *params_arr = yyjson_mut_arr(doc);
            for (const auto& param : node.parameters) {
                yyjson_mut_val *param_obj = yyjson_mut_obj(doc);
                yyjson_mut_obj_add_str(doc, param_obj, "name", param.name.data(), param.name.size());
                yyjson_mut_obj_add_uint(doc, param_obj, "type_id", param.type_id);
                
                if (param.default_value.has_value()) {
                    yyjson_mut_obj_add_str(doc, param_obj, "default",
                        param.default_value.value().data(), param.default_value.value().size());
                }
                
                yyjson_mut_arr_append(params_arr, param_obj);
            }
            yyjson_mut_obj_add_val(doc, node_obj, "parameters", params_arr);
            
            // Add output
            yyjson_mut_val *output_obj = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_str(doc, output_obj, "name", node.output.name.data(), node.output.name.size());
            yyjson_mut_obj_add_uint(doc, output_obj, "type_id", node.output.type_id);
            yyjson_mut_obj_add_val(doc, node_obj, "output", output_obj);
            
            // Add invocations
            yyjson_mut_val *invocations_arr = yyjson_mut_arr(doc);
            for (const auto& inv : node.invocations) {
                yyjson_mut_val *inv_obj = yyjson_mut_obj(doc);
                yyjson_mut_obj_add_uint(doc, inv_obj, "target_node_id", inv.target_node_id);
                yyjson_mut_obj_add_uint(doc, inv_obj, "argument_index", inv.argument_index);
                yyjson_mut_arr_append(invocations_arr, inv_obj);
            }
            yyjson_mut_obj_add_val(doc, node_obj, "invocations", invocations_arr);
            
            // Add source location
            if (node.source_location.has_value()) {
                yyjson_mut_obj_add_str(doc, node_obj, "source_location",
                    node.source_location.value().data(), node.source_location.value().size());
            }
            
            yyjson_mut_arr_append(nodes_arr, node_obj);
        }
        
        yyjson_mut_obj_add_val(doc, module_obj, "nodes", nodes_arr);
        yyjson_mut_arr_append(root, module_obj);
    }
    
    size_t json_size = 0;
    char *json_str = yyjson_mut_write(doc, 0, &json_size);
    std::string result(json_str, json_size);
    
    free(json_str);
    yyjson_mut_doc_free(doc);
    
    return result;
}

bool NodeRegistry::from_json(std::string_view json_str) {
    yyjson_doc *doc = yyjson_read(json_str.data(), json_str.size(), 0);
    if (!doc) return false;
    
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_arr(root)) {
        yyjson_doc_free(doc);
        return false;
    }
    
    clear();
    
    yyjson_arr_iter iter = yyjson_arr_iter_with(root);
    yyjson_val *module_obj;
    while ((module_obj = yyjson_arr_iter_next(&iter))) {
        Module module;
        
        yyjson_val *name_val = yyjson_obj_get(module_obj, "name");
        if (name_val) module.name = alloc_str(yyjson_get_str(name_val));
        
        yyjson_val *id_val = yyjson_obj_get(module_obj, "id");
        if (id_val) {
            module.unique_id = yyjson_get_uint(id_val);
            next_module_id_ = std::max(next_module_id_, module.unique_id + 1);
        }
        
        // Read nodes
        yyjson_val *nodes_arr = yyjson_obj_get(module_obj, "nodes");
        if (nodes_arr && yyjson_is_arr(nodes_arr)) {
            yyjson_arr_iter nodes_iter = yyjson_arr_iter_with(nodes_arr);
            yyjson_val *node_obj;
            while ((node_obj = yyjson_arr_iter_next(&nodes_iter))) {
                Node node;
                
                yyjson_val *node_name = yyjson_obj_get(node_obj, "name");
                if (node_name) node.name = alloc_str(yyjson_get_str(node_name));
                
                yyjson_val *node_id = yyjson_obj_get(node_obj, "id");
                if (node_id) node.id = static_cast<uint32_t>(yyjson_get_uint(node_id));
                
                yyjson_val *node_kind = yyjson_obj_get(node_obj, "kind");
                if (node_kind) node.kind = static_cast<Node::Kind>(yyjson_get_uint(node_kind));
                
                yyjson_val *node_uid = yyjson_obj_get(node_obj, "unique_id");
                if (node_uid) {
                    node.unique_id = yyjson_get_uint(node_uid);
                    next_unique_id_ = std::max(next_unique_id_, node.unique_id + 1);
                }
                
                // Read parameters
                yyjson_val *params_arr = yyjson_obj_get(node_obj, "parameters");
                if (params_arr && yyjson_is_arr(params_arr)) {
                    yyjson_arr_iter params_iter = yyjson_arr_iter_with(params_arr);
                    yyjson_val *param_val;
                    while ((param_val = yyjson_arr_iter_next(&params_iter))) {
                        NodeParameter param;
                        
                        yyjson_val *pname = yyjson_obj_get(param_val, "name");
                        if (pname) param.name = alloc_str(yyjson_get_str(pname));
                        
                        yyjson_val *ptype = yyjson_obj_get(param_val, "type_id");
                        if (ptype) param.type_id = yyjson_get_uint(ptype);
                        
                        yyjson_val *pdef = yyjson_obj_get(param_val, "default");
                        if (pdef && yyjson_is_str(pdef)) {
                            param.default_value = alloc_str(yyjson_get_str(pdef));
                        }
                        
                        node.parameters.push_back(param);
                    }
                }
                
                // Read output
                yyjson_val *output_val = yyjson_obj_get(node_obj, "output");
                if (output_val && yyjson_is_obj(output_val)) {
                    yyjson_val *oname = yyjson_obj_get(output_val, "name");
                    if (oname) node.output.name = alloc_str(yyjson_get_str(oname));
                    
                    yyjson_val *otype = yyjson_obj_get(output_val, "type_id");
                    if (otype) node.output.type_id = yyjson_get_uint(otype);
                }
                
                // Read invocations
                yyjson_val *invocations_arr = yyjson_obj_get(node_obj, "invocations");
                if (invocations_arr && yyjson_is_arr(invocations_arr)) {
                    yyjson_arr_iter inv_iter = yyjson_arr_iter_with(invocations_arr);
                    yyjson_val *inv_val;
                    while ((inv_val = yyjson_arr_iter_next(&inv_iter))) {
                        NodeInvocation inv;
                        
                        yyjson_val *target = yyjson_obj_get(inv_val, "target_node_id");
                        if (target) inv.target_node_id = static_cast<uint32_t>(yyjson_get_uint(target));
                        
                        yyjson_val *arg_idx = yyjson_obj_get(inv_val, "argument_index");
                        if (arg_idx) inv.argument_index = static_cast<uint32_t>(yyjson_get_uint(arg_idx));
                        
                        node.invocations.push_back(inv);
                    }
                }
                
                // Read source location
                yyjson_val *src_loc = yyjson_obj_get(node_obj, "source_location");
                if (src_loc && yyjson_is_str(src_loc)) {
                    node.source_location = alloc_str(yyjson_get_str(src_loc));
                }
                
                std::string nname(node.name.data(), node.name.size());
                module.name_to_node_id[nname] = node.id;
                module.nodes.push_back(node);
            }
        }
        
        std::string mname(module.name.data(), module.name.size());
        name_to_module_id_[mname] = module.unique_id;
        modules_.push_back(module);
    }
    
    yyjson_doc_free(doc);
    return true;
}

bool NodeRegistry::save_to_file(std::string_view filename) const {
    std::string json = to_json();
    std::ofstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) return false;
    file.write(json.c_str(), json.size());
    return file.good();
}

bool NodeRegistry::load_from_file(std::string_view filename) {
    std::ifstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) return false;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return from_json(buffer.str());
}

void NodeRegistry::clear() {
    modules_.clear();
    name_to_module_id_.clear();
    next_module_id_ = 1;
    next_unique_id_ = 1;
}

// ============================================================================
// PROJECT REGISTRY IMPLEMENTATION
// ============================================================================

ProjectRegistry::ProjectRegistry(Arena& arena)
    : arena_(arena), type_registry_(arena), node_registry_(arena, type_registry_) {}

bool ProjectRegistry::rename_global_type(uint64_t type_id, std::string_view new_name) {
    // Rename in type registry
    if (!type_registry_.rename_type(type_id, new_name)) {
        return false;
    }
    
    // Update all references in nodes
    auto type_opt = type_registry_.get_type(type_id);
    if (!type_opt) return false;
    
    std::string new_name_str(new_name.data(), new_name.size());
    
    /*
    for (auto& module : node_registry_.get_modules()) {
        for (auto& node : module.nodes) {
            // Update parameter types
            for (auto& param : node.parameters) {
                // This would be handled more elegantly by looking up type references
            }
        }
    }
    */
    
    return true;
}

bool ProjectRegistry::rename_struct_member(uint64_t type_id, std::string_view old_member_name,
                                         std::string_view new_member_name) {
    if (!type_registry_.rename_member(type_id, old_member_name, new_member_name)) {
        return false;
    }
    
    // Note: Updating nodes would require semantic analysis to track which nodes
    // access this member. This is deferred to semantic analyzer.
    
    return true;
}

std::string ProjectRegistry::to_json() const {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(0);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);
    
    // Serialize types
    std::string types_json = type_registry_.to_json();
    yyjson_doc *types_doc = yyjson_read(types_json.c_str(), types_json.size(), 0);
    if (types_doc) {
        yyjson_val *types_root = yyjson_doc_get_root(types_doc);
        yyjson_mut_val *types_mut = yyjson_val_mut_copy(doc, types_root);
        yyjson_mut_obj_add_val(doc, root, "types", types_mut);
        yyjson_doc_free(types_doc);
    }
    
    // Serialize nodes
    std::string nodes_json = node_registry_.to_json();
    yyjson_doc *nodes_doc = yyjson_read(nodes_json.c_str(), nodes_json.size(), 0);
    if (nodes_doc) {
        yyjson_val *nodes_root = yyjson_doc_get_root(nodes_doc);
        yyjson_mut_val *nodes_mut = yyjson_val_mut_copy(doc, nodes_root);
        yyjson_mut_obj_add_val(doc, root, "nodes", nodes_mut);
        yyjson_doc_free(nodes_doc);
    }
    
    size_t json_size = 0;
    char *json_str = yyjson_mut_write(doc, 0, &json_size);
    std::string result(json_str, json_size);
    
    free(json_str);
    yyjson_mut_doc_free(doc);
    
    return result;
}

bool ProjectRegistry::from_json(std::string_view json_str) {
    yyjson_doc *doc = yyjson_read(json_str.data(), json_str.size(), 0);
    if (!doc) return false;
    
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) {
        yyjson_doc_free(doc);
        return false;
    }
    
    clear();
    
    // Load types
    yyjson_val *types_arr = yyjson_obj_get(root, "types");
    if (types_arr) {
        size_t json_size = 0;
        char *types_json = yyjson_val_write(types_arr, 0, &json_size);
        type_registry_.from_json(std::string_view(types_json, json_size));
        free(types_json);
    }
    
    // Load nodes
    yyjson_val *nodes_arr = yyjson_obj_get(root, "nodes");
    if (nodes_arr) {
        size_t json_size = 0;
        char *nodes_json = yyjson_val_write(nodes_arr, 0, &json_size);
        node_registry_.from_json(std::string_view(nodes_json, json_size));
        free(nodes_json);
    }
    
    yyjson_doc_free(doc);
    return true;
}

bool ProjectRegistry::save_to_file(std::string_view filename) const {
    std::string json = to_json();
    std::ofstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) return false;
    file.write(json.c_str(), json.size());
    return file.good();
}

bool ProjectRegistry::load_from_file(std::string_view filename) {
    std::ifstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) return false;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return from_json(buffer.str());
}

void ProjectRegistry::clear() {
    type_registry_.clear();
    node_registry_.clear();
}

} // namespace ibex
