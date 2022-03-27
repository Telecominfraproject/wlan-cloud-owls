cmake_minimum_required(VERSION 3.13)
project(owls VERSION 2.6.0)

# cmake .. -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@1.1 -DMYSQL_ROOT_DIR=/usr/local/opt/mysql-client

if(UNIX AND APPLE)
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
    set(MYSQL_ROOT_DIR /usr/local/opt/mysql-client)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
endif()

if(UNIX AND NOT APPLE)
    set(PostgreSQL_TYPE_INCLUDE_DIR /usr/include/postgresql)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
endif()

set(CMAKE_CXX_STANDARD 17)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/build)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/build BUILD_NUM)
    if(BUILD_INCREMENT)
        MATH(EXPR BUILD_NUM "${BUILD_NUM}+1")
        file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/build ${BUILD_NUM})
    endif()
else()
    set(BUILD_NUM 1)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/build ${BUILD_NUM})
endif()

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --always --tags
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_RESULT
            OUTPUT_VARIABLE GIT_HASH)
    if(NOT GIT_RESULT EQUAL "0")
        message(FATAL_ERROR "git describe --always --tags failed with ${GIT_RESULT}")
    endif()
    string(REGEX REPLACE "\n$" "" GIT_HASH "${GIT_HASH}")
endif()

find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(MySQL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(fmt REQUIRED)
find_package(CppKafka REQUIRED)
find_package(nlohmann_json  REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)
find_package(Poco REQUIRED COMPONENTS Crypto Net Util JWT NetSSL Data DataSQLite DataPostgreSQL DataMySQL)

include_directories(/usr/local/include  /usr/local/opt/openssl/include src include/kafka /usr/local/opt/mysql-client/include)

configure_file(src/ow_version.h.in ${PROJECT_SOURCE_DIR}/src/ow_version.h @ONLY)

add_compile_options(-Wall -Wextra)
if(ASAN)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

add_executable( owls
        build
        src/ow_version.h.in
        src/framework/CountryCodes.h
        src/framework/KafkaTopics.h
        src/framework/MicroService.h
        src/framework/OpenWifiTypes.h
        src/framework/orm.h
        src/framework/StorageClass.h
        src/framework/ConfigurationValidator.cpp
        src/framework/ConfigurationValidator.h
        src/framework/ow_constants.h
        src/RESTAPI/RESTAPI_routers.cpp
        src/Daemon.cpp src/Daemon.h
        src/Dashboard.cpp src/Dashboard.h
        src/uCentralClient.cpp
        src/uCentralClient.h
        src/Simulator.cpp src/Simulator.h
        src/uCentralEvent.cpp src/uCentralEvent.h
        src/uCentralEventTypes.h
        src/SimStats.h
        src/RESTObjects/RESTAPI_OWLSobjects.cpp src/RESTObjects/RESTAPI_OWLSobjects.h
        src/RESTObjects/RESTAPI_SecurityObjects.cpp src/RESTObjects/RESTAPI_SecurityObjects.h
        src/RESTAPI/RESTAPI_deviceDashboardHandler.cpp src/RESTAPI/RESTAPI_deviceDashboardHandler.h
        src/StorageService.cpp src/StorageService.h src/storage/storage_simulations.cpp src/storage/storage_simulations.h src/storage/storage_results.cpp src/storage/storage_results.h src/RESTAPI/RESTAPI_simulation_handler.cpp src/RESTAPI/RESTAPI_simulation_handler.h src/RESTAPI/RESTAPI_results_handler.cpp src/RESTAPI/RESTAPI_results_handler.h src/RESTAPI/RESTAPI_status_handler.cpp src/RESTAPI/RESTAPI_status_handler.h src/RESTAPI/RESTAPI_operation_handler.cpp src/RESTAPI/RESTAPI_operation_handler.h src/Simulation.cpp src/Simulation.h)

target_link_libraries(owls PRIVATE
        ${Poco_LIBRARIES}
        ${PostgreSQL_LIBRARIES}
        ${MySQL_LIBRARIES}
        ${ZLIB_LIBRARIES}
        fmt::fmt
        CppKafka::cppkafka
        nlohmann_json_schema_validator)