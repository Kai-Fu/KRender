cmake_minimum_required (VERSION 2.6)
project (KRender)

# The version number.
set( KRender_VERSION_MAJOR 1 )
set( KRender_VERSION_MINOR 0 )

# module names of KRT
set( KRT_CORE_MODULE_NAME "KRTCore" )
set( KRT_CONSOLE_MODULE_NAME "KRTConsole" )
set( KRT_FBX_TRANSLATOR_NAME "KFBXTranslator" )
set( KSC_MODULE_NAME "KShaderCompiler" )

# Use the environment variable "KRT_EXTERNAL_LIB_PATH" to define the path where exnternal library is located
set( EXTERNAL_LIB_PATH $ENV{KRT_EXTERNAL_LIB_PATH} )

message( STATUS "External lib path is ${EXTERNAL_LIB_PATH}" )

# The default CPU type is x86
set( CPU_TYPE "x86" )

if( WIN32 )
  if( CMAKE_GENERATOR MATCHES "Win64" )
    set( CPU_TYPE "x64" )
  endif( CMAKE_GENERATOR MATCHES "Win64" )
else( WIN32 )
  message( FATAL_ERROR "Currently only Windows platform is supported." )
endif( WIN32 )

# Supported configuration type
set( KRT_BUILD_TYPE_LIST "Debug" "Release" )

message( "CPU type is ${CPU_TYPE}" )

# Header file of KRT
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}" )


# Shared module KRTCore
add_subdirectory( KRTCore )

# Executable module KRTConsole
add_subdirectory( KRTConsole )


# Executable module KFBXTranslator
add_subdirectory( KFBXTranslator )


# Shared module KShaderCompiler
add_subdirectory( KShaderCompiler/src )



