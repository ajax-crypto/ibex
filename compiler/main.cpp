// Compiler main entry point
#include "lexer.h"
#include "parser.h"
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

    std::vector<std::unique_ptr<std::string>> file_contents;
    std::vector<ibex::Token> tokens;

    for (int i = 1; i < argc; ++i) {
        std::string input_file = argv[i];
        // Read input file
        std::ifstream file(input_file);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file: " << input_file << "\n";
            return 1;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file_contents.push_back(std::make_unique<std::string>(buffer.str()));
    }

    // Lexical analysis
    for (size_t i = 0; i < file_contents.size(); ++i) {
        ibex::Lexer lexer(*file_contents[i]);
        auto file_tokens = lexer.tokenize();
        
        if (!lexer.get_errors().empty()) {
            std::cerr << "Lexical errors in file " << argv[i+1] << ":\n";
            for (const auto& error : lexer.get_errors()) {
                std::cerr << "  " << error << "\n";
            }
            return 1;
        }

        for (auto tok : file_tokens) {
            if (tok.type == ibex::TokenType::EOF_TOKEN) {
                if (i == file_contents.size() - 1) {
                    tokens.push_back(tok);
                }
            } else {
                tokens.push_back(tok);
            }
        }
    }

    std::cout << "Tokenized " << tokens.size() << " tokens across " << file_contents.size() << " files\n";

    // Parsing
    ibex::Arena ast_arena;
    ibex::Program program(ast_arena);
    ibex::Parser parser(tokens, program);
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
