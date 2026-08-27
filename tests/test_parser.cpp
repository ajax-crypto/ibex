#include <iostream>
#include <cassert>
#include "parser_new.h"
#include "lexer.h"
#include "semantic_analyzer.h"
#include "type_registry.h"

#include <fstream>
#include <sstream>

int main(int argc, char** argv) {
    ibex::Arena arena;
    ibex::Program program(arena);
    
    std::string code_str;
    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "Could not open file: " << argv[1] << "\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        code_str = buffer.str();
    } else {
        code_str = R"(
            flag Permissions { Read, Write, Execute }
        )";
    }
    std::string_view code = code_str;
    
    std::cerr << "Lexing...\n" << std::flush;
    ibex::Lexer lexer(code);
    std::vector<ibex::Token> tokens;
    while (true) {
        auto tok = lexer.next_token();
        std::cerr << "Token: type=" << (int)tok.type << ", lexeme='" << tok.lexeme << "'\n";
        tokens.push_back(tok);
        if (tok.type == ibex::TokenType::EOF_TOKEN) break;
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
    ibex::TypeRegistry type_registry(arena);
    ibex::SemanticAnalyzer semantic(parser.program(), type_registry);
    semantic.analyze();
    
    if (semantic.has_errors()) {
        std::cerr << "Semantic analysis failed:\n";
        for (const auto& err : semantic.get_errors()) {
            std::cerr << err << "\n";
        }
        return 1;
    }
    
    std::cout << "Flag syntax and semantics test passed successfully!\n";
    return 0;
}
