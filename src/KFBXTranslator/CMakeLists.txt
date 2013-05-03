file(GLOB_RECURSE KRT_FBX_TRANSLATOR_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_definitions( -DFBXSDK_SHARED )
add_executable( ${KRT_FBX_TRANSLATOR_NAME} ${KRT_FBX_TRANSLATOR_SRC} )

# Link against KRTCore
target_link_libraries( ${KRT_FBX_TRANSLATOR_NAME} ${KRT_CORE_MODULE_NAME} )

if( CMAKE_GENERATOR MATCHES "Visual Studio 10" )
  set( FBX_VS_LIB_PATH "vs2010" )
endif( CMAKE_GENERATOR MATCHES "Visual Studio 10" )

if( CMAKE_GENERATOR MATCHES "Visual Studio 9 2008" )
  set( FBX_VS_LIB_PATH "vs2008" )
endif( CMAKE_GENERATOR MATCHES "Visual Studio 9 2008" )

if( NOT FBX_VS_LIB_PATH )
  message( "There's no FBX SDK for the specified compiler." )
endif( NOT FBX_VS_LIB_PATH )

set( FBX_SDK_PATH CACHE PATH "Path to FBX SDK." )
include_directories( "${FBX_SDK_PATH}/include" )
# Link against FBX SDK
target_link_libraries( ${KRT_FBX_TRANSLATOR_NAME} debug "${FBX_SDK_PATH}/lib/${FBX_VS_LIB_PATH}/${CPU_TYPE}/fbxsdk-2013.3d.lib" )
target_link_libraries( ${KRT_FBX_TRANSLATOR_NAME} optimized "${FBX_SDK_PATH}/lib/${FBX_VS_LIB_PATH}/${CPU_TYPE}/fbxsdk-2013.3.lib" )
include_directories( "${FBX_SDK_PATH}/include" )
# Install the FBX runtime DLL
install( FILES "${FBX_SDK_PATH}/lib/${FBX_VS_LIB_PATH}/${CPU_TYPE}/fbxsdk-2013.3d.dll" DESTINATION bin CONFIGURATIONS Debug )
install( FILES "${FBX_SDK_PATH}/lib/${FBX_VS_LIB_PATH}/${CPU_TYPE}/fbxsdk-2013.3.dll" DESTINATION bin CONFIGURATIONS Release )

install( TARGETS ${KRT_FBX_TRANSLATOR_NAME} RUNTIME DESTINATION bin )