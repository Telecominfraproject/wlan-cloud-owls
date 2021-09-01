cmake_minimum_required(VERSION 3.13)
project(owls VERSION 2.1.0)

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
    file(READ build BUILD_NUM)
    if(BUILD_INCREMENT)
        MATH(EXPR BUILD_NUM "${BUILD_NUM}+1")
        file(WRITE build ${BUILD_NUM})
    endif()
else()
    set(BUILD_NUM 1)
    file(WRITE build ${BUILD_NUM})
endif()

add_definitions(-DAPP_VERSION="${CMAKE_PROJECT_VERSION}" -DBUILD_NUMBER="${BUILD_NUM}")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED system)
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(MySQL REQUIRED)
find_package(ODBC REQUIRED)
find_package(ZLIB REQUIRED)
find_package(CppKafka REQUIRED)
find_package(Poco REQUIRED COMPONENTS Crypto Net Util NetSSL Data DataSQLite DataPostgreSQL DataMySQL DataODBC)

include_directories(/usr/local/include  /usr/local/opt/openssl/include src)

add_executable( owls
                build
                src/Daemon.cpp src/Daemon.h
                src/Dashboard.cpp src/Dashboard.h
                src/AuthClient.cpp src/AuthClient.h
                src/uCentralClient.cpp src/uCentralClient.h
                src/uCentralClientApp.cpp src/uCentralClientApp.h
                src/Simulator.cpp src/Simulator.h
                src/uCentralEvent.cpp src/uCentralEvent.h
                src/uCentralEventTypes.h
                src/SimStats.cpp src/SimStats.h
                src/StatsDisplay.cpp src/StatsDisplay.h
                src/Utils.h src/Utils.cpp
                src/Kafka_topics.h src/KafkaManager.cpp src/KafkaManager.h
                src/MicroService.h src/MicroService.cpp
                src/SubSystemServer.cpp src/SubSystemServer.h
                src/RESTAPI_OWLSobjects.cpp src/RESTAPI_OWLSobjects.h
                src/OpenAPIRequest.cpp src/OpenAPIRequest.h
                src/RESTAPI_handler.cpp src/RESTAPI_handler.h
                src/RESTAPI_SecurityObjects.cpp src/RESTAPI_SecurityObjects.h
                src/RESTAPI_utils.cpp src/RESTAPI_utils.h
                src/RESTAPI_server.cpp src/RESTAPI_server.h
                src/RESTAPI_InternalServer.cpp src/RESTAPI_InternalServer.h
                src/RESTAPI_deviceDashboardHandler.cpp src/RESTAPI_deviceDashboardHandler.h
                src/RESTAPI_system_command.cpp src/RESTAPI_system_command.h
                src/ALBHealthCheckServer.h
                )

target_link_libraries(owls PRIVATE
        ${Poco_LIBRARIES} ${Boost_LIBRARIES} ${PostgreSQL_LIBRARIES}
        ${MySQL_LIBRARIES} ${ODBC_LIBRARIES} ${ZLIB_LIBRARIES}
        CppKafka::cppkafka)