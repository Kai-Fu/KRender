add_definitions( -DBUILD_KRT_CORE_DLL )

file( GLOB_RECURSE KRT_CORE_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_library( ${KRT_CORE_MODULE_NAME} SHARED ${KRT_CORE_SRC} )

set( PTHREAD_SDK_PATH CACHE PATH "Path to pthread SDK." )
include_directories( "${PTHREAD_SDK_PATH}/include" )

set( FREEIMAGE_SDK_PATH CACHE PATH "Path to FreeImage SDK." )
# Include the header file of FreeImage
include_directories( "${FREEIMAGE_SDK_PATH}" )
# Link to the library of FreeImage
target_link_libraries( ${KRT_CORE_MODULE_NAME} debug "${FREEIMAGE_SDK_PATH}/FreeImage.lib" )
target_link_libraries( ${KRT_CORE_MODULE_NAME} optimized "${FREEIMAGE_SDK_PATH}/FreeImage.lib" )

# Specify the dependencies of library
target_link_libraries( ${KRT_CORE_MODULE_NAME} "${PTHREAD_SDK_PATH}/lib/pthread_${CPU_TYPE}.lib" )

foreach( build_type ${KRT_BUILD_TYPE_LIST} )  
  # Install the dependencies of DLL to the destination
  install( FILES "${PTHREAD_SDK_PATH}/bin/pthread_${CPU_TYPE}.dll" DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
  install( FILES "${FREEIMAGE_SDK_PATH}/FreeImage.dll" DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
  # Install the KRTCore runtime
  install( TARGETS ${KRT_CORE_MODULE_NAME} RUNTIME DESTINATION bin/${CPU_TYPE}/${build_type} CONFIGURATIONS ${build_type} )
endforeach()

