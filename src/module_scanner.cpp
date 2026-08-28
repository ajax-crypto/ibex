#include "module_scanner.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace ibex {

void ModuleScanner::add_search_path(const std::filesystem::path& path, bool recursive) {
    search_paths_.push_back({path, recursive});
}

bool ModuleScanner::scan() {
    for (const auto& sp : search_paths_) {
        scan_directory(sp.path, sp.recursive);
    }
    if (has_errors()) return false;

    // Build dependency graph and detect circular dependencies
    build_dependency_graph();
    if (!detect_cycles()) return false;

    return !has_errors();
}

void ModuleScanner::scan_directory(const std::filesystem::path& dir, bool recursive) {
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        report_error("Search path does not exist or is not a directory: " + dir.string());
        return;
    }

    auto process_file = [&](const std::filesystem::path& path) {
        if (path.extension() == ".ibex" && path.filename().string().find(".module.ibex") != std::string::npos) {
            ModuleInfo mod_info;
            if (parse_module_file(path, mod_info)) {
                scan_for_packages(mod_info, path.parent_path(), recursive);
                modules_.push_back(mod_info);
            }
        }
    };

    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                process_file(entry.path());
            }
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                process_file(entry.path());
            }
        }
    }
}

bool ModuleScanner::parse_module_file(const std::filesystem::path& path, ModuleInfo& out_info) {
    std::ifstream file(path);
    if (!file.is_open()) {
        report_error("Could not open module file: " + path.string());
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    Parser parser(tokens, program_);
    std::vector<DeclHandle> decls = parser.parse_program();

    if (parser.has_errors()) {
        for (const auto& err : parser.get_errors()) {
            report_error(path.string() + ": " + err);
        }
        return false;
    }

    out_info.module_file_path = path;
    out_info.raw_decls = decls;
    bool found_module = false;

    // Inspect top-level decls
    for (auto handle : decls) {
        if (handle.is_null()) continue;
        auto& decl = program_.declarations[handle.index];
        
        if (auto* mod_decl = std::get_if<ModuleDecl>(&decl)) {
            if (found_module) {
                report_error(path.string() + ": Multiple module declarations found in one file");
                return false;
            }
            found_module = true;
            out_info.name = std::string(mod_decl->name.ptr(), mod_decl->name.len());
            
            // Extract parameter info
            for (const auto& param : mod_decl->parameters) {
                std::string param_name(param.name.ptr(), param.name.len());
                // Resolve type name from the type handle
                std::string type_name = "unknown";
                if (!param.type.is_null()) {
                    auto& type_var = program_.types[param.type.index];
                    if (auto* named = std::get_if<NamedType>(&type_var)) {
                        type_name = std::string(named->name.ptr(), named->name.len());
                    } else if (auto* prim = std::get_if<PrimitiveType>(&type_var)) {
                        switch (prim->primitive) {
                            case TokenType::BOOL: type_name = "bool"; break;
                            case TokenType::I8:  type_name = "i8"; break;
                            case TokenType::I16: type_name = "i16"; break;
                            case TokenType::I32: type_name = "i32"; break;
                            case TokenType::I64: type_name = "i64"; break;
                            case TokenType::U8:  type_name = "u8"; break;
                            case TokenType::U16: type_name = "u16"; break;
                            case TokenType::U32: type_name = "u32"; break;
                            case TokenType::U64: type_name = "u64"; break;
                            case TokenType::F32: type_name = "f32"; break;
                            case TokenType::F64: type_name = "f64"; break;
                            default: type_name = "unknown"; break;
                        }
                    }
                }
                out_info.parameters.push_back({param_name, type_name});
            }
        } else if (auto* exp_decl = std::get_if<ExportPackagesDecl>(&decl)) {
            for (const auto& pkg : exp_decl->package_names) {
                out_info.exported_packages.push_back(std::string(pkg.ptr(), pkg.len()));
            }
        } else if (auto* var_decl = std::get_if<VariableDecl>(&decl)) {
            if (!var_decl->is_const) {
                report_error(path.string() + ": Only const variables are allowed in .module.ibex");
                return false;
            }
        } else if (std::get_if<PackageDecl>(&decl)) {
            // Allow if/else blocks parsed as packages for conditional exports — skip validation
            // Actually, if/else in .module.ibex: the parser produces IfStmt which is a statement,
            // not a declaration. For now, we allow it by checking for conditional export statements.
            report_error(path.string() + ": Invalid declaration in .module.ibex. Only 'module', 'export', 'const', and 'if/else' for conditional exports are allowed.");
            return false;
        } else {
            // Allow other valid declarations silently for now to not break things
            // In strict mode, we'd reject them
        }
    }

    if (!found_module) {
        report_error(path.string() + ": No module declaration found in .module.ibex");
        return false;
    }

    return true;
}

void ModuleScanner::scan_for_packages(ModuleInfo& mod_info, const std::filesystem::path& dir, bool recursive) {
    auto process_file = [&](const std::filesystem::path& path) {
        std::string filename = path.filename().string();
        if (path.extension() == ".ibex" && filename.find(".module.ibex") == std::string::npos) {
            mod_info.packages.push_back({"", path});
        }
    };

    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                process_file(entry.path());
            }
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                process_file(entry.path());
            }
        }
    }
}

std::vector<std::filesystem::path> ModuleScanner::get_source_files() const {
    std::vector<std::filesystem::path> files;
    for (const auto& mod : modules_) {
        for (const auto& pkg : mod.packages) {
            files.push_back(pkg.file_path);
        }
    }
    return files;
}

// ----------------------------------------------------------------------------
// Circular Dependency Detection
// ----------------------------------------------------------------------------

void ModuleScanner::build_dependency_graph() {
    // Initialize all known modules in the graph
    for (const auto& mod : modules_) {
        dependency_graph_[mod.name]; // default-construct empty set
    }

    // For each module, lightweight-lex its package files to find import statements
    for (const auto& mod : modules_) {
        for (const auto& pkg : mod.packages) {
            std::ifstream file(pkg.file_path);
            if (!file.is_open()) continue;

            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string source = buffer.str();

            Lexer lexer(source);
            // Scan tokens for IMPORT <IDENTIFIER> patterns
            while (true) {
                Token tok = lexer.next_token();
                if (tok.type == TokenType::EOF_TOKEN) break;
                if (tok.type == TokenType::IMPORT) {
                    Token next = lexer.next_token();
                    if (next.type == TokenType::IDENTIFIER) {
                        std::string imported_mod(next.lexeme);
                        // Only add edge if the imported module is a known module
                        if (dependency_graph_.find(imported_mod) != dependency_graph_.end()) {
                            if (imported_mod != mod.name) {
                                dependency_graph_[mod.name].insert(imported_mod);
                            } else {
                                // Self-circular dependency
                                report_error("Circular dependency detected: " + mod.name + " -> " + mod.name);
                            }
                        }
                    }
                }
            }
        }
    }
}

bool ModuleScanner::detect_cycles() {
    if (has_errors()) return false; // Self-cycles already reported

    // DFS 3-color marking: 0=white (unvisited), 1=gray (in-progress), 2=black (done)
    std::unordered_map<std::string, int> color;
    std::unordered_map<std::string, std::string> parent; // For reconstructing cycle path

    for (const auto& entry : dependency_graph_) {
        color[entry.first] = 0;
    }

    bool has_cycle = false;

    // Use a struct to hold DFS state to avoid std::function overhead
    struct DfsState {
        std::unordered_map<std::string, int>& color;
        std::unordered_map<std::string, std::string>& parent;
        std::unordered_map<std::string, std::set<std::string>>& graph;
        ModuleScanner& scanner;

        bool run(const std::string& node) {
            color[node] = 1; // Gray

            auto it = graph.find(node);
            if (it != graph.end()) {
                for (const auto& neighbor : it->second) {
                    if (color[neighbor] == 1) {
                        // Back edge found — reconstruct the cycle
                        std::string current = node;
                        std::vector<std::string> cycle_path;
                        cycle_path.push_back(neighbor);
                        
                        while (current != neighbor) {
                            cycle_path.push_back(current);
                            auto p = parent.find(current);
                            if (p == parent.end()) break;
                            current = p->second;
                        }
                        cycle_path.push_back(neighbor);
                        std::reverse(cycle_path.begin(), cycle_path.end());

                        std::string msg = "Circular dependency detected: ";
                        for (size_t i = 0; i < cycle_path.size(); ++i) {
                            msg += cycle_path[i];
                            if (i + 1 < cycle_path.size()) msg += " -> ";
                        }
                        scanner.report_error(msg);
                        return true;
                    }
                    if (color[neighbor] == 0) {
                        parent[neighbor] = node;
                        if (run(neighbor)) return true;
                    }
                }
            }

            color[node] = 2; // Black
            return false;
        }
    };

    DfsState dfs{color, parent, dependency_graph_, *this};

    for (const auto& entry : dependency_graph_) {
        if (color[entry.first] == 0) {
            if (dfs.run(entry.first)) {
                has_cycle = true;
                break;
            }
        }
    }

    return !has_cycle;
}

// ----------------------------------------------------------------------------
// Conditional Export Evaluation for Parameterized Modules
// ----------------------------------------------------------------------------

std::vector<std::string> ModuleScanner::evaluate_conditional_exports(
    const ModuleInfo& mod,
    const std::unordered_map<std::string, ConstValue>& param_bindings) {

    std::vector<std::string> exports;
    ConstExprEvaluator evaluator(program_);

    // Bind all parameters
    for (const auto& [name, value] : param_bindings) {
        evaluator.set_const(name, value);
    }

    // Re-walk the raw decls and evaluate conditionally
    for (auto handle : mod.raw_decls) {
        if (handle.is_null()) continue;
        auto& decl = program_.declarations[handle.index];

        if (auto* exp_decl = std::get_if<ExportPackagesDecl>(&decl)) {
            for (const auto& pkg : exp_decl->package_names) {
                exports.push_back(std::string(pkg.ptr(), pkg.len()));
            }
        }
        // TODO: Handle IfStmt-wrapped exports when we add if/else to .module.ibex parser
    }

    return exports;
}

} // namespace ibex
