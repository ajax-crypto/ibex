// Copyright (c) 2026 Akash Pradhan
// SPDX-License-Identifier: MIT

#include "node_registry.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>
#include <type_traits>
#include <utility>
#include <ranges>
#include <glaze/glaze.hpp>

namespace ibex {

// Glaze Mirror Structs
struct GlazeNodeParameter {
    std::string name;
    uint64_t type_id;
    std::optional<std::string> default_value;
};

struct GlazeNodeOutput {
    std::string name;
    uint64_t type_id;
};

struct GlazeNodeInvocation {
    uint32_t target_node_id;
    uint32_t argument_index;
};

struct GlazeNode {
    uint32_t id;
    Node::Kind kind;
    std::string name;
    std::vector<GlazeNodeParameter> parameters;
    GlazeNodeOutput output;
    std::vector<GlazeNodeInvocation> invocations;
    std::optional<std::string> source_location;
    uint64_t unique_id;
};

struct GlazeModule {
    std::string name;
    std::vector<GlazeNode> nodes;
    uint64_t unique_id;
};

} // namespace ibex

template <> struct glz::meta<ibex::GlazeNodeParameter> {
    using T = ibex::GlazeNodeParameter;
    static constexpr auto value = object("name", &T::name, "type_id", &T::type_id, "default_value", &T::default_value);
};
template <> struct glz::meta<ibex::GlazeNodeOutput> {
    using T = ibex::GlazeNodeOutput;
    static constexpr auto value = object("name", &T::name, "type_id", &T::type_id);
};
template <> struct glz::meta<ibex::GlazeNodeInvocation> {
    using T = ibex::GlazeNodeInvocation;
    static constexpr auto value = object("target_node_id", &T::target_node_id, "argument_index", &T::argument_index);
};
template <> struct glz::meta<ibex::GlazeNode> {
    using T = ibex::GlazeNode;
    static constexpr auto value = object(
        "id", &T::id, "kind", &T::kind, "name", &T::name,
        "parameters", &T::parameters, "output", &T::output,
        "invocations", &T::invocations, "source_location", &T::source_location, "unique_id", &T::unique_id
    );
};
template <> struct glz::meta<ibex::GlazeModule> {
    using T = ibex::GlazeModule;
    static constexpr auto value = object("name", &T::name, "nodes", &T::nodes, "unique_id", &T::unique_id);
};

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
    
    Module& module = **module_opt;
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
    
    Module& module = **module_opt;
    auto node_opt = module.find_node_by_id(node_id);
    if (!node_opt) return false;
    
    Node& node = **node_opt;
    
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
    
    Node& node = **node_opt;
    
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
    
    Node& node = **node_opt;
    node.output.type_id = new_type_id;
    
    return true;
}

bool NodeRegistry::add_parameter(uint64_t module_id, uint32_t node_id, const NodeParameter& param) {
    auto node_opt = get_node(module_id, node_id);
    if (!node_opt) return false;
    
    Node& node = **node_opt;
    
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
    std::vector<GlazeModule> glaze_modules;
    glaze_modules.reserve(modules_.size());
    
    for (const auto& module : modules_) {
        GlazeModule gm;
        gm.name = std::string(module.name.ptr(), module.name.len());
        gm.unique_id = module.unique_id;
        
        for (const auto& node : module.nodes) {
            GlazeNode gn;
            gn.id = node.id;
            gn.kind = node.kind;
            gn.name = std::string(node.name.ptr(), node.name.len());
            gn.unique_id = node.unique_id;
            if (node.source_location) gn.source_location = std::string(node.source_location->ptr(), node.source_location->len());
            
            gn.output.name = std::string(node.output.name.ptr(), node.output.name.len());
            gn.output.type_id = node.output.type_id;
            
            for (const auto& param : node.parameters) {
                GlazeNodeParameter gp;
                gp.name = std::string(param.name.ptr(), param.name.len());
                gp.type_id = param.type_id;
                if (param.default_value) gp.default_value = std::string(param.default_value->ptr(), param.default_value->len());
                gn.parameters.push_back(std::move(gp));
            }
            
            for (const auto& inv : node.invocations) {
                gn.invocations.push_back({inv.target_node_id, inv.argument_index});
            }
            
            gm.nodes.push_back(std::move(gn));
        }
        
        glaze_modules.push_back(std::move(gm));
    }
    
    std::string json;
    glz::write_json(glaze_modules, json);
    return json;
}

bool NodeRegistry::from_json(std::string_view json_str) {
    std::vector<GlazeModule> glaze_modules;
    auto ec = glz::read_json(glaze_modules, json_str);
    if (ec) return false;
    
    clear();
    
    for (const auto& gm : glaze_modules) {
        Module module;
        module.name = alloc_str(gm.name);
        module.unique_id = gm.unique_id;
        next_module_id_ = std::max(next_module_id_, module.unique_id + 1);
        
        for (const auto& gn : gm.nodes) {
            Node node;
            node.id = gn.id;
            node.kind = gn.kind;
            node.name = alloc_str(gn.name);
            node.unique_id = gn.unique_id;
            next_unique_id_ = std::max(next_unique_id_, node.unique_id + 1);
            if (gn.source_location) node.source_location = alloc_str(*gn.source_location);
            
            node.output.name = alloc_str(gn.output.name);
            node.output.type_id = gn.output.type_id;
            
            for (const auto& gp : gn.parameters) {
                NodeParameter param;
                param.name = alloc_str(gp.name);
                param.type_id = gp.type_id;
                if (gp.default_value) param.default_value = alloc_str(*gp.default_value);
                node.parameters.push_back(param);
            }
            
            for (const auto& gi : gn.invocations) {
                node.invocations.push_back({gi.target_node_id, gi.argument_index});
            }
            
            module.name_to_node_id[std::string(node.name.ptr(), node.name.len())] = static_cast<uint32_t>(module.nodes.size());
            module.nodes.push_back(node);
        }
        
        name_to_module_id_[std::string(module.name.ptr(), module.name.len())] = module.unique_id;
        modules_.push_back(module);
    }
    
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
// COMBINED REGISTRY IMPLEMENTATION
// ============================================================================

ProjectRegistry::ProjectRegistry(Arena& arena)
    : arena_(arena), type_registry_(arena), node_registry_(arena, type_registry_) {}

bool ProjectRegistry::rename_global_type(uint64_t type_id, std::string_view new_name) {
    if (!type_registry_.rename_type(type_id, new_name)) return false;
    return true;
}

bool ProjectRegistry::rename_struct_member(uint64_t type_id, std::string_view old_member_name, 
                                         std::string_view new_member_name) {
    if (!type_registry_.rename_member(type_id, old_member_name, new_member_name)) {
        return false;
    }
    return true;
}

std::string ProjectRegistry::to_json() const {
    return std::string(R"({"types":)") + type_registry_.to_json() + R"(,"nodes":)" + node_registry_.to_json() + "}";
}

struct GlazeProject {
    glz::raw_json types;
    glz::raw_json nodes;
};
} // namespace ibex

template <> struct glz::meta<ibex::GlazeProject> {
    using T = ibex::GlazeProject;
    static constexpr auto value = object("types", &T::types, "nodes", &T::nodes);
};

namespace ibex {

bool ProjectRegistry::from_json(std::string_view json_str) {
    GlazeProject project;
    auto ec = glz::read_json(project, json_str);
    if (ec) return false;
    
    clear();
    
    if (!project.types.str.empty()) {
        if (!type_registry_.from_json(project.types.str)) return false;
    }
    
    if (!project.nodes.str.empty()) {
        if (!node_registry_.from_json(project.nodes.str)) return false;
    }
    
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
