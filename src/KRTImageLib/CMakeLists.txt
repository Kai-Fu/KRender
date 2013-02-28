add_definitions( -DBUILD_KRT_IMAGE_LIB_DLL )

file(GLOB_RECURSE KRT_IMAGE_LIB_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_library( ${KRT_IMAGE_LIB_MODULE_NAME} SHARED ${KRT_IMAGE_LIB_SRC} )

# Specify the dependencies of library
target_link_libraries( ${KRT_IMAGE_LIB_MODULE_NAME} "${EXTERNAL_LIB_PATH}/DevIL_SDK1.7.8/${CPU_TYPE}/DevIL.lib" )
target_link_libraries( ${KRT_IMAGE_LIB_MODULE_NAME} "${EXTERNAL_LIB_PATH}/DevIL_SDK1.7.8/${CPU_TYPE}/ILU.lib" )
target_link_libraries( ${KRT_IMAGE_LIB_MODULE_NAME} "${EXTERNAL_LIB_PATH}/DevIL_SDK1.7.8/${CPU_TYPE}/ILUT.lib" )

foreach( build_type ${KRT_BUILD_TYPE_LIST} )  
  # Install the dependencies of DLL to the destination
  install( FILES "${EXTERNAL_LIB_PATH}/DevIL_SDK1.7.8/${CPU_TYPE}/DevIL.dll" DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
  install( FILES "${EXTERNAL_LIB_PATH}/DevIL_SDK1.7.8/${CPU_TYPE}/ILU.dll" DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
  install( FILES "${EXTERNAL_LIB_PATH}/DevIL_SDK1.7.8/${CPU_TYPE}/ILUT.dll" DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
  # Install the runtime of KRTImageLib
  install( TARGETS ${KRT_IMAGE_LIB_MODULE_NAME} RUNTIME DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
endforeach()  