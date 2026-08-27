if(BUILD_TESTS)
    add_executable(test_runner
        test_runner.cpp
    )
    target_link_libraries(test_runner PRIVATE ibex_compiler)
    target_include_directories(test_runner PRIVATE ${PROJECT_SOURCE_DIR}/include)

    file(GLOB_RECURSE IBEX_SOURCE_TESTS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/test_*.ibex"
    )
    list(FILTER IBEX_SOURCE_TESTS EXCLUDE REGEX "/test_modules/")
    add_test(NAME IbexSourceTests COMMAND test_runner ${IBEX_SOURCE_TESTS})

    add_test(NAME IbexModuleTests
        COMMAND test_runner
            "${CMAKE_CURRENT_SOURCE_DIR}/test_modules/main.ibex"
            --import-recursive "${CMAKE_CURRENT_SOURCE_DIR}/test_modules/math"
    )
endif()
