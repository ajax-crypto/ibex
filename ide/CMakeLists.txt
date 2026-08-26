add_executable(ibex_ide
    main.cpp
    editor.cpp
    visual_editor.cpp
    callstack_view.cpp
    dataflow_view.cpp
)

target_link_libraries(ibex_ide PRIVATE ibex_compiler ibex_runtime)
target_include_directories(ibex_ide PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Note: This would typically link against a UI framework like Qt or ImGui
# For now, this is a placeholder for the IDE structure
