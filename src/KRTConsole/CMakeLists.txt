file(GLOB_RECURSE KRT_CONSOLE_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_executable( ${KRT_CONSOLE_MODULE_NAME} ${KRT_CONSOLE_SRC} )

# Link against KRTCore
target_link_libraries( ${KRT_CONSOLE_MODULE_NAME} ${KRT_CORE_MODULE_NAME} )

# Link against Lua 5.1
target_link_libraries( ${KRT_CONSOLE_MODULE_NAME} "${EXTERNAL_LIB_PATH}/Lua5_1_4/${CPU_TYPE}/lua5.1.lib" )
target_link_libraries( ${KRT_CONSOLE_MODULE_NAME} "${EXTERNAL_LIB_PATH}/Lua5_1_4/${CPU_TYPE}/lua51.lib" )

foreach( build_type ${KRT_BUILD_TYPE_LIST} )  
  # Install the dependencies of DLL to the destination
  install( FILES "${EXTERNAL_LIB_PATH}/Lua5_1_4/${CPU_TYPE}/lua5.1.dll" DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
  install( FILES "${EXTERNAL_LIB_PATH}/Lua5_1_4/${CPU_TYPE}/lua51.dll" DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )

  # Install the runtime of KTRConsole
  install( TARGETS ${KRT_CONSOLE_MODULE_NAME} RUNTIME DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
  
  # Install the startup LUA script
  install( FILES "startup.lua" DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
endforeach()  