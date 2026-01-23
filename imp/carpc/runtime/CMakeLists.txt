set( SOURCES
      comm/event/IEvent.cpp
      core/Context.cpp
      core/Types.cpp
   )

target_sources( ${PROJECT_TARGET_NAME}-shared
      PRIVATE
         ${SOURCES}
   )

target_sources( ${PROJECT_TARGET_NAME}-static
      PRIVATE
         ${SOURCES}
   )
