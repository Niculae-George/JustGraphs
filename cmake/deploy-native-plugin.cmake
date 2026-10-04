# Included at install time after Qt's framework deployment.
execute_process(COMMAND /usr/bin/otool -L "${PLUGIN_FILE}"
    OUTPUT_VARIABLE dependencies RESULT_VARIABLE inspect_result)
if(NOT inspect_result EQUAL 0)
    message(FATAL_ERROR "Cannot inspect installed Qt plugin: ${PLUGIN_FILE}")
endif()
string(REGEX MATCHALL "@rpath/(Qt[^ \t\n]+|libQt6[^ \t\n]+)" qt_dependencies "${dependencies}")
foreach(dependency IN LISTS qt_dependencies)
    string(REPLACE "@rpath/" "" framework_path "${dependency}")
    if(NOT EXISTS "${BUNDLE_DIR}/Contents/Frameworks/${framework_path}")
        message(FATAL_ERROR "Missing bundled plugin dependency: ${framework_path}")
    endif()
    execute_process(COMMAND /usr/bin/install_name_tool -change "${dependency}"
        "@loader_path/../../Frameworks/${framework_path}" "${PLUGIN_FILE}"
        RESULT_VARIABLE patch_result)
    if(NOT patch_result EQUAL 0)
        message(FATAL_ERROR "Cannot link installed Qt plugin to bundled frameworks")
    endif()
endforeach()
execute_process(COMMAND /usr/bin/codesign --force --sign - "${PLUGIN_FILE}"
    RESULT_VARIABLE sign_result)
if(NOT sign_result EQUAL 0)
    message(FATAL_ERROR "Cannot sign installed Qt plugin: ${PLUGIN_FILE}")
endif()
