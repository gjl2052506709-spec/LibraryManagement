#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include<QString>
#include<QSqlDatabase>
#include<QSqlError>
#include<QSqlQuery>

class DBConnection
{
public:
    // 禁止拷贝和赋值（单例模式）
    DBConnection(const DBConnection&) = delete;
    DBConnection& operator=(const DBConnection&) = delete;

    // 获取单例实例
    static DBConnection& getInstance();

    // 连接数据库
    bool connect(const QString& host = "localhost",
                 const QString& database = "library_db",
                 const QString& username = "root",
                 const QString& password = "gjl123456",
                 int port = 3306) ;

    // 断开数据库连接
    void disconnect();

    // 检查连接是否有效
    bool isConnected() const;

    // 执行查询（返回结果集）
    std::unique_ptr<QSqlQuery> executeQuery(const QString& sql);

    // 执行更新（INSERT、UPDATE、DELETE），返回影响行数
    int executeUpdate(const QString& sql);

    // 执行预处理语句（防止SQL注入）
    std::unique_ptr<QSqlQuery> prepareQuery(const QString& sql);

    // 获取最后一次错误信息
    QString getLastError() const;

    // 开启事务
    bool beginTransaction();

    // 提交事务
    bool commitTransaction();

    // 回滚事务
    bool rollbackTransaction();

    // 获取数据库连接引用（供其他类使用）
    QSqlDatabase& getDatabase();

private:
    // 私有构造函数（单例）
    DBConnection();
    ~DBConnection();

    // 初始化数据库驱动
    bool initDriver();

    //构建 ODBC 连接字符串的辅助函数
    QString buildODBCConnectionString(const QString& host, const QString& database,
                                      const QString& username, const QString& password,
                                      int port);

private:
    QSqlDatabase m_db;          // 数据库连接对象
    QString m_lastError;        // 最后一次错误信息
    bool m_isConnected;         // 连接状态

    // 数据库连接参数
    QString m_host;
    QString m_database;
    QString m_username;
    QString m_password;
    int m_port;
};



#endif // DBCONNECTION_H
