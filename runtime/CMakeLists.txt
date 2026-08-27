add_library(ibex_runtime STATIC
    memory.cpp
    runtime.cpp
)

target_include_directories(ibex_runtime PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Link C standard library
target_link_libraries(ibex_runtime
    PUBLIC
        $<$<CXX_COMPILER_ID:Clang>:c++>
        $<$<CXX_COMPILER_ID:GNU>:stdc++>
)
