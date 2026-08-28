// Copyright (c) 2026 Akash Pradhan
// SPDX-License-Identifier: MIT

#pragma once

#include "lexer.h"
#include "parser.h"
#include "const_eval.h"
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <set>

namespace ibex {

struct PackageInfo {
    std::string name;
    std::filesystem::path file_path;
};

struct ModuleParamInfo {
    std::string name;
    std::string type_name;  // "bool", "i32", etc.
};

struct ModuleInfo {
    std::string name;
    std::filesystem::path module_file_path;
    std::vector<std::string> exported_packages;
    std::vector<PackageInfo> packages;
    std::vector<ModuleParamInfo> parameters;  // Empty for non-parameterized
    std::vector<DeclHandle> raw_decls;        // Raw decls for re-evaluating conditional exports
};

class ModuleScanner {
public:
    ModuleScanner(Program& program, Arena& arena) : program_(program), arena_(arena) {}

    // Add a search path to the scanner
    void add_search_path(const std::filesystem::path& path, bool recursive);

    // Scan for modules and packages, parse .module.ibex files
    bool scan();

    // Get discovered modules
    const std::vector<ModuleInfo>& get_modules() const { return modules_; }
    
    // Get all source files to parse (excluding .module.ibex which are already parsed)
    std::vector<std::filesystem::path> get_source_files() const;

    // Evaluate conditional exports for a parameterized module given argument bindings
    std::vector<std::string> evaluate_conditional_exports(
        const ModuleInfo& mod,
        const std::unordered_map<std::string, ConstValue>& param_bindings);

    const std::vector<std::string>& get_errors() const { return errors_; }
    bool has_errors() const { return !errors_.empty(); }

private:
    void scan_directory(const std::filesystem::path& dir, bool recursive);
    bool parse_module_file(const std::filesystem::path& path, ModuleInfo& out_info);
    void scan_for_packages(ModuleInfo& mod_info, const std::filesystem::path& dir, bool recursive);

    // Circular dependency detection
    void build_dependency_graph();
    bool detect_cycles();

    struct SearchPath {
        std::filesystem::path path;
        bool recursive;
    };

    Program& program_;
    Arena& arena_;
    std::vector<SearchPath> search_paths_;
    std::vector<ModuleInfo> modules_;
    std::vector<std::string> errors_;

    // Dependency graph: module_name -> set of module_names it depends on
    std::unordered_map<std::string, std::set<std::string>> dependency_graph_;

    void report_error(const std::string& msg) {
        errors_.push_back(msg);
    }
};

} // namespace ibex
