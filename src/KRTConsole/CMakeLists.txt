file(GLOB_RECURSE KRT_CONSOLE_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_definitions( "/wd4996" )
add_executable( ${KRT_CONSOLE_MODULE_NAME} ${KRT_CONSOLE_SRC} )

# Link against KRTCore
target_link_libraries( ${KRT_CONSOLE_MODULE_NAME} ${KRT_CORE_MODULE_NAME} )
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.2.2_src" )

install( TARGETS ${KRT_CONSOLE_MODULE_NAME} RUNTIME DESTINATION bin )
install( FILES "startup.lua" DESTINATION bin )
