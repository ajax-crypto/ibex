add_library(tree_sitter_lib STATIC 
    ${tree_sitter_SOURCE_DIR}/lib/src/lib.c
    ${tree_sitter_c_SOURCE_DIR}/src/parser.c
)
target_include_directories(tree_sitter_lib PRIVATE ${tree_sitter_SOURCE_DIR}/lib/src)

add_library(ibex_compiler
    lexer.cpp
    parser.cpp
    type_registry.cpp
    node_registry.cpp
    semantic_analyzer.cpp
    module_scanner.cpp
    const_eval.cpp
    ffi_parser.cpp
)

# Link against standard library and yyjson
target_link_libraries(ibex_compiler
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:>
        $<$<CXX_COMPILER_ID:Clang>:c++>
        $<$<CXX_COMPILER_ID:GNU>:stdc++>
    PUBLIC
        glaze::glaze
        tree_sitter_lib
)

target_include_directories(ibex_compiler PUBLIC ${PROJECT_SOURCE_DIR}/include)

