set_target_properties(JustGraphs PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "JustGraphs"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.justgraphs.app"
)

if(APPLE)
    set(asset_dir "$<TARGET_BUNDLE_DIR:JustGraphs>/Contents/Resources")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/JustGraphs-launcher" [=[#!/bin/sh
justgraphs_macos_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
cd "$justgraphs_macos_dir/../Resources" || exit 1
exec "$justgraphs_macos_dir/JustGraphs" "$@"
]=])
    file(CHMOD "${CMAKE_CURRENT_BINARY_DIR}/JustGraphs-launcher"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE)
    add_custom_target(justgraphs_runtime ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${asset_dir}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_BINARY_DIR}/JustGraphs-launcher"
            "$<TARGET_BUNDLE_DIR:JustGraphs>/Contents/MacOS/JustGraphs-launcher"
        COMMAND "${CMAKE_COMMAND}"
            "-DBUNDLE_DIR=$<TARGET_BUNDLE_DIR:JustGraphs>"
            -DEXECUTABLE_NAME=JustGraphs-launcher
            -P "${PROJECT_SOURCE_DIR}/cmake/set-bundle-executable.cmake"
        DEPENDS JustGraphs
        VERBATIM
    )
else()
    set(asset_dir "$<TARGET_FILE_DIR:JustGraphs>")
    add_custom_target(justgraphs_runtime ALL DEPENDS JustGraphs)
endif()

add_custom_target(run
    COMMAND "$<TARGET_FILE:JustGraphs>"
    WORKING_DIRECTORY "${asset_dir}"
    DEPENDS justgraphs_runtime
    USES_TERMINAL
    VERBATIM
)

include(GNUInstallDirs)
install(TARGETS JustGraphs
    BUNDLE DESTINATION .
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
if(APPLE)
    # macdeployqt must inspect the Mach-O binary, then Finder uses the launcher.
    install(CODE "
        set(BUNDLE_DIR \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/JustGraphs.app\")
        set(EXECUTABLE_NAME JustGraphs)
        include(\"${PROJECT_SOURCE_DIR}/cmake/set-bundle-executable.cmake\")
        file(REMOVE \"\${BUNDLE_DIR}/Contents/MacOS/JustGraphs-launcher\")
    ")
endif()
if(APPLE)
    # Deploy only the Qt modules needed by this Widgets application.
    get_filename_component(qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
    set(deploy_tool_options "-libpath=${qt_prefix}/lib")
    get_target_property(qt_core_type Qt6::Core TYPE)
    if(qt_core_type STREQUAL "SHARED_LIBRARY" AND TARGET Qt6::QCocoaIntegrationPlugin)
        list(APPEND deploy_tool_options -no-plugins)
    endif()
endif()
# The versioned function preserves install-time QT_DEPLOY_PREFIX references.
qt6_generate_deploy_app_script(
    TARGET JustGraphs
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS ${deploy_tool_options}
)
install(SCRIPT "${deploy_script}")
if(APPLE AND qt_core_type STREQUAL "SHARED_LIBRARY")
    foreach(plugin IN ITEMS QCocoaIntegrationPlugin QMacStylePlugin)
        if(TARGET Qt6::${plugin})
            if(plugin STREQUAL "QCocoaIntegrationPlugin")
                set(plugin_category platforms)
            else()
                set(plugin_category styles)
            endif()
            install(CODE "
                set(plugin_file \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/JustGraphs.app/Contents/PlugIns/${plugin_category}/$<TARGET_FILE_NAME:Qt6::${plugin}>\")
                file(REAL_PATH \"$<TARGET_FILE:Qt6::${plugin}>\" plugin_source)
                get_filename_component(plugin_dir \"\${plugin_file}\" DIRECTORY)
                file(MAKE_DIRECTORY \"\${plugin_dir}\")
                file(REMOVE \"\${plugin_file}\")
                file(COPY_FILE \"\${plugin_source}\" \"\${plugin_file}\")
                set(BUNDLE_DIR \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/JustGraphs.app\")
                set(PLUGIN_FILE \"\${plugin_file}\")
                include(\"${PROJECT_SOURCE_DIR}/cmake/deploy-native-plugin.cmake\")
            ")
        endif()
    endforeach()
endif()
if(APPLE)
    install(CODE "
        set(BUNDLE_DIR \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/JustGraphs.app\")
        set(EXECUTABLE_NAME JustGraphs-launcher)
        file(COPY \"${CMAKE_CURRENT_BINARY_DIR}/JustGraphs-launcher\"
            DESTINATION \"\${BUNDLE_DIR}/Contents/MacOS\")
        include(\"${PROJECT_SOURCE_DIR}/cmake/set-bundle-executable.cmake\")
        execute_process(COMMAND /usr/bin/codesign --force --sign -
            \"\${BUNDLE_DIR}/Contents/MacOS/JustGraphs\"
            RESULT_VARIABLE native_sign_result)
        if(NOT native_sign_result EQUAL 0)
            message(FATAL_ERROR \"Failed to ad-hoc sign native JustGraphs executable\")
        endif()
        execute_process(COMMAND /usr/bin/codesign --force --sign - \"\${BUNDLE_DIR}\"
            RESULT_VARIABLE sign_result)
        if(NOT sign_result EQUAL 0)
            message(FATAL_ERROR \"Failed to ad-hoc sign installed JustGraphs.app\")
        endif()
    " COMPONENT RuntimeFinalize)
endif()
