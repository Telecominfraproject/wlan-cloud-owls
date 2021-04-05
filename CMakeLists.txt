cmake_minimum_required(VERSION 3.17)
project(ucentral_clnt)

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

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED system)
find_package(OpenSSL REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(MySQL REQUIRED)
find_package(ODBC REQUIRED)
find_package(Poco REQUIRED COMPONENTS Crypto Net Util NetSSL Data DataSQLite DataPostgreSQL DataMySQL DataODBC)

include_directories(/usr/local/include  /usr/local/opt/openssl/include src)

add_executable(ucentral_clnt src/main.cpp src/uCentralClient.cpp src/uCentralClient.h src/uCentralClientApp.cpp src/uCentralClientApp.h src/Simulator.cpp src/Simulator.h src/uCentralEvent.cpp src/uCentralEvent.h src/uCentralCommand.cpp src/uCentralCommand.h src/uCentralEventTypes.h)

target_link_libraries(ucentral_clnt PRIVATE yaml-cpp
        ${Poco_LIBRARIES} ${Boost_LIBRARIES} ${PostgreSQL_LIBRARIES}
        ${MySQL_LIBRARIES} ${ODBC_LIBRARIES})