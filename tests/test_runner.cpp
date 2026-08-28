// Copyright (c) 2026 Akash Pradhan
// SPDX-License-Identifier: MIT

#include "module_scanner.h"
#include <iostream>
#include <cassert>
#include "parser.h"
#include "lexer.h"
#include "semantic_analyzer.h"
#include "type_registry.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <crtdbg.h>
#include <stdlib.h>
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    // Redirect all CRT asserts/errors to stderr instead of dialog boxes
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif

    ibex::Arena arena;
    ibex::Program program(arena);
    
    ibex::ModuleScanner scanner(program, arena);
    
    std::vector<std::string> input_files;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--import" && i + 1 < argc) {
            scanner.add_search_path(argv[++i], false);
        } else if (arg == "--import-recursive" && i + 1 < argc) {
            scanner.add_search_path(argv[++i], true);
        } else {
            input_files.push_back(arg);
        }
    }
    
    if (const char* env_path = std::getenv("IBEX_MODULE_PATH")) {
        bool recursive = false;
        if (const char* env_rec = std::getenv("IBEX_MODULE_RECURSE")) {
            if (std::string(env_rec) == "1") recursive = true;
        }
        
        std::stringstream ss(env_path);
        std::string path;
        while (std::getline(ss, path, ';')) {
            if (!path.empty()) scanner.add_search_path(path, recursive);
        }
    }
    
    std::cerr << "Scanning modules...\n";
    if (!scanner.scan()) {
        std::cerr << "Module scanning failed:\n";
        for (const auto& err : scanner.get_errors()) {
            std::cerr << err << "\n";
        }
        return 1;
    }
    
    auto mod_files = scanner.get_source_files();
    for (const auto& f : mod_files) {
        input_files.push_back(f.string());
    }
    
    std::vector<std::unique_ptr<std::string>> file_contents;
    std::vector<ibex::Token> tokens;
    
    if (!input_files.empty()) {
        for (const auto& file_path : input_files) {
            std::ifstream file(file_path);
            if (!file.is_open()) {
                std::cerr << "Could not open file: " << file_path << "\n";
                return 1;
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            file_contents.push_back(std::make_unique<std::string>(buffer.str()));
        }
    } else {
        file_contents.push_back(std::make_unique<std::string>("flag Permissions { Read, Write, Execute }"));
    }

    std::cerr << "Lexing...\n" << std::flush;
    for (size_t i = 0; i < file_contents.size(); ++i) {
        ibex::Lexer lexer(*file_contents[i]);
        while (true) {
            auto tok = lexer.next_token();
            if (tok.type == ibex::TokenType::EOF_TOKEN) {
                if (i == file_contents.size() - 1) {
                    tokens.push_back(tok);
                }
                break;
            }
            // std::cerr << "Token: type=" << (int)tok.type << ", lexeme='" << tok.lexeme << "'\n";
            tokens.push_back(tok);
        }
    }

    std::cerr << "Parsing...\n" << std::flush;

    ibex::Parser parser(tokens, program);
    parser.parse_program();
    
    if (parser.has_errors()) {
        std::cerr << "Syntax analysis failed:\n";
        for (const auto& err : parser.get_errors()) {
            std::cerr << err << "\n";
        }
        return 1;
    }

    std::cerr << "Semantic analysis...\n" << std::flush;
    try {
        ibex::TypeRegistry type_registry(arena);
        ibex::SemanticAnalyzer semantic(program, type_registry);
        semantic.analyze();
        
        // Print any warnings
        for (const auto& warn : semantic.get_warnings()) {
            std::cerr << "Warning: " << warn << "\n";
        }
        
        if (semantic.has_errors()) {
            std::cerr << "Semantic analysis failed:\n";
            for (const auto& err : semantic.get_errors()) {
                std::cerr << err << "\n";
            }
            return 1;
        }
        
        std::cerr << "Syntax and semantics test passed successfully!\n";
    } catch (const std::out_of_range& e) {
        std::cerr << "EXCEPTION (out_of_range): " << e.what() << "\n";
        return 1;
    } catch (const std::bad_variant_access& e) {
        std::cerr << "EXCEPTION (bad_variant_access): " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
