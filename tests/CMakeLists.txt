if(BUILD_TESTS)
    add_executable(test_lexer
        test_lexer.cpp
    )
    target_link_libraries(test_lexer PRIVATE ibex_compiler)
    target_include_directories(test_lexer PRIVATE ${PROJECT_SOURCE_DIR}/include)
    add_test(NAME LexerTests COMMAND test_lexer)

    add_executable(test_parser
        test_parser.cpp
    )
    target_link_libraries(test_parser PRIVATE ibex_compiler)
    target_include_directories(test_parser PRIVATE ${PROJECT_SOURCE_DIR}/include)
    add_test(NAME ParserTests COMMAND test_parser)

    add_executable(test_type_registry
        test_type_registry.cpp
    )
    target_link_libraries(test_type_registry PRIVATE ibex_compiler)
    target_include_directories(test_type_registry PRIVATE ${PROJECT_SOURCE_DIR}/include)
    add_test(NAME TypeRegistryTests COMMAND test_type_registry)
endif()
