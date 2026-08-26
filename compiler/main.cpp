// Compiler main entry point
#include "lexer.h"
#include "parser_new.h"
#include "type_registry.h"
#include "semantic_analyzer.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ibexc <input_file> [options]\n";
        std::cerr << "  -o <output>  Output file path\n";
        return 1;
    }

    std::string input_file = argv[1];

    // Read input file
    std::ifstream file(input_file);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file: " << input_file << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Lexical analysis
    ibex::Lexer lexer(source);
    std::vector<ibex::Token> tokens = lexer.tokenize();

    if (!lexer.get_errors().empty()) {
        std::cerr << "Lexical errors:\n";
        for (const auto& error : lexer.get_errors()) {
            std::cerr << "  " << error << "\n";
        }
        return 1;
    }

    std::cout << "Tokenized " << tokens.size() << " tokens\n";

    // Parsing
    ibex::Arena ast_arena;
    ibex::ParserNew parser(tokens, ast_arena);
    auto ast = parser.parse_program();

    if (!parser.get_errors().empty()) {
        std::cerr << "Parse errors:\n";
        for (const auto& error : parser.get_errors()) {
            std::cerr << "  " << error << "\n";
        }
        return 1;
    }

    std::cout << "Parsed " << ast.size() << " declarations\n";

    // Initialize type registry
    ibex::TypeRegistry registry(parser.program().arena());
    std::cout << "Type registry initialized\n";

    // Semantic analysis
    ibex::SemanticAnalyzer semantic_analyzer(parser.program(), registry);
    if (!semantic_analyzer.analyze()) {
        std::cerr << "Semantic errors:\n";
        for (const auto& error : semantic_analyzer.get_errors()) {
            std::cerr << "  " << error << "\n";
        }
        return 1;
    }
    std::cout << "Semantic analysis complete without errors\n";

    // TODO: Semantic analysis
    // TODO: Code generation
    // TODO: Link with C runtime

    std::cout << "Compilation successful (preliminary)\n";
    return 0;
}
