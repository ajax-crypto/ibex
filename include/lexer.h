#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>

namespace ibex {

// Token types enumeration
enum class TokenType {
    // Literals
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    RAW_STRING_LITERAL,
    TRUE_LITERAL,
    FALSE_LITERAL,
    NULL_LITERAL,

    // Keywords
    STRUCT,         // struct
    ENUM,           // enum
    FLAG,           // flag (bitset enum)
    USING,          // using (compile-time function binding)
    RETURN,         // return
    IF,             // if
    ELSE,           // else
    FOR,            // for
    IN,             // in (for-in loops)
    WHILE,          // while
    BREAK,          // break
    CONTINUE,       // continue
    STATIC,         // static
    CONST_KW,       // const
    AS,             // as (type casting)
    PACKAGE,        // package
    EXPORT,         // export
    MODULE,         // module
    IMPORT,         // import
    TYPEOF,         // typeof
    SIZEOF,         // sizeof

    // Primitive types
    I8, I16, I32, I64,
    U8, U16, U32, U64,
    BYTE,           // byte type (u8 alias)
    F32, F64,
    BOOL,
    TEXT,           // text type

    // Operators
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /
    PERCENT,        // %
    EQUAL,          // =
    COLON_EQUAL,    // := (type deduction)
    PLUS_EQUAL,     // +=
    MINUS_EQUAL,    // -=
    STAR_EQUAL,     // *=
    SLASH_EQUAL,    // /=
    EQ_EQ,          // ==
    NOT_EQ,         // !=
    LESS,           // <
    GREATER,        // >
    LESS_EQ,        // <=
    GREATER_EQ,     // >=
    AMPERSAND,      // &
    AT,             // @ (address-of, allocation)
    HASH,           // # (pound/hash - compile-time prefix)
    PIPE,           // |
    CARET,          // ^
    TILDE,          // ~
    AMPERSAND_AMPERSAND, // &&
    PIPE_PIPE,      // ||
    BANG,           // !
    LSHIFT,         // <<
    RSHIFT,         // >>
    DOT,            // .
    ARROW,          // ->
    COLON_COLON,    // ::

    // Delimiters
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    LBRACK,         // [
    RBRACK,         // ]
    LBRACKET_LBRACKET,  // [[ (attribute start)
    RBRACKET_RBRACKET,  // ]] (attribute end)
    SEMICOLON,      // ;
    COLON,          // :
    COMMA,          // ,
    RANGE_OP,       // .. (range operator)

    // Special
    IDENTIFIER,
    EOF_TOKEN,
    UNKNOWN,

    // Ellipsis
    DOT_DOT_DOT,    // ... (variadic)
};

// Token structure
struct Token {
    TokenType type;
    std::string lexeme;
    std::string_view line_content;
    uint32_t line;
    uint32_t column;
    
    // Value storage for literals
    union {
        int64_t int_value;
        double float_value;
    } value;
    bool is_float;

    Token() : type(TokenType::UNKNOWN), line(0), column(0), is_float(false) {}
    
    Token(TokenType t, std::string lex, uint32_t l, uint32_t c)
        : type(t), lexeme(std::move(lex)), line(l), column(c), is_float(false) {}
};

// Lexer class
class Lexer {
public:
    explicit Lexer(std::string_view source);
    
    // Get next token
    Token next_token();
    
    // Peek at next token without consuming
    Token peek_token() const;
    
    // Return all tokens from input
    std::vector<Token> tokenize();

    // Get error messages
    const std::vector<std::string>& get_errors() const { return errors_; }

private:
    std::string source_;
    size_t current_;
    size_t start_;
    uint32_t line_;
    uint32_t column_;
    std::vector<std::string> errors_;

    // Helper methods
    char current_char() const;
    char peek_char(size_t offset = 1) const;
    void advance();
    bool is_at_end() const;
    
    // Character categorization
    static bool is_alpha(char c);
    static bool is_digit(char c);
    static bool is_alphanumeric(char c);
    
    // Token creation
    Token make_token(TokenType type);
    Token make_literal_token(TokenType type);
    
    // Lexing helpers
    Token scan_number();
    Token scan_string();
    Token scan_raw_string();
    Token scan_identifier();
    
    // Keyword lookup
    static TokenType keyword_type(std::string_view lexeme);
};

} // namespace ibex
