#pragma once

#include "ast.h"
#include "semantic_analyzer.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace ibex {

class FFIParser {
public:
    FFIParser(const FFIConfig& config, Program& program);
    
    // Parses C headers and returns a map of function signatures
    std::unordered_map<std::string, FunctionType> parse_c_headers(const std::vector<DeclHandle>& foreign_blocks);

private:
    FFIConfig config_;
    Program& program_;
    
    TypeHandle map_c_type(const std::string& c_type_str, bool is_pointer);
};

} // namespace ibex
