#include <iostream>
#include <cassert>
#include "parser_new.h"
#include "lexer.h"
#include "semantic_analyzer.h"
#include "type_registry.h"

int main() {
    ibex::Arena arena;
    ibex::Program program(arena);
    
    std::string_view code = R"(
        flag Permissions {
            Read,
            Write,
            Execute
        }

        flag SuperPermissions : Permissions {
            Admin,
            Root = Admin or Execute
        }

        struct User {
            perms: Permissions;
        }
    )";
    
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
