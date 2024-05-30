set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_executable(logging-example
	exampleLoggingMain.cpp
	exampleLoggingOtherFile.cpp
)

target_link_libraries(logging-example
	${LIB_NAME}
)

install(TARGETS logging-example
		DESTINATION ${CMAKE_INSTALL_BINDIR}
)
