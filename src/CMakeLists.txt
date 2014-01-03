cmake_minimum_required (VERSION 2.6)

list( LENGTH CMAKE_CONFIGURATION_TYPES num_build_type )
if ( num_build_type GREATER  1 )
  message( FATAL_ERROR "CMAKE_CONFIGURATION_TYPES must be explicitly Debug or Release." )
endif ( num_build_type GREATER  1 )

if ( CMAKE_CONFIGURATION_TYPES MATCHES "Debug" )
  project (KRender_Debug)
else ( CMAKE_CONFIGURATION_TYPES MATCHES "Debug" )
  project (KRender_Release)
endif ( CMAKE_CONFIGURATION_TYPES MATCHES "Debug" )

# The version number.
set( KRender_VERSION_MAJOR 1 )
set( KRender_VERSION_MINOR 0 )

# module names of KRT
set( KRT_CORE_MODULE_NAME "KRTCore" )
set( KRT_CONSOLE_MODULE_NAME "KRTConsole" )
set( KSC_MODULE_NAME "KShaderCompiler" )

# The default CPU type is x86
set( CPU_TYPE "x86" )

if( WIN32 )
  if( CMAKE_GENERATOR MATCHES "Win64" )
    set( CPU_TYPE "x64" )
  endif( CMAKE_GENERATOR MATCHES "Win64" )
else( WIN32 )
  message( FATAL_ERROR "Currently only Windows platform is supported." )
endif( WIN32 )

message( "CPU type is ${CPU_TYPE}" )
message( "Current configuration type is ${CMAKE_CONFIGURATION_TYPES}" )
  
# Header file of KRT
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}" )

# Shared module KRTCore
add_subdirectory( KRTCore )

# Executable module KRTConsole
add_subdirectory( KRTConsole )


# Shared module KShaderCompiler
add_subdirectory( KShaderCompiler/src )



