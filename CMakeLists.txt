cmake_minimum_required(VERSION 3.15)
project(AsioServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    # MSYS2 + MinGW64
    cmake_policy(SET CMP0167 NEW)

    # 1. 增加 charconv, url, context 组件 (Boost.MySQL 需要)
    find_package(Boost CONFIG REQUIRED COMPONENTS system thread charconv url context)
    # 2. 查找 OpenSSL (MySQL 必须)
    find_package(OpenSSL REQUIRED)

    find_package(SQLite3 REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MARIADB REQUIRED libmariadb)
else()
    # MacOS 配置
    set(BOOST_ROOT "/opt/homebrew/opt/boost@1.85")
    set(BOOST_INCLUDE_DIR "${BOOST_ROOT}/include")
    set(BOOST_LIB_DIR "${BOOST_ROOT}/lib")

    # 3. 新增: OpenSSL 路径 (MacOS Homebrew)
    set(OPENSSL_ROOT "/opt/homebrew/opt/openssl@3") # 或者是 openssl@1.1，视安装情况而定
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT}/include")
    set(OPENSSL_LIB_DIR "${OPENSSL_ROOT}/lib")

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

    ./services/CommunicationService/ClientManager.cpp
    ./services/CommunicationService/CommunicationService.cpp
    ./services/CommunicationService/CommunicationServiceRegister.cpp

    ./services/HeartService/HeartService.cpp
    ./services/HeartService/HeartServiceRegister.cpp
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
    ${CMAKE_SOURCE_DIR}/services/CommunicationService
    ${CMAKE_SOURCE_DIR}/services/HeartService

)

if(WIN32)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${MARIADB_INCLUDE_DIRS}
        ${OpenSSL_INCLUDE_DIRS} # Windows 下自动查找的路径
    )
else()
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${BOOST_INCLUDE_DIR}
        ${MYSQL_INCLUDE_DIR}
        ${SQLITE3_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR} # MacOS 下需要手动指定
    )
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        mariadb
        sqlite3
        ws2_32
        mswsock
        bcrypt
        OpenSSL::SSL    # 新增
        OpenSSL::Crypto # 新增
        Boost::url      # 新增
        Boost::charconv # 新增
        Boost::context  # 新增
    )

else()

    target_link_directories(${PROJECT_NAME} PRIVATE
        ${BOOST_LIB_DIR}
        ${MYSQL_LIB_DIR}
        ${SQLITE3_LIB_DIR}
        ${OPENSSL_LIB_DIR} # OpenSSL 路径
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        boost_system
        boost_thread
        boost_url       # 新增: libboost_url.dylib
        boost_charconv  # 新增
        boost_context   # 新增
        ssl             # 新增: libssl
        crypto          # 新增: libcrypto
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