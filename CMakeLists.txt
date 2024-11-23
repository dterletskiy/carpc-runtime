cmake_minimum_required( VERSION 3.16 FATAL_ERROR )

fenix_project( TARGET ${TARGET_PREFIX}runtime PROJECT RUNTIME )



###########################################################################################
#                                                                                         #
#                             Compilation include directories                             #
#                                                                                         #
###########################################################################################
include_directories( ${PROJECT_SOURCE_DIR}/api )
include_directories( ${TRACING_SOURCE_DIR}/api )
include_directories( ${BASE_SOURCE_DIR}/api )
include_directories( ${OSW_SOURCE_DIR}/api )
include_directories( ${TOOLS_SOURCE_DIR}/api )



###########################################################################################
#                                                                                         #
#                                Linkage include directories                              #
#                                                                                         #
###########################################################################################
link_directories( ${TRACING_BINARY_DIR} )
link_directories( ${BASE_BINARY_DIR} )
link_directories( ${OSW_BINARY_DIR} )
link_directories( ${TOOLS_BINARY_DIR} )



###########################################################################################
#                                                                                         #
#                                   Compile definitions                                   #
#                                                                                         #
###########################################################################################



###########################################################################################
#                                                                                         #
#                                      Code generation                                    #
#                                                                                         #
###########################################################################################
find_files_by_ext( RECURSE FILES PLANTUML_FILES
      LOCATION ${PROJECT_SOURCE_DIR}
      EXTENTIONS ${EXTENTIONS_PLANTUML}
   )
generate_plantumls( PLANTUML_FILES ${PLANTUML_FILES} GENERATED_FILES PROJECT_GEN_PLANTUML_FILES )



###########################################################################################
#                                                                                         #
#                                   Build source file list                                #
#                                                                                         #
###########################################################################################
find_files_by_ext( RECURSE FILES PROJECT_SOURCE_FILES
      LOCATION ${PROJECT_SOURCE_DIR}
      EXTENTIONS ${EXTENTIONS_CPP_SRC}
   )



###########################################################################################
#                                                                                         #
#                                      Debug messages                                     #
#                                                                                         #
###########################################################################################
print_variables( )


add_library(
      ${PROJECT_TARGET_NAME}-shared SHARED
      ${PROJECT_SOURCE_FILES}
   )
set_target_properties(
      ${PROJECT_TARGET_NAME}-shared PROPERTIES
      OUTPUT_NAME ${PROJECT_TARGET_NAME}
   )
target_link_libraries(
      ${PROJECT_TARGET_NAME}-shared
      PUBLIC ${OSW_TARGET_NAME}-shared
      PUBLIC ${TOOLS_TARGET_NAME}-shared
   )

add_library(
      ${PROJECT_TARGET_NAME}-static STATIC
      ${PROJECT_SOURCE_FILES}
   )
set_target_properties(
      ${PROJECT_TARGET_NAME}-static PROPERTIES
      OUTPUT_NAME ${PROJECT_TARGET_NAME}
   )
target_link_libraries(
      ${PROJECT_TARGET_NAME}-static
      PUBLIC ${OSW_TARGET_NAME}-static
      PUBLIC ${TOOLS_TARGET_NAME}-static
   )


add_custom_target( "${PROJECT_TARGET_NAME}-documentation" ALL
      COMMENT "cmake ${PROJECT_TARGET_NAME}-documentation"
      DEPENDS ${PROJECT_GEN_PLANTUML_FILES}
   )

add_custom_target(
      ${PROJECT_TARGET_NAME} ALL
      DEPENDS
         ${PROJECT_TARGET_NAME}-shared
         ${PROJECT_TARGET_NAME}-static
         ${PROJECT_TARGET_NAME}-documentation
   )



install(
      TARGETS
         ${PROJECT_TARGET_NAME}-shared
         ${PROJECT_TARGET_NAME}-static
      ARCHIVE DESTINATION lib/carpc/
      LIBRARY DESTINATION lib/carpc/
   )

install( DIRECTORY ${PROJECT_SOURCE_DIR}/api/carpc/ DESTINATION include/carpc/ )



add_custom_target( deploy_${PROJECT_TARGET_NAME}
      DEPENDS ${PROJECT_TARGET_NAME}
      COMMENT "Deploying '${PROJECT_TARGET_NAME}'"
   )

file( MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/lib/carpc/ )
add_custom_command(
      TARGET deploy_${PROJECT_TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy 
         $<TARGET_FILE:${PROJECT_TARGET_NAME}-static> ${CMAKE_INSTALL_PREFIX}/lib/carpc/
      COMMAND ${CMAKE_COMMAND} -E copy 
         $<TARGET_FILE:${PROJECT_TARGET_NAME}-shared> ${CMAKE_INSTALL_PREFIX}/lib/carpc/
      COMMENT "Copying library articats"
   )

file( MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/include/ )
add_custom_command(
      TARGET deploy_${PROJECT_TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory 
         ${PROJECT_SOURCE_DIR}/api/carpc/ ${CMAKE_INSTALL_PREFIX}/include/carpc/
      COMMENT "Copying include artifacts"
   )
