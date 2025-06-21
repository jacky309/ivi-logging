
set(HEADER_FILES
	ivi-logging.h
	ivi-logging-types.h
	ivi-logging-stl.h
	ivi-logging-console.h
	ivi-logging-file.h
	ivi-logging-common.h
	ivi-logging-null.h
	ivi-logging-utils.h
	ivi-no-logging.h
)

if(ENABLE_DLT_BACKEND)
	list(APPEND HEADER_FILES ivi-logging-dlt.h)
endif()

if(ENABLE_DLTCPP_BACKEND)
	list(APPEND DLT_LIBRARY_FILES ivi-logging-dltcpp.cpp)
	list(APPEND HEADER_FILES ivi-logging-dltcpp.h)
endif()

add_library(${LIB_NAME} SHARED
	ivi-logging.cpp
	backtrace.cpp
	${DLT_LIBRARY_FILES}
)

set_target_properties(${LIB_NAME} PROPERTIES VERSION ${VERSION} SOVERSION ${PROJECT_MAJOR_VERSION})
target_compile_features(${LIB_NAME} PUBLIC cxx_std_17)

target_link_libraries(${LIB_NAME}
   ivi-logging-dependencies
   pthread
)

set(HEADER_INSTALLATION_PATH ${CMAKE_INSTALL_INCLUDEDIR}/${PACKAGE})

target_include_directories(${LIB_NAME}
PUBLIC
   $<INSTALL_INTERFACE:${HEADER_INSTALLATION_PATH}>
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
   $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
)

install(TARGETS ${LIB_NAME}
   EXPORT ${PROJECT_NAME}Targets
   DESTINATION ${CMAKE_INSTALL_LIBDIR}
)


foreach(HEADER_FILE ${HEADER_FILES})
   list(APPEND INCLUDE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/${HEADER_FILE})
endforeach()

install(
   FILES ${INCLUDE_FILES} ${PROJECT_BINARY_DIR}/ivi-logging-config.h
   DESTINATION ${HEADER_INSTALLATION_PATH}
)
