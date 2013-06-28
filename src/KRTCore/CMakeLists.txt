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

# Setup the Alembic library
set( HDF5_PATH CACHE PATH "Path to HDF5 SDK." )
include_directories( "${HDF5_PATH}/include" )  # HDF5 stuff
target_link_libraries( ${KRT_CORE_MODULE_NAME} 
	optimized "${HDF5_PATH}/lib/libhdf5.lib"
	optimized "${HDF5_PATH}/lib/libhdf5_hl.lib"
	)
target_link_libraries( ${KRT_CORE_MODULE_NAME} 
	debug "${HDF5_PATH}/lib/libhdf5_D.lib"
	debug "${HDF5_PATH}/lib/libhdf5_hl_D.lib"
	)
set( ILMBASE_PATH CACHE PATH "Path to Ilm Base SDK." )  # Ilm Base stuff
include_directories( "${ILMBASE_PATH}/include" )
target_link_libraries( ${KRT_CORE_MODULE_NAME} 
	optimized "${ILMBASE_PATH}/lib/${CPU_TYPE}/Release/Half.lib"
	optimized "${ILMBASE_PATH}/lib/${CPU_TYPE}/Release/Iex.lib"
	optimized "${ILMBASE_PATH}/lib/${CPU_TYPE}/Release/IlmThread.lib"
	optimized "${ILMBASE_PATH}/lib/${CPU_TYPE}/Release/Imath.lib"
	)
install( FILES "${ILMBASE_PATH}/bin/${CPU_TYPE}/Release/Half.dll" DESTINATION bin CONFIGURATIONS Release )
install( FILES "${ILMBASE_PATH}/bin/${CPU_TYPE}/Release/Iex.dll" DESTINATION bin CONFIGURATIONS Release )
install( FILES "${ILMBASE_PATH}/bin/${CPU_TYPE}/Release/IlmThread.dll" DESTINATION bin CONFIGURATIONS Release )
install( FILES "${ILMBASE_PATH}/bin/${CPU_TYPE}/Release/Imath.dll" DESTINATION bin CONFIGURATIONS Release )
target_link_libraries( ${KRT_CORE_MODULE_NAME} 
	debug "${ILMBASE_PATH}/lib/${CPU_TYPE}/Debug/Half.lib"
	debug "${ILMBASE_PATH}/lib/${CPU_TYPE}/Debug/Iex.lib"
	debug "${ILMBASE_PATH}/lib/${CPU_TYPE}/Debug/IlmThread.lib"
	debug "${ILMBASE_PATH}/lib/${CPU_TYPE}/Debug/Imath.lib"
	)
install( FILES "${ILMBASE_PATH}/bin/${CPU_TYPE}/Debug/Half.dll" DESTINATION bin CONFIGURATIONS Debug )
install( FILES "${ILMBASE_PATH}/bin/${CPU_TYPE}/Debug/Iex.dll" DESTINATION bin CONFIGURATIONS Debug )
install( FILES "${ILMBASE_PATH}/bin/${CPU_TYPE}/Debug/IlmThread.dll" DESTINATION bin CONFIGURATIONS Debug )
install( FILES "${ILMBASE_PATH}/bin/${CPU_TYPE}/Debug/Imath.dll" DESTINATION bin CONFIGURATIONS Debug )
set( ALEMBIC_PATH CACHE PATH "Path to Alembic SDK." ) # Abc stuff
include_directories( "${ALEMBIC_PATH}/include" )
target_link_libraries( ${KRT_CORE_MODULE_NAME} 
	optimized "${ALEMBIC_PATH}/lib/static_release/AlembicAbc.lib"
	optimized "${ALEMBIC_PATH}/lib/static_release/AlembicAbcCollection.lib"
	optimized "${ALEMBIC_PATH}/lib/static_release/AlembicAbcCoreAbstract.lib"
	optimized "${ALEMBIC_PATH}/lib/static_release/AlembicAbcCoreHDF5.lib"
	optimized "${ALEMBIC_PATH}/lib/static_release/AlembicAbcGeom.lib"
	optimized "${ALEMBIC_PATH}/lib/static_release/AlembicAbcMaterial.lib"
	optimized "${ALEMBIC_PATH}/lib/static_release/AlembicUtil.lib"
	)
target_link_libraries( ${KRT_CORE_MODULE_NAME} 
	debug "${ALEMBIC_PATH}/lib/static_debug/AlembicAbc.lib"
	debug "${ALEMBIC_PATH}/lib/static_debug/AlembicAbcCollection.lib"
	debug "${ALEMBIC_PATH}/lib/static_debug/AlembicAbcCoreAbstract.lib"
	debug "${ALEMBIC_PATH}/lib/static_debug/AlembicAbcCoreHDF5.lib"
	debug "${ALEMBIC_PATH}/lib/static_debug/AlembicAbcGeom.lib"
	debug "${ALEMBIC_PATH}/lib/static_debug/AlembicAbcMaterial.lib"
	debug "${ALEMBIC_PATH}/lib/static_debug/AlembicUtil.lib"
	)
	
# Install the KRTCore runtime
install( TARGETS ${KRT_CORE_MODULE_NAME} RUNTIME DESTINATION bin )

install( DIRECTORY "asset" DESTINATION bin )
