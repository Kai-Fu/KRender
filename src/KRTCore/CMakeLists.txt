add_definitions( -DBUILD_KRT_CORE_DLL )
add_definitions( "/wd4996" )

file( GLOB_RECURSE KRT_CORE_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_library( ${KRT_CORE_MODULE_NAME} SHARED ${KRT_CORE_SRC} )

target_link_libraries( ${KRT_CORE_MODULE_NAME} ${KSC_MODULE_NAME} )

set( FREEIMAGE_SDK_PATH CACHE PATH "Path to FreeImage SDK." )
# Setup the dependency for FreeImage
include_directories( "${FREEIMAGE_SDK_PATH}" )
target_link_libraries( ${KRT_CORE_MODULE_NAME} debug "${FREEIMAGE_SDK_PATH}/FreeImaged.lib" )
target_link_libraries( ${KRT_CORE_MODULE_NAME} optimized "${FREEIMAGE_SDK_PATH}/FreeImage.lib" )
install( FILES "${FREEIMAGE_SDK_PATH}/FreeImaged.dll" DESTINATION bin CONFIGURATIONS Debug )
install( FILES "${FREEIMAGE_SDK_PATH}/FreeImage.dll" DESTINATION bin CONFIGURATIONS Release )

# Setup the dependency for pthread
set( PTHREAD_SDK_PATH CACHE PATH "Path to pthread SDK." )
include_directories( "${PTHREAD_SDK_PATH}/include" )
target_link_libraries( ${KRT_CORE_MODULE_NAME} "${PTHREAD_SDK_PATH}/lib/pthread_${CPU_TYPE}.lib" )
install( FILES "${PTHREAD_SDK_PATH}/bin/pthread_${CPU_TYPE}.dll" DESTINATION bin )

# Install the KRTCore runtime
install( TARGETS ${KRT_CORE_MODULE_NAME} RUNTIME DESTINATION bin )



