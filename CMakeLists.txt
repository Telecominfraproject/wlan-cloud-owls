cmake_minimum_required(VERSION 3.13)
project(owls VERSION 2.10.0)

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
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_RESULT
            OUTPUT_VARIABLE GIT_HASH)
    if(NOT GIT_RESULT EQUAL "0")
        message(FATAL_ERROR "git rev-parse --short HEAD failed with ${GIT_RESULT}")
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
find_package(Poco REQUIRED COMPONENTS Crypto Net Util JWT NetSSL Data DataSQLite DataPostgreSQL DataMySQL)

include_directories(/usr/local/include  /usr/local/opt/openssl/include src include/kafka /usr/local/opt/mysql-client/include)

configure_file(src/ow_version.h.in ${PROJECT_SOURCE_DIR}/src/ow_version.h @ONLY)

add_definitions(-DPOCO_LOG_DEBUG="1" -DBOOST_NO_CXX98_FUNCTION_BASE=1)

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
        src/framework/MicroServiceErrorHandler.h
        src/framework/UI_WebSocketClientServer.cpp
        src/framework/UI_WebSocketClientServer.h
        src/framework/UI_WebSocketClientNotifications.cpp
        src/framework/UI_WebSocketClientNotifications.h
        src/framework/utils.h
        src/framework/utils.cpp
        src/framework/AppServiceRegistry.h
        src/framework/SubSystemServer.cpp
        src/framework/SubSystemServer.h
        src/framework/RESTAPI_utils.h
        src/framework/AuthClient.cpp
        src/framework/AuthClient.h
        src/framework/MicroServiceNames.h
        src/framework/MicroServiceFuncs.h
        src/framework/OpenAPIRequests.cpp
        src/framework/OpenAPIRequests.h
        src/framework/MicroServiceFuncs.cpp
        src/framework/ALBserver.cpp
        src/framework/ALBserver.h
        src/framework/KafkaManager.cpp
        src/framework/KafkaManager.h
        src/framework/RESTAPI_RateLimiter.h
        src/framework/WebSocketLogger.h
        src/framework/RESTAPI_GenericServerAccounting.h
        src/framework/CIDR.h
        src/framework/RESTAPI_Handler.cpp
        src/framework/RESTAPI_Handler.h
        src/framework/RESTAPI_ExtServer.h
        src/framework/RESTAPI_ExtServer.cpp
        src/framework/RESTAPI_IntServer.cpp
        src/framework/RESTAPI_IntServer.h
        src/framework/RESTAPI_SystemCommand.h
        src/framework/RESTAPI_WebSocketServer.h
        src/framework/EventBusManager.cpp
        src/framework/EventBusManager.h
        src/framework/RESTAPI_PartHandler.h
        src/framework/MicroService.cpp
        src/framework/MicroServiceExtra.h
        src/framework/default_device_types.h
        src/RESTObjects/RESTAPI_SecurityObjects.h src/RESTObjects/RESTAPI_SecurityObjects.cpp
        src/RESTObjects/RESTAPI_GWobjects.h src/RESTObjects/RESTAPI_GWobjects.cpp
        src/RESTObjects/RESTAPI_FMSObjects.h src/RESTObjects/RESTAPI_FMSObjects.cpp
        src/RESTObjects/RESTAPI_CertObjects.cpp src/RESTObjects/RESTAPI_CertObjects.h
        src/RESTObjects/RESTAPI_OWLSobjects.cpp src/RESTObjects/RESTAPI_OWLSobjects.h
        src/RESTObjects/RESTAPI_ProvObjects.cpp src/RESTObjects/RESTAPI_ProvObjects.h
        src/RESTObjects/RESTAPI_AnalyticsObjects.cpp src/RESTObjects/RESTAPI_AnalyticsObjects.h
        src/RESTAPI/RESTAPI_routers.cpp
        src/Daemon.cpp src/Daemon.h
        src/Dashboard.cpp src/Dashboard.h
        src/OWLSclient.cpp
        src/OWLSclient.h
        src/SimulationRunner.cpp src/SimulationRunner.h
#        src/OWLSevent.cpp src/OWLSevent.h
        src/OWLSdefinitions.h
        src/SimStats.h
        src/RESTAPI/RESTAPI_deviceDashboardHandler.cpp src/RESTAPI/RESTAPI_deviceDashboardHandler.h
        src/StorageService.cpp src/StorageService.h src/storage/storage_simulations.cpp src/storage/storage_simulations.h src/storage/storage_results.cpp src/storage/storage_results.h src/RESTAPI/RESTAPI_simulation_handler.cpp src/RESTAPI/RESTAPI_simulation_handler.h src/RESTAPI/RESTAPI_results_handler.cpp src/RESTAPI/RESTAPI_results_handler.h src/RESTAPI/RESTAPI_status_handler.cpp src/RESTAPI/RESTAPI_status_handler.h src/RESTAPI/RESTAPI_operation_handler.cpp src/RESTAPI/RESTAPI_operation_handler.h src/SimulationCoordinator.cpp src/SimulationCoordinator.h src/UI_Owls_WebSocketNotifications.cpp src/UI_Owls_WebSocketNotifications.h src/OWLS_utils.h src/MockElements.h src/libs/ctpl_stl.h src/libs/Cron.h src/libs/InterruptableSleep.h src/libs/Scheduler.h src/OWLS_EstablishConnection.cpp src/OWLS_Reconnect.cpp src/OWLS_Connect.cpp src/OWLSclientEvents.h src/OWLS_State.cpp src/OWLS_HealthCheck.cpp src/OWLS_Log.cpp src/OWLS_Update.cpp src/OWLS_WSPing.cpp src/OWLS_KeepAlive.cpp src/OWLS_Disconnect.cpp src/OWLS_PendingConfig.cpp src/OWLS_CrashLog.cpp src/CensusReport.h)

target_link_libraries(owls PRIVATE
        ${Poco_LIBRARIES}
        ${PostgreSQL_LIBRARIES}
        ${MySQL_LIBRARIES}
        ${ZLIB_LIBRARIES}
        fmt::fmt
        CppKafka::cppkafka
        resolv
)
