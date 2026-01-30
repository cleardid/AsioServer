
#include "MySQLConnection.h"
#include "../../infra/log/Logger.h"

#include <sstream>

MySQLConnection::MySQLConnection(boost::asio::io_context &ioc,
                                 const std::string &host,
                                 const uint16_t &port,
                                 const std::string &user,
                                 const std::string &pwd,
                                 const std::string &db)
    : _conn(ioc)
{
    try
    {
        // 1. 设置连接参数，包含主机、端口和认证信息
        boost::mysql::connect_params params;
        params.server_address.emplace_host_and_port(host, port);
        params.username = user;
        params.password = pwd;
        params.database = db;

        // 2. 同步连接
        _conn.connect(params);

        this->_isConnected = true;
        LOG_INFO << "MySQL Connection Success -> " << host << ":" << port << "/" << db << std::endl;
    }
    catch (const std::exception &e)
    {
        this->_isConnected = false;
        LOG_ERROR << "MySQL Connection Failed: " << e.what() << std::endl;
    }
}

MySQLConnection::~MySQLConnection()
{
    // any_connection 会自动关闭连接，无需显式检查
    // 析构函数中不需要额外的清理操作
}

bool MySQLConnection::IsValid() const
{
    return this->_isConnected;
}

boost::asio::awaitable<bool> MySQLConnection::Execute(const std::string &sql, DBResult &out)
{
    // 安全性检查
    if (!this->_isConnected)
    {
        out.errorMsg = "MySQL Connection Not Valid";
        co_return false;
    }

    try
    {
        boost::mysql::results result;

        // ==========================================
        // 关键点：使用 co_await 进行异步非阻塞调用
        // 这会释放当前线程去处理其他任务，直到数据库返回
        // ==========================================
        co_await _conn.async_execute(sql, result, boost::asio::use_awaitable);

        // 处理结果集
        if (result.has_value())
        {
            // 1. 处理元数据 (Affected Rows / Insert ID)
            out.affectedRows = result.affected_rows();
            out.lastInsertId = result.last_insert_id();

            // 2. 判断是否有数据行 (SELECT 语句)
            if (result.has_value() && !result.rows().empty())
            {
                out.type = DBResult::Type::RESULT_SET;

                // 2.1 获取列名
                // Boost.MySQL 的列名在 meta() 中
                auto meta = result.meta();
                for (const auto &col : meta)
                {
                    out.columns.emplace_back(col.column_name());
                }

                // 2.2 获取行数据
                // 遍历 rows()
                for (auto row : result.rows())
                {
                    std::vector<std::string> r;
                    for (auto field : row)
                    {
                        // 将字段转换为 string，处理 NULL
                        if (field.is_null())
                        {
                            r.emplace_back("");
                        }
                        else
                        {
                            // as_string() 可能会抛异常如果类型不匹配，
                            // 生产环境建议用 stream 或者 field.to_string() (视 boost 版本)
                            std::stringstream ss;
                            ss << field;
                            r.emplace_back(ss.str());
                        }
                    }
                    out.rows.emplace_back(std::move(r));
                }
            }
            else
            {
                // 非 SELECT (INSERT/UPDATE/DELETE)
                out.type = DBResult::Type::EXEC_RESULT;
            }
        }

        co_return true;
    }
    catch (const boost::system::system_error &e)
    {
        out.errorMsg = e.code().message(); // 获取错误信息
        LOG_ERROR << "MySQL Async Execute Error: " << out.errorMsg << std::endl;

        // 如果连接断开，可能需要重置状态
        // this->_isConnected = false;
        co_return false;
    }
    catch (const std::exception &e)
    {
        out.errorMsg = e.what();
        LOG_ERROR << "MySQL Exception: " << e.what() << std::endl;
        co_return false;
    }
}
