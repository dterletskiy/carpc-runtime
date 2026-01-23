cmake_minimum_required( VERSION 3.16 FATAL_ERROR )



###############################################################################
#                                                                             #
#                       Compilation include directories                       #
#                                                                             #
###############################################################################
include_directories( ${PROJECT_SOURCE_DIR}/api )
include_directories( ${TRACING_SOURCE_DIR}/api )



###############################################################################
#                                                                             #
#                          Linkage include directories                        #
#                                                                             #
###############################################################################
link_directories( ${PROJECT_BINARY_DIR} )



###############################################################################
#                                                                             #
#                             Compile definitions                             #
#                                                                             #
###############################################################################



###############################################################################
#                                                                             #
#                             Code generation                                 #
#                                                                             #
###############################################################################



###############################################################################
#                                                                             #
#                           Build source file list                            #
#                                                                             #
###############################################################################
# find_files_by_ext( RECURSE
#       FILES TEST_SOURCE_FILES
#       LOCATION ${CMAKE_CURRENT_LIST_DIR}
#       EXTENTIONS ${EXTENTIONS_CPP_SRC}
#    )



###############################################################################
#                                                                             #
#                                Debug messages                               #
#                                                                             #
###############################################################################
print_variables( )



add_executable(
      ${PROJECT_TARGET_NAME}-test
      ${TEST_SOURCE_FILES}
   )
target_link_libraries( ${PROJECT_TARGET_NAME}-test
      ${PROJECT_TARGET_NAME}-shared
   )

if( DLT_TRACE )
   target_link_libraries( ${PROJECT_TARGET_NAME}-test dlt )
endif( )
if( ${OS_TARGET} EQUAL ${OS_ANDROID} )
   target_link_libraries( ${PROJECT_TARGET_NAME}-test log )
endif( )

add_dependencies(
      ${PROJECT_TARGET_NAME}-test
      ${PROJECT_TARGET_NAME}
   )



fenix_add_deploy_targets( NAME ${PROJECT_TARGET_NAME}-test
      SUBDIR_TARGETS carpc
      TARGETS
         ${PROJECT_TARGET_NAME}-test
      DEPENDS
            deploy_${PROJECT_TARGET_NAME}
   )

target_sources( ${PROJECT_TARGET_NAME}-test
      PRIVATE
         main.cpp
   )

