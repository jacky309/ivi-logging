
cmake_minimum_required(VERSION 3.6)

set(PACKAGE ivi-logging)
PROJECT(${PACKAGE})

include(GNUInstallDirs)

SET( PROJECT_MAJOR_VERSION 1 )
SET( PROJECT_MINOR_VERSION 3 )
SET( PROJECT_PATCH_LEVEL 0 )

set(VERSION ${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_PATCH_LEVEL})
set(PACKAGE_VERSION ${VERSION})

OPTION( ENABLE_DLTCPP_BACKEND "Include DLT CPP as default logging backend" OFF )
OPTION( ENABLE_DLT_BACKEND "Include DLT as default logging backend" ${ENABLE_DLTCPP_BACKEND})
OPTION( ENABLE_CONSOLE_BACKEND "Include console as default logging backend" ON )
OPTION( ENABLE_EXAMPLES "Build examples" ON )
OPTION( ENABLE_TESTS "Build tests" ON )

# Dependencies
if (ENABLE_DLT_BACKEND)
    set(ADDITIONAL_PKGCONFIG_DEPENDENCIES automotive-dlt)
    set(BACKEND_DEFINES "#define IVI_LOGGING_DLT_AVAILABLE")
    set(DLT_FOUND ON)

    if (ENABLE_DLTCPP_BACKEND)
        list(APPEND BACKEND_CLASSES DltCppContextClass)
    else()
        list(APPEND BACKEND_CLASSES DltContextClass)
    endif()
endif()

foreach(CLASS ${BACKEND_CLASSES})
    list(APPEND BACKEND_DATA_TYPE_CLASSES_DLT_ONLY "${CLASS}::LogDataType" )
endforeach()
string(JOIN ", " LOG_CONTEXTS_DLT_ONLY ${BACKEND_CLASSES_DLT_ONLY})
string(JOIN ", " LOG_CONTEXTS_LOG_DATA_TYPES_DLT_ONLY ${BACKEND_DATA_TYPE_CLASSES_DLT_ONLY})


if (ENABLE_CONSOLE_BACKEND)
    list(APPEND BACKEND_CLASSES ConsoleLogContext)
endif()

string(JOIN ", " LOG_CONTEXTS ${BACKEND_CLASSES})

foreach(CLASS ${BACKEND_CLASSES})
    list(APPEND BACKEND_DATA_TYPE_CLASSES "${CLASS}::LogDataType" )
endforeach()
string(JOIN ", " LOG_CONTEXTS ${BACKEND_CLASSES})
string(JOIN ", " LOG_CONTEXTS_LOG_DATA_TYPES ${BACKEND_DATA_TYPE_CLASSES})

configure_file(cmake/dependencies.cmake.in dependencies.cmake @ONLY)
include(${CMAKE_CURRENT_BINARY_DIR}/dependencies.cmake)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -Wno-variadic-macros")

configure_file(include/ivi-logging-config.h.in ivi-logging-config.h @ONLY)

set(LIB_NAME ${PACKAGE})


configure_file(ivi-logging.pc.in ivi-logging.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ivi-logging.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

set(DEVELOPMENT_INCLUDE_PATH " -I${CMAKE_CURRENT_SOURCE_DIR}/include -I${CMAKE_CURRENT_BINARY_DIR} #")
set(DEVELOPMENT_LIBRARY_PATH " -L${CMAKE_CURRENT_BINARY_DIR}/src #" )
configure_file(ivi-logging.pc.in ivi-logging-uninstalled.pc)
if(INSTALL_PKGCONFIG_UNINSTALLED_FILE)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ivi-logging-uninstalled.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

add_subdirectory (src)

if(ENABLE_TESTS)
	add_subdirectory(test)
endif()

if(ENABLE_EXAMPLES)
	add_subdirectory(examples)
endif()

add_subdirectory (doc)

include(CMakePackageConfigHelpers)

# Generate and install the files needed to import the package via "find_package(XXXX)" from its install location
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Config.cmake.in "
    @PACKAGE_INIT@
    include ( \"\${CMAKE_CURRENT_LIST_DIR}/dependencies.cmake\" )
    include ( \"\${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}Targets.cmake\" )
")

configure_package_config_file(${CMAKE_CURRENT_BINARY_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_export/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/cmake_export/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/dependencies.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)

# Generate and install the files needed to import the package via "find_package(XXXX)" from its build folder
set(BUILD_FOLDER_IMPORT_PATH ${CMAKE_CURRENT_BINARY_DIR})
export(EXPORT ${PROJECT_NAME}Targets FILE "${BUILD_FOLDER_IMPORT_PATH}/${PROJECT_NAME}Targets.cmake")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake "
    include ( \"\${CMAKE_CURRENT_LIST_DIR}/dependencies.cmake\" )
    include(\"${BUILD_FOLDER_IMPORT_PATH}/${PROJECT_NAME}Targets.cmake\")
")
