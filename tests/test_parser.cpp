#include <iostream>
#include <cassert>
#include "parser_new.h"
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
    
    std::vector<std::unique_ptr<std::string>> file_contents;
    std::vector<ibex::Token> tokens;
    
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::ifstream file(argv[i]);
            if (!file.is_open()) {
                std::cerr << "Could not open file: " << argv[i] << "\n";
                return 1;
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            file_contents.push_back(std::make_unique<std::string>(buffer.str()));
        }
    } else {
        file_contents.push_back(std::make_unique<std::string>(R"(
            flag Permissions { Read, Write, Execute }
        )"));
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
    ibex::ParserNew parser(tokens, arena);
    parser.parse_program();
    bool success = !parser.has_errors();
    
    if (!success) {
        std::cerr << "Parser failed!\n";
        for (const auto& err : parser.get_errors()) {
            std::cerr << err << "\n";
        }
        return 1;
    }
    
    std::cerr << "Semantic analysis...\n" << std::flush;
    try {
        ibex::TypeRegistry type_registry(arena);
        ibex::SemanticAnalyzer semantic(parser.program(), type_registry);
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
