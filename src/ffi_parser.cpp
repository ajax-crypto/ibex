#include "ffi_parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <tree_sitter/api.h>

extern "C" TSLanguage *tree_sitter_c();

namespace ibex {

FFIParser::FFIParser(const FFIConfig& config, Program& program) 
    : config_(config), program_(program) {}

TypeHandle FFIParser::map_c_type(const std::string& c_type, bool is_pointer) {
    TypeHandle base_handle;
    
    if (c_type == "int" || c_type == "int32_t") {
        PrimitiveType pt{TokenType::I32};
        program_.types.push_back(pt);
        base_handle = TypeHandle{static_cast<uint32_t>(program_.types.size() - 1)};
    } else if (c_type == "float") {
        PrimitiveType pt{TokenType::F32};
        program_.types.push_back(pt);
        base_handle = TypeHandle{static_cast<uint32_t>(program_.types.size() - 1)};
    } else if (c_type == "double") {
        PrimitiveType pt{TokenType::F64};
        program_.types.push_back(pt);
        base_handle = TypeHandle{static_cast<uint32_t>(program_.types.size() - 1)};
    } else if (c_type == "char" || c_type == "void") {
        PrimitiveType pt{TokenType::BYTE};
        program_.types.push_back(pt);
        base_handle = TypeHandle{static_cast<uint32_t>(program_.types.size() - 1)};
    } else {
        // Fallback to void pointer essentially
        PrimitiveType pt{TokenType::BYTE};
        program_.types.push_back(pt);
        base_handle = TypeHandle{static_cast<uint32_t>(program_.types.size() - 1)};
    }
    
    if (is_pointer) {
        PointerType ptr{base_handle};
        program_.types.push_back(ptr);
        return TypeHandle{static_cast<uint32_t>(program_.types.size() - 1)};
    }
    
    if (c_type == "void" && !is_pointer) {
        return TypeHandle{0xffffffff};
    }
    
    return base_handle;
}

std::unordered_map<std::string, FunctionType> FFIParser::parse_c_headers(const std::vector<DeclHandle>& foreign_blocks) {
    std::unordered_map<std::string, FunctionType> ffi_functions;
    
    // Early exit if no foreign blocks or no compiler configured
    if (foreign_blocks.empty() || config_.c_compiler_path.empty()) {
        return ffi_functions;
    }

    std::string temp_c_path = "ibex_ffi_temp.c";
    {
        std::ofstream temp_file(temp_c_path);
        for (auto handle : foreign_blocks) {
            auto& decl = program_.declarations[handle.index];
            if (auto* fb = std::get_if<ForeignBlockDecl>(&decl)) {
                std::string lang(fb->language.ptr(), fb->language.len());
                if (lang == "c") {
                    temp_file << std::string(fb->code.ptr(), fb->code.len()) << "\n";
                }
            }
        }
    }

    std::stringstream cmd;
    cmd << config_.c_compiler_path << " -E";
    for (const auto& inc : config_.include_paths) {
        cmd << " -I\"" << inc << "\"";
    }
    for (const auto& def : config_.definitions) {
        cmd << " -D\"" << def << "\"";
    }
    cmd << " " << temp_c_path;

    std::string preprocessed_code;
#ifdef _WIN32
    FILE* pipe = _popen(cmd.str().c_str(), "r");
#else
    FILE* pipe = popen(cmd.str().c_str(), "r");
#endif

    if (!pipe) {
        std::cerr << "Error: Failed to run C compiler for FFI preprocessing.\n";
        return ffi_functions;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        preprocessed_code += buffer;
    }

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    std::cout << "Parsing C code with tree-sitter\n";
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_c());

    TSTree *tree = ts_parser_parse_string(parser, nullptr, preprocessed_code.c_str(), preprocessed_code.length());
    TSNode root_node = ts_tree_root_node(tree);
    uint32_t count = ts_node_named_child_count(root_node);
    for (uint32_t i = 0; i < count; ++i) {
        TSNode node = ts_node_named_child(root_node, i);
        if (std::string(ts_node_type(node)) == "declaration") {
            TSNode declarator = ts_node_child_by_field_name(node, "declarator", 10);
            if (ts_node_is_null(declarator)) continue;
            
            if (std::string(ts_node_type(declarator)) == "function_declarator") {
                TSNode type_node = ts_node_child_by_field_name(node, "type", 4);
                std::string c_ret_type;
                if (!ts_node_is_null(type_node)) {
                    uint32_t start = ts_node_start_byte(type_node);
                    uint32_t end = ts_node_end_byte(type_node);
                    c_ret_type = preprocessed_code.substr(start, end - start);
                }
                
                TSNode name_node = ts_node_child_by_field_name(declarator, "declarator", 10);
                std::string func_name;
                bool ret_is_pointer = false;
                
                if (std::string(ts_node_type(name_node)) == "pointer_declarator") {
                    ret_is_pointer = true;
                    name_node = ts_node_child_by_field_name(name_node, "declarator", 10);
                }
                
                if (!ts_node_is_null(name_node) && std::string(ts_node_type(name_node)) == "identifier") {
                    uint32_t start = ts_node_start_byte(name_node);
                    uint32_t end = ts_node_end_byte(name_node);
                    func_name = preprocessed_code.substr(start, end - start);
                }
                
                if (!func_name.empty()) {
                    TSNode params_node = ts_node_child_by_field_name(declarator, "parameters", 10);
                    std::vector<TypeHandle> param_types;
                    
                    if (!ts_node_is_null(params_node)) {
                        uint32_t param_count = ts_node_named_child_count(params_node);
                        for (uint32_t p = 0; p < param_count; ++p) {
                            TSNode param_node = ts_node_named_child(params_node, p);
                            if (std::string(ts_node_type(param_node)) == "parameter_declaration") {
                                TSNode ptype_node = ts_node_child_by_field_name(param_node, "type", 4);
                                std::string p_c_type;
                                if (!ts_node_is_null(ptype_node)) {
                                    uint32_t start = ts_node_start_byte(ptype_node);
                                    uint32_t end = ts_node_end_byte(ptype_node);
                                    p_c_type = preprocessed_code.substr(start, end - start);
                                }
                                
                                bool p_is_pointer = false;
                                TSNode pdecl_node = ts_node_child_by_field_name(param_node, "declarator", 10);
                                if (!ts_node_is_null(pdecl_node)) {
                                    if (std::string(ts_node_type(pdecl_node)) == "pointer_declarator") {
                                        p_is_pointer = true;
                                    }
                                }
                                param_types.push_back(map_c_type(p_c_type, p_is_pointer));
                            }
                        }
                    }
                    
                    FunctionType ft;
                    ft.return_type = map_c_type(c_ret_type, ret_is_pointer);
                    ft.param_types = program_.allocate_array(param_types);
                    ffi_functions[func_name] = ft;
                }
            }
        }
    }

    ts_tree_delete(tree);
    ts_parser_delete(parser);
    std::remove(temp_c_path.c_str());

    return ffi_functions;
}

} // namespace ibex
