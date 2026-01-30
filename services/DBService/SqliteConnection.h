
#ifndef SQLITECONNECTION_H
#define SQLITECONNECTION_H

#include <sqlite3.h>
#include <mutex>

#include "DBConnection.h"

class SqliteConnection : public DBConnection
{
public:
    explicit SqliteConnection(const std::string &dbPath);
    ~SqliteConnection();

    bool IsValid() const override;

    boost::asio::awaitable<bool> Execute(const std::string &sql,
                                         DBResult &out) override;

private:
    sqlite3 *_db;
    std::mutex _mtx; // SQLite 本身不是线程安全的
};

#endif // SQLITECONNECTION_H