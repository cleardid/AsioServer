cmake_minimum_required(VERSION 3.15)
project(AsioServer)

set(CMAKE_CXX_STANDARD 20)

set(BOOST_ROOT "/opt/homebrew/opt/boost@1.85")
set(BOOST_INCLUDE_DIR "${BOOST_ROOT}/include")
set(BOOST_LIB_DIR "${BOOST_ROOT}/lib")

set(MYSQL_ROOT "/opt/homebrew/opt/mysql@8.0")
set(MYSQL_INCLUDE_DIR "${MYSQL_ROOT}/include")
set(MYSQL_LIB_DIR "${MYSQL_ROOT}/lib")

set(SQLITE3_ROOT "/opt/homebrew/opt/sqlite")
set(SQLITE3_INCLUDE_DIR "${SQLITE3_ROOT}/include")
set(SQLITE3_LIB_DIR "${SQLITE3_ROOT}/lib")

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

    /config

    /core/common
    /core/logic
    /core/message
    /core/protocol
    /core/server
    /core/session

    /infra/util
    /infra/log

    /services
    /services/HelloService
    /services/DBService

)

# 添加库目录并链接库文件
target_link_directories(${PROJECT_NAME} PRIVATE
    ${BOOST_LIB_DIR}
    ${MYSQL_LIB_DIR}
    ${SQLITE3_LIB_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    boost_system                   # 直接链接具体的库文件
    mysqlclient
    sqlite3
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)