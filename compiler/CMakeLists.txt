add_executable(ibexc
    main.cpp
)

target_link_libraries(ibexc PRIVATE ibex_compiler)
target_include_directories(ibexc PRIVATE ${PROJECT_SOURCE_DIR}/include)
