add_library(ibex_compiler
    lexer.cpp
    parser_new.cpp
    type_registry.cpp
    node_registry.cpp
    semantic_analyzer.cpp
    module_scanner.cpp
    const_eval.cpp
)

# Link against standard library and yyjson
target_link_libraries(ibex_compiler
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:>
        $<$<CXX_COMPILER_ID:Clang>:c++>
        $<$<CXX_COMPILER_ID:GNU>:stdc++>
    PUBLIC
        glaze::glaze
)

target_include_directories(ibex_compiler PUBLIC ${PROJECT_SOURCE_DIR}/include)

