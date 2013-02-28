add_definitions( -DBUILD_KRT_CORE_DLL )

file( GLOB_RECURSE KRT_CORE_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_library( ${KRT_CORE_MODULE_NAME} SHARED ${KRT_CORE_SRC} )

# Link against KRTImageLib
target_link_libraries( ${KRT_CORE_MODULE_NAME} ${KRT_IMAGE_LIB_MODULE_NAME} )

# Specify the dependencies of library
target_link_libraries( ${KRT_CORE_MODULE_NAME} "${EXTERNAL_LIB_PATH}/pthread/lib/pthread_${CPU_TYPE}.lib" )

foreach( build_type ${KRT_BUILD_TYPE_LIST} )  
  # Install the dependencies of DLL to the destination
  install( FILES "${EXTERNAL_LIB_PATH}/pthread/bin/pthread_${CPU_TYPE}.dll" DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
  # Install the KRTCore runtime
  install( TARGETS ${KRT_CORE_MODULE_NAME} RUNTIME DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
endforeach()

