
#include "SqliteConnection.h"

#include "../../infra/log/Logger.h"

SqliteConnection::SqliteConnection(const std::string &dbPath)
{
    if (sqlite3_open(dbPath.c_str(), &this->_db) != SQLITE_OK)
    {
        std::string err = sqlite3_errmsg(this->_db);
        sqlite3_close(this->_db);
        this->_db = nullptr;
        this->_isConnected = false;
        LOG_ERROR << "SQLite open failed: " << err << std::endl;
        return;
    }

    // 推荐设置：提高并发友好性
    sqlite3_exec(this->_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(this->_db, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);

    this->_isConnected = true;
    LOG_INFO << "SQLite Connection Success -> " << dbPath << std::endl;
}

SqliteConnection::~SqliteConnection()
{
    if (this->_db)
    {
        sqlite3_close(this->_db);
        this->_db = nullptr;
        this->_isConnected = false;
    }
}

bool SqliteConnection::IsValid() const
{
    return this->_isConnected;
}

bool SqliteConnection::Execute(const std::string &sql, DBResult &out)
{
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        out.errorMsg = sqlite3_errmsg(_db);
        return false;
    }

    int colCount = sqlite3_column_count(stmt);

    if (colCount > 0)
    {
        out.type = DBResult::Type::RESULT_SET;

        for (int i = 0; i < colCount; ++i)
            out.columns.emplace_back(sqlite3_column_name(stmt, i));
    }

    while (true)
    {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW)
        {
            std::vector<std::string> r;
            for (int i = 0; i < colCount; ++i)
            {
                const char *txt =
                    reinterpret_cast<const char *>(sqlite3_column_text(stmt, i));
                r.emplace_back(txt ? txt : "");
            }
            out.rows.emplace_back(std::move(r));
        }
        else if (rc == SQLITE_DONE)
        {
            break;
        }
        else
        {
            out.errorMsg = sqlite3_errmsg(_db);
            sqlite3_finalize(stmt);
            return false;
        }
    }

    if (out.type != DBResult::Type::RESULT_SET)
    {
        out.type = DBResult::Type::EXEC_RESULT;
        out.affectedRows = sqlite3_changes(_db);
        out.lastInsertId = sqlite3_last_insert_rowid(_db);
    }

    sqlite3_finalize(stmt);
    return true;
}
