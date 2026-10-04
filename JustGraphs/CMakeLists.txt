include("${PROJECT_SOURCE_DIR}/cmake/Modules.cmake")

justgraphs_module(justgraphs_domain DIRECTORIES domain LIBRARIES Qt6::Core)
justgraphs_module(justgraphs_algorithms DIRECTORIES algorithms LIBRARIES justgraphs_domain)
justgraphs_module(justgraphs_maps DIRECTORIES maps LIBRARIES Qt6::Core)
justgraphs_module(justgraphs_rendering DIRECTORIES rendering ui
    LIBRARIES justgraphs_domain justgraphs_algorithms Qt6::Gui Qt6::Widgets)
justgraphs_module(justgraphs_features DIRECTORIES features
    LIBRARIES justgraphs_algorithms justgraphs_maps justgraphs_rendering Qt6::Widgets)
justgraphs_module(justgraphs_shell DIRECTORIES app LIBRARIES justgraphs_features Qt6::Widgets)

qt_add_executable(JustGraphs app/main.cpp)
target_link_libraries(JustGraphs PRIVATE justgraphs_shell)
justgraphs_compile_options(JustGraphs)
include("${PROJECT_SOURCE_DIR}/cmake/Deployment.cmake")

if(BUILD_TESTING)
    find_package(Qt6 6.9 REQUIRED COMPONENTS Test)
    justgraphs_tests(domain justgraphs_domain)
    justgraphs_tests(algorithms justgraphs_algorithms)
    justgraphs_tests(maps justgraphs_maps)
    justgraphs_tests(rendering justgraphs_rendering)
    justgraphs_tests(features justgraphs_features)
    justgraphs_tests(app justgraphs_shell)
    add_test(NAME architecture_boundaries
        COMMAND "${CMAKE_COMMAND}" "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            -P "${PROJECT_SOURCE_DIR}/cmake/test-architecture.cmake")
endif()
