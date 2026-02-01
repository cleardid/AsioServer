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
elseif(APPLE)
    cmake_policy(SET CMP0144 NEW)
    cmake_policy(SET CMP0167 NEW)
    # 1. 查找 Boost (不再手动 set LIB_DIR)
    # 如果你安装了特定版本(boost@1.85)，可以保留 BOOST_ROOT 作为提示，
    # 但核心是靠 find_package 自动找库文件
    set(BOOST_ROOT "/opt/homebrew/opt/boost@1.85")
    find_package(Boost REQUIRED COMPONENTS system thread charconv url context)

    # 2. 查找 OpenSSL
    set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl@3") 
    find_package(OpenSSL REQUIRED)

    # 3. 查找 MySQL (手动路径通常还需要，除非用 pkg-config)
    set(MYSQL_ROOT "/opt/homebrew/opt/mysql@8.0")
    set(MYSQL_INCLUDE_DIR "${MYSQL_ROOT}/include")
    set(MYSQL_LIB_DIR "${MYSQL_ROOT}/lib")

    # 4. 查找 SQLite3
    set(SQLITE3_ROOT "/opt/homebrew/opt/sqlite")
    set(SQLITE3_INCLUDE_DIR "${SQLITE3_ROOT}/include")
    set(SQLITE3_LIB_DIR "${SQLITE3_ROOT}/lib")
endif()

# 添加源文件
add_executable(${PROJECT_NAME}
    ./app/server_coroutine/main.cpp

    ./config/ConfigReader.cpp
    ./config/ConfigManager.cpp

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
    ${BOOST_INCLUDE_DIRS}  # 使用 find_package 自动生成的变量
    ${MYSQL_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR} # 使用 find_package 自动生成的变量

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

elseif(APPLE)

    # MacOS 手动指定库目录 (仅针对 find_package 找不到的库，如 MySQL)
    target_link_directories(${PROJECT_NAME} PRIVATE
        ${MYSQL_LIB_DIR}
        ${SQLITE3_LIB_DIR}
    )

    # MacOS 链接
    target_link_libraries(${PROJECT_NAME} PRIVATE
        # ✅ 使用 Boost::xxx 目标 (自动处理 boost_thread-mt vs boost_thread)
        Boost::system
        Boost::thread
        Boost::url      
        Boost::charconv 
        Boost::context
        
        # ✅ 使用 OpenSSL::xxx 目标
        OpenSSL::SSL
        OpenSSL::Crypto
        
        # 传统库链接
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