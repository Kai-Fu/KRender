cmake_minimum_required (VERSION 2.6)
project (KRender)

# The version number.
set( KRender_VERSION_MAJOR 1 )
set( KRender_VERSION_MINOR 0 )

# module names of KRT
set( KRT_CORE_MODULE_NAME "KRTCore" )
set( KRT_CONSOLE_MODULE_NAME "KRTConsole" )
set( KRT_FBX_TRANSLATOR_NAME "KFBXTranslator" )

# Use the environment variable "KRT_EXTERNAL_LIB_PATH" to define the path where exnternal library is located
set( EXTERNAL_LIB_PATH $ENV{KRT_EXTERNAL_LIB_PATH} )

message( STATUS "External lib path is ${EXTERNAL_LIB_PATH}" )

set( ${CMAKE_BUILD_TYPE} "Debug" "Release" )

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

foreach(flag_var CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS )
  if(${flag_var} MATCHES "/MD")
    string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
  endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)


# Header file of KRT
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}" )

# Add the KRT modules
add_subdirectory( KRTCore )
add_subdirectory( KRTConsole )
add_subdirectory( KFBXTranslator )



