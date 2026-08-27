#include "lexer.h"
#include <cctype>
#include <stdexcept>

namespace ibex {

Lexer::Lexer(std::string_view source)
    : source_(source), current_(0), start_(0), line_(1), column_(0) {}

char Lexer::current_char() const {
    if (is_at_end()) return '\0';
    return source_[current_];
}

char Lexer::peek_char(size_t offset) const {
    size_t pos = current_ + offset;
    if (pos >= source_.size()) return '\0';
    return source_[pos];
}

void Lexer::advance() {
    if (!is_at_end()) {
        if (source_[current_] == '\n') {
            line_++;
            column_ = 0;
        } else {
            column_++;
        }
        current_++;
    }
}

bool Lexer::is_at_end() const {
    return current_ >= source_.size();
}

bool Lexer::is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool Lexer::is_alphanumeric(char c) {
    return is_alpha(c) || is_digit(c);
}

Token Lexer::make_token(TokenType type) {
    std::string lexeme(source_.substr(start_, current_ - start_));
    return Token(type, lexeme, line_, column_);
}

Token Lexer::make_literal_token(TokenType type) {
    Token token = make_token(type);
    return token;
}

Token Lexer::scan_number() {
    while (is_digit(current_char())) {
        advance();
    }

    // Check for float
    if (current_char() == '.' && is_digit(peek_char())) {
        advance();  // consume '.'
        while (is_digit(current_char())) {
            advance();
        }
        return make_literal_token(TokenType::FLOAT_LITERAL);
    }

    return make_literal_token(TokenType::INTEGER_LITERAL);
}

Token Lexer::scan_string() {
    advance();  // consume opening quote
    
    while (!is_at_end() && current_char() != '"') {
        if (current_char() == '\\') {
            advance();  // consume escape char
        }
        advance();
    }

    if (is_at_end()) {
        errors_.push_back("Unterminated string literal");
        return make_token(TokenType::UNKNOWN);
    }

    advance();  // consume closing quote
    return make_literal_token(TokenType::STRING_LITERAL);
}

Token Lexer::scan_raw_string() {
    advance(); advance(); advance(); // consume '''
    
    while (!is_at_end()) {
        if (current_char() == '\'' && peek_char() == '\'' && peek_char(2) == '\'') {
            break;
        }
        advance();
    }

    if (is_at_end()) {
        errors_.push_back("Unterminated raw string literal");
        return make_token(TokenType::UNKNOWN);
    }

    advance(); advance(); advance(); // consume closing '''
    return make_literal_token(TokenType::RAW_STRING_LITERAL);
}

Token Lexer::scan_identifier() {
    while (is_alphanumeric(current_char())) {
        advance();
    }

    if (current_char() == '"') {
        return scan_string();
    }
    if (current_char() == '\'' && peek_char() == '\'' && peek_char(2) == '\'') {
        return scan_raw_string();
    }

    std::string lexeme(source_.substr(start_, current_ - start_));
    TokenType type = keyword_type(lexeme);
    return make_token(type);
}

TokenType Lexer::keyword_type(std::string_view lexeme) {
    static const std::unordered_map<std::string_view, TokenType> keywords = {
        {"struct", TokenType::STRUCT},
        {"enum", TokenType::ENUM},
        {"flag", TokenType::FLAG},
        {"using", TokenType::USING},
        {"return", TokenType::RETURN},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"for", TokenType::FOR},
        {"in", TokenType::IN},
        {"while", TokenType::WHILE},
        {"break", TokenType::BREAK},
        {"continue", TokenType::CONTINUE},
        {"static", TokenType::STATIC},
        {"const", TokenType::CONST_KW},
        {"var", TokenType::VAR},
        {"as", TokenType::AS},
        {"package", TokenType::PACKAGE},
        {"export", TokenType::EXPORT},
        {"module", TokenType::MODULE},
        {"import", TokenType::IMPORT},
        {"typeof", TokenType::TYPEOF},
        {"sizeof", TokenType::SIZEOF},
        {"true", TokenType::TRUE_LITERAL},
        {"false", TokenType::FALSE_LITERAL},
        {"null", TokenType::NULL_LITERAL},
        {"i8", TokenType::I8},
        {"i16", TokenType::I16},
        {"i32", TokenType::I32},
        {"i64", TokenType::I64},
        {"u8", TokenType::U8},
        {"u16", TokenType::U16},
        {"u32", TokenType::U32},
        {"u64", TokenType::U64},
        {"byte", TokenType::BYTE},
        {"f32", TokenType::F32},
        {"f64", TokenType::F64},
        {"bool", TokenType::BOOL},
        {"text", TokenType::TEXT},
        {"or", TokenType::PIPE},
        {"and", TokenType::AMPERSAND},
        {"xor", TokenType::CARET},
        {"complement", TokenType::TILDE},
    };

    auto it = keywords.find(lexeme);
    if (it != keywords.end()) {
        return it->second;
    }
    return TokenType::IDENTIFIER;
}

Token Lexer::next_token() {
    // Skip whitespace
    while (!is_at_end() && std::isspace(current_char())) {
        advance();
    }

    // Skip comments
    if (current_char() == '/' && peek_char() == '/') {
        while (!is_at_end() && current_char() != '\n') {
            advance();
        }
        return next_token();  // Get next real token
    }

    if (is_at_end()) {
        return make_token(TokenType::EOF_TOKEN);
    }

    start_ = current_;
    char c = current_char();

    if (is_digit(c)) {
        return scan_number();
    }

    if (c == '"') {
        return scan_string();
    }
    
    if (c == '\'' && peek_char() == '\'' && peek_char(2) == '\'') {
        return scan_raw_string();
    }

    if (is_alpha(c)) {
        return scan_identifier();
    }

    advance();

    // Two-character operators
    if (c == '+') {
        if (current_char() == '=') {
            advance();
            return make_token(TokenType::PLUS_EQUAL);
        }
        return make_token(TokenType::PLUS);
    }

    if (c == '-') {
        if (current_char() == '=') {
            advance();
            return make_token(TokenType::MINUS_EQUAL);
        }
        if (current_char() == '>') {
            advance();
            return make_token(TokenType::ARROW);
        }
        return make_token(TokenType::MINUS);
    }

    if (c == '*') {
        if (current_char() == '=') {
            advance();
            return make_token(TokenType::STAR_EQUAL);
        }
        return make_token(TokenType::STAR);
    }

    if (c == '/') {
        if (current_char() == '=') {
            advance();
            return make_token(TokenType::SLASH_EQUAL);
        }
        return make_token(TokenType::SLASH);
    }

    if (c == '=') {
        if (current_char() == '=') {
            advance();
            return make_token(TokenType::EQ_EQ);
        }
        return make_token(TokenType::EQUAL);
    }

    if (c == '!') {
        if (current_char() == '=') {
            advance();
            return make_token(TokenType::NOT_EQ);
        }
        return make_token(TokenType::BANG);
    }

    if (c == '<') {
        if (current_char() == '=') {
            advance();
            return make_token(TokenType::LESS_EQ);
        }
        if (current_char() == '<') {
            advance();
            return make_token(TokenType::LSHIFT);
        }
        return make_token(TokenType::LESS);
    }

    if (c == '>') {
        if (current_char() == '=') {
            advance();
            return make_token(TokenType::GREATER_EQ);
        }
        if (current_char() == '>') {
            advance();
            return make_token(TokenType::RSHIFT);
        }
        return make_token(TokenType::GREATER);
    }

    if (c == '&') {
        if (current_char() == '&') {
            advance();
            return make_token(TokenType::AMPERSAND_AMPERSAND);
        }
        return make_token(TokenType::AMPERSAND);
    }

    if (c == '|') {
        if (current_char() == '|') {
            advance();
            return make_token(TokenType::PIPE_PIPE);
        }
        return make_token(TokenType::PIPE);
    }

    if (c == ':') {
        if (current_char() == '=') {
            advance();
            return make_token(TokenType::COLON_EQUAL);
        }
        if (current_char() == ':') {
            advance();
            return make_token(TokenType::COLON_COLON);
        }
        return make_token(TokenType::COLON);
    }

    if (c == '.') {
        if (current_char() == '.' && peek_char() == '.') {
            advance();
            advance();
            return make_token(TokenType::DOT_DOT_DOT);
        }
        if (current_char() == '.') {
            advance();
            return make_token(TokenType::RANGE_OP);
        }
        return make_token(TokenType::DOT);
    }

    if (c == '@') {
        return make_token(TokenType::AT);
    }

    if (c == '#') {
        return make_token(TokenType::HASH);
    }

    // Single-character operators
    switch (c) {
        case '%': return make_token(TokenType::PERCENT);
        case '^': return make_token(TokenType::CARET);
        case '~': return make_token(TokenType::TILDE);
        case '(': return make_token(TokenType::LPAREN);
        case ')': return make_token(TokenType::RPAREN);
        case '{': return make_token(TokenType::LBRACE);
        case '}': return make_token(TokenType::RBRACE);
        case '[': {
            if (current_char() == '[') {
                advance();
                return make_token(TokenType::LBRACKET_LBRACKET);
            }
            return make_token(TokenType::LBRACK);
        }
        case ']': {
            if (current_char() == ']') {
                advance();
                return make_token(TokenType::RBRACKET_RBRACKET);
            }
            return make_token(TokenType::RBRACK);
        }
        case ';': return make_token(TokenType::SEMICOLON);
        case ',': return make_token(TokenType::COMMA);
        default:
            errors_.push_back(std::string("Unknown character: ") + c);
            return make_token(TokenType::UNKNOWN);
    }
}

Token Lexer::peek_token() const {
    // This would require saving/restoring state
    // For now, returning EOF
    return Token(TokenType::EOF_TOKEN, "", line_, column_);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token token = next_token();
        tokens.push_back(token);
        if (token.type == TokenType::EOF_TOKEN) {
            break;
        }
    }
    return tokens;
}

} // namespace ibex
