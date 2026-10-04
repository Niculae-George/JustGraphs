if(NOT DEFINED BUNDLE_DIR OR NOT DEFINED EXECUTABLE_NAME)
    message(FATAL_ERROR "BUNDLE_DIR and EXECUTABLE_NAME are required")
endif()
set(plist "${BUNDLE_DIR}/Contents/Info.plist")
file(READ "${plist}" content)
if(NOT content MATCHES "<key>CFBundleExecutable</key>[ \t\r\n]*<string>[^<]+</string>")
    message(FATAL_ERROR "Missing CFBundleExecutable entry in ${plist}")
endif()
string(REGEX REPLACE
    "<key>CFBundleExecutable</key>[ \t\r\n]*<string>[^<]+</string>"
    "<key>CFBundleExecutable</key>\n\t<string>${EXECUTABLE_NAME}</string>"
    content "${content}"
)
file(WRITE "${plist}" "${content}")
