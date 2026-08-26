// Example: Using the High-Performance Ibex Compiler Architecture
//
// This example demonstrates how to lex, parse, and process an Ibex program
// using the new arena-based, discriminated-union architecture.

#if 0  // This is pseudocode showing the API

#include "lexer.h"
#include "parser_new.h"
#include "ast_visitor.h"

int main() {
    using namespace ibex;

    // ========================================================================
    // Step 1: Lexing
    // ========================================================================
    
    const char* source = R"(
        add: (x: i32, y: i32) -> i32 {
            return x + y;
        }
        
        main: () -> i32 {
            result := add(10, 20);
            result
        }
    )";

    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    
    if (!lexer.get_errors().empty()) {
        for (const auto& err : lexer.get_errors()) {
            fprintf(stderr, "Lexer error: %s\n", err.c_str());
        }
        return 1;
    }

    // ========================================================================
    // Step 2: Parsing into Arena-Allocated AST
    // ========================================================================
    
    Arena arena;
    ParserNew parser(tokens, arena);
    
    auto decl_handles = parser.parse_program();
    
    if (parser.has_errors()) {
        for (const auto& err : parser.get_errors()) {
            fprintf(stderr, "Parser error: %s\n", err.c_str());
        }
        return 1;
    }

    const Program& program = parser.program();
    
    printf("Parsed %zu declarations\n", program.declarations.size());
    printf("Total expressions: %zu\n", program.expressions.size());
    printf("Total statements: %zu\n", program.statements.size());
    printf("Total types: %zu\n", program.types.size());
    printf("Arena memory: %zu bytes\n", arena.memory_usage());

    // ========================================================================
    // Step 3: AST Processing with Visitors
    // ========================================================================
    
    class PrintVisitor : public DeclVisitor {
    public:
        void visit(const FunctionDecl& d) override {
            printf("  Function: %.*s with %zu params -> type\n",
                   (int)d.name.len(), d.name.ptr(), d.parameters.size());
        }
        
        void visit(const VariableDecl& d) override {
            printf("  Variable: %.*s\n", (int)d.name.len(), d.name.ptr());
        }
        
        void visit(const StructDecl& d) override {
            printf("  Struct: %.*s with %zu members\n",
                   (int)d.name.len(), d.name.ptr(), d.members.size());
        }
        
        void visit(const ClassDecl& d) override {
            printf("  Class: %.*s\n", (int)d.name.len(), d.name.ptr());
        }
        
        void visit(const EnumDecl& d) override {
            printf("  Enum: %.*s with %zu variants\n",
                   (int)d.name.len(), d.name.ptr(), d.members.size());
        }
    };

    PrintVisitor visitor;
    printf("Declarations:\n");
    for (const auto& decl : program.declarations) {
        visit_decl(decl, &visitor);
    }

    // ========================================================================
    // Step 4: Pattern Matching Example (Alternative to Visitor)
    // ========================================================================
    
    printf("\nExpression Stats:\n");
    int call_count = 0;
    int binary_count = 0;
    
    for (const auto& expr : program.expressions) {
        if (std::get_if<CallExpr>(&expr)) {
            call_count++;
        }
        if (std::get_if<BinaryExpr>(&expr)) {
            binary_count++;
        }
    }
    
    printf("  Calls: %d\n", call_count);
    printf("  Binary ops: %d\n", binary_count);

    // ========================================================================
    // Step 5: Semantic Analysis (Example Type Checker)
    // ========================================================================
    
    class TypeChecker : public ExprVisitor {
    private:
        const Program& program_;
        std::unordered_map<uint32_t, TypeHandle> expr_types_;
        
    public:
        explicit TypeChecker(const Program& prog) : program_(prog) {}
        
        void visit(const BinaryExpr& expr) override {
            // In real implementation, would check type compatibility
            printf("  Binary: %d %d %d\n", expr.left.index, 
                   static_cast<int>(expr.op), expr.right.index);
        }
        
        void visit(const CallExpr& expr) override {
            // Check function exists and argument types match
            printf("  Call: func at %d with %zu args\n", 
                   expr.function.index, expr.arguments.size());
        }
        
        void visit(const LiteralExpr&) override {
            // Literal types are always known
        }
        
        void visit(const IdentifierExpr& expr) override {
            // Look up identifier in symbol table
            printf("  Identifier: %.*s\n", (int)expr.name.len(), expr.name.ptr());
        }
        
        void visit(const CastExpr& expr) override {
            printf("  Cast: expr at %d to type\n", expr.operand.index);
        }
        
        void visit(const MemberExpr& expr) override {
            printf("  Member: expr.%.*s\n", (int)expr.member.len(), expr.member.ptr());
        }
        
        void visit(const IndexExpr& expr) override {
            printf("  Index: expr[expr]\n");
        }
        
        void visit(const UnaryExpr& expr) override {
            printf("  Unary: %d\n", static_cast<int>(expr.op));
        }
        
        void visit(const BlockExpr& expr) override {
            printf("  Block: %zu statements\n", expr.statements.size());
        }
    };

    TypeChecker type_checker(program);
    printf("\nType Checking:\n");
    for (const auto& expr : program.expressions) {
        visit_expr(expr, &type_checker);
    }

    // ========================================================================
    // Step 6: Direct Access via Handles (Fast Path)
    // ========================================================================
    
    if (!decl_handles.empty() && !program.declarations.empty()) {
        DeclHandle first_decl = decl_handles[0];
        const Decl& decl = program.declarations[first_decl.index];
        
        if (auto* func = std::get_if<FunctionDecl>(&decl)) {
            printf("\nFirst Declaration Details:\n");
            printf("  Name: %.*s\n", (int)func->name.len(), func->name.ptr());
            printf("  Parameters: %zu\n", func->parameters.size());
            printf("  Body: statement %d\n", func->body.index);
        }
    }

    // ========================================================================
    // Step 7: Memory Efficiency Summary
    // ========================================================================
    
    printf("\nMemory Summary:\n");
    printf("  Arena chunks: Up to %zu bytes\n", arena.memory_usage());
    printf("  Declaration count: %zu\n", program.declarations.size());
    printf("  Total expression nodes: %zu\n", program.expressions.size());
    printf("  String storage: In arena (zero-copy views)\n");

    return 0;
}

// ============================================================================
// KEY DESIGN PATTERNS
// ============================================================================

/*
PATTERN 1: Using Handles Instead of Pointers

Old (Dynamic Dispatch):
    std::unique_ptr<Expression> expr = std::make_unique<BinaryExpr>(...);
    if (auto* bin = dynamic_cast<BinaryExpr*>(expr.get())) { ... }

New (Direct Access):
    ExprHandle handle = store_expr(BinaryExpr{...});
    BinaryExpr& bin = std::get<BinaryExpr>(program.expressions[handle.index]);

PATTERN 2: Using Visitor Pattern with std::visit

    std::visit([](const auto& expr) {
        if constexpr (std::is_same_v<decltype(expr), const BinaryExpr&>) {
            // Handle binary
        }
    }, program.expressions[handle.index]);

PATTERN 3: Using if constexpr for Compile-Time Specialization

    if (auto* ptr = std::get_if<BinaryExpr>(&expr)) {
        // Safe optional access
        process_binary(*ptr);
    }

PATTERN 4: Array Allocation in Arena

    std::vector<StmtHandle> stmts;
    // ... populate stmts
    std::span<StmtHandle> span = program.allocate_array(stmts);
    BlockStmt block{span};

PATTERN 5: String Allocation in Arena

    Str name = program.allocate_string("my_function");
    FunctionDecl func{.name = name, ...};
*/

#endif  // End of pseudocode example
