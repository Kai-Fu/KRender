file(GLOB_RECURSE KRT_CONSOLE_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_executable( ${KRT_CONSOLE_MODULE_NAME} ${KRT_CONSOLE_SRC} )

# Link against KRTCore
target_link_libraries( ${KRT_CONSOLE_MODULE_NAME} ${KRT_CORE_MODULE_NAME} )
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.2.2_src" )

foreach( build_type ${KRT_BUILD_TYPE_LIST} )  

  # Install the runtime of KTRConsole
  install( TARGETS ${KRT_CONSOLE_MODULE_NAME} RUNTIME DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
  
  # Install the startup LUA script
  install( FILES "startup.lua" DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
endforeach()  