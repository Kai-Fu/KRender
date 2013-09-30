file( GLOB_RECURSE SAMPLE_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_executable( ray_tri_test ${SAMPLE_SRC} )
set_target_properties( ray_tri_test PROPERTIES FOLDER "TestCases" )

install( TARGETS ray_tri_test RUNTIME DESTINATION bin)
install( FILES "ray_tri_test.ls" DESTINATION bin)
# Specify the dependencies of library
target_link_libraries( ray_tri_test ${KSC_MODULE_NAME} )



