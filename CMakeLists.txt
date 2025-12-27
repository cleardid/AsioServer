cmake_minimum_required(VERSION 3.15)
project(AsioServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    # MSYS2 + MinGW64
    cmake_policy(SET CMP0167 NEW)

    find_package(Boost CONFIG REQUIRED COMPONENTS system thread)
    find_package(SQLite3 REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MARIADB REQUIRED libmariadb)
else()
    # MacOS 配置
    set(BOOST_ROOT "/opt/homebrew/opt/boost@1.85")
    set(BOOST_INCLUDE_DIR "${BOOST_ROOT}/include")
    set(BOOST_LIB_DIR "${BOOST_ROOT}/lib")

    set(MYSQL_ROOT "/opt/homebrew/opt/mysql@8.0")
    set(MYSQL_INCLUDE_DIR "${MYSQL_ROOT}/include")
    set(MYSQL_LIB_DIR "${MYSQL_ROOT}/lib")

    set(SQLITE3_ROOT "/opt/homebrew/opt/sqlite")
    set(SQLITE3_INCLUDE_DIR "${SQLITE3_ROOT}/include")
    set(SQLITE3_LIB_DIR "${SQLITE3_ROOT}/lib")
endif()

# 添加源文件
add_executable(${PROJECT_NAME}
    ./app/server_coroutine/main.cpp

    ./config/ConfigReader.cpp

    ./core/logic/LogicSystem.cpp
    ./core/message/MsgNode.cpp
    ./core/server/CServer.cpp
    ./core/session/AsioIOServicePool.cpp
    ./core/session/CSession.cpp

    ./infra/log/Logger.cpp
    ./infra/util/PathUtils.cpp

    ./net/threaded/AsyncSession.cpp
    ./net/coroutine/CoroutineSession.cpp

    ./services/IService.cpp
    ./services/ServiceManager.cpp
    ./services/HelloService/HelloService.cpp
    ./services/DBService/DBConnectionPool.cpp
    ./services/DBService/DBExecutor.cpp
    ./services/DBService/DBService.cpp
    ./services/DBService/DBServiceRegister.cpp
    ./services/DBService/MySQLConnection.cpp
    ./services/DBService/MySQLConnectionPool.cpp
    ./services/DBService/SQLiteConnection.cpp
    ./services/DBService/SQLiteConnectionPool.cpp
)

# 添加包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${BOOST_INCLUDE_DIR}
    ${MYSQL_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIR}

    ${CMAKE_SOURCE_DIR}

    ${CMAKE_SOURCE_DIR}/config
    ${CMAKE_SOURCE_DIR}/core/common
    ${CMAKE_SOURCE_DIR}/core/logic
    ${CMAKE_SOURCE_DIR}/core/message
    ${CMAKE_SOURCE_DIR}/core/protocol
    ${CMAKE_SOURCE_DIR}/core/server
    ${CMAKE_SOURCE_DIR}/core/session

    ${CMAKE_SOURCE_DIR}/infra/util
    ${CMAKE_SOURCE_DIR}/infra/log

    ${CMAKE_SOURCE_DIR}/services
    ${CMAKE_SOURCE_DIR}/services/HelloService
    ${CMAKE_SOURCE_DIR}/services/DBService
)

if(WIN32)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${MARIADB_INCLUDE_DIRS}
    )
else()
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${BOOST_INCLUDE_DIR}
        ${MYSQL_INCLUDE_DIR}
        ${SQLITE3_INCLUDE_DIR}
    )
endif()

if(WIN32)

    # target_link_libraries(${PROJECT_NAME} PRIVATE
    #     boost_system
    #     boost_thread
    #     mysqlclient       # 或 libmariadb
    #     sqlite3
    #     ws2_32
    #     mswsock           # 必须加
    #     bcrypt            # 推荐
    # )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        mariadb
        sqlite3
        ws2_32
        mswsock
        bcrypt
    )

else()

    target_link_directories(${PROJECT_NAME} PRIVATE
        ${BOOST_LIB_DIR}
        ${MYSQL_LIB_DIR}
        ${SQLITE3_LIB_DIR}
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        boost_system
        mysqlclient
        sqlite3
    )

endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
# Windows下Debug/Release分开输出（可选，方便管理）
if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release")
endif()