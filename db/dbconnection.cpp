#include "dbconnection.h"

DBConnection::DBConnection()
    : m_isConnected(false)
    , m_port(3306) {
    initDriver();
}

DBConnection::~DBConnection() {
    disconnect();
}

bool DBConnection::initDriver() {
    QStringList drivers = QSqlDatabase::drivers();

    // 检查是否有 ODBC 驱动
    if (!drivers.contains("QODBC")) {
        m_lastError = "ODBC驱动(QODBC)未找到！请确保Qt安装了ODBC插件。";
        qCritical() << m_lastError;
        qDebug() << "可用的驱动：" << drivers;
        return false;
    }

    qDebug() << "ODBC驱动可用";
    return true;
}

QString DBConnection::buildODBCConnectionString(const QString &host, const QString &database, const QString &username, const QString &password, int port)
{
    // 构建 ODBC 连接字符串
    // 直接连接
    return QString(
               "DRIVER={MySQL ODBC 9.2 Unicode Driver};"
               "SERVER=%1;"
               "PORT=%2;"
               "DATABASE=%3;"
               "USER=%4;"
               "PASSWORD=%5;"
               "OPTION=3;"
               "CHARSET=utf8mb4;"
               ).arg(host)
        .arg(port)
        .arg(database)
        .arg(username)
        .arg(password);
}

DBConnection &DBConnection::getInstance()
{
    static DBConnection instance;
    return instance;
}

bool DBConnection::connect(const QString &host, const QString &database, const QString &username, const QString &password, int port)
{
    // 如果已经连接，先断开
    if (m_isConnected) {
        disconnect();
    }

    // 保存连接参数
    m_host = host;
    m_database = database;
    m_username = username;
    m_password = password;
    m_port = port;

    // 创建 ODBC 数据库连接
    m_db = QSqlDatabase::addDatabase("QODBC", "library_connection");

    // 构建 ODBC 连接字符串
    QString connectionString = buildODBCConnectionString(host, database, username, password, port);
    m_db.setDatabaseName(connectionString);

    qDebug() << "ODBC连接字符串：" << connectionString;

    // 尝试打开连接
    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        m_isConnected = false;
        qCritical() << "数据库连接失败：" << m_lastError;
        return false;
    }

    m_isConnected = true;
    m_lastError.clear();
    qDebug() << "数据库连接成功！" << database << "@" << host << ":" << port;
    return true;
}

void DBConnection::disconnect() {
    if (m_isConnected) {
        m_db.close();
        m_isConnected = false;
        qDebug() << "数据库已断开连接";
    }
}

bool DBConnection::isConnected() const {
    return m_isConnected && m_db.isOpen();
}

std::unique_ptr<QSqlQuery> DBConnection::executeQuery(const QString& sql) {
    auto query = std::make_unique<QSqlQuery>(m_db);

    if (!query->exec(sql)) {
        m_lastError = query->lastError().text();
        qCritical() << "查询执行失败：" << sql;
        qCritical() << "错误信息：" << m_lastError;
        return nullptr;
    }

    return query;
}

int DBConnection::executeUpdate(const QString& sql) {
    QSqlQuery query(m_db);

    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        qCritical() << "更新执行失败：" << sql;
        qCritical() << "错误信息：" << m_lastError;
        return -1;
    }

    return query.numRowsAffected();
}

std::unique_ptr<QSqlQuery> DBConnection::prepareQuery(const QString& sql) {
    auto query = std::make_unique<QSqlQuery>(m_db);

    if (!query->prepare(sql)) {
        m_lastError = query->lastError().text();
        qCritical() << "预处理语句失败：" << sql;
        qCritical() << "错误信息：" << m_lastError;
        return nullptr;
    }

    return query;
}

QString DBConnection::getLastError() const {
    return m_lastError;
}

bool DBConnection::beginTransaction() {
    if (!m_isConnected) {
        m_lastError = "数据库未连接，无法开启事务";
        return false;
    }

    if (!m_db.transaction()) {
        m_lastError = m_db.lastError().text();
        qCritical() << "开启事务失败：" << m_lastError;
        return false;
    }

    return true;
}

bool DBConnection::commitTransaction() {
    if (!m_isConnected) {
        m_lastError = "数据库未连接，无法提交事务";
        return false;
    }

    if (!m_db.commit()) {
        m_lastError = m_db.lastError().text();
        qCritical() << "提交事务失败：" << m_lastError;
        return false;
    }

    return true;
}

bool DBConnection::rollbackTransaction() {
    if (!m_isConnected) {
        m_lastError = "数据库未连接，无法回滚事务";
        return false;
    }

    if (!m_db.rollback()) {
        m_lastError = m_db.lastError().text();
        qCritical() << "回滚事务失败：" << m_lastError;
        return false;
    }

    return true;
}

QSqlDatabase& DBConnection::getDatabase() {
    return m_db;
}