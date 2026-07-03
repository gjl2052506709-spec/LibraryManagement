#include "readerdao.h"

ReaderDao::ReaderDao() {}

bool ReaderDao::addReader(const Reader& reader, int& generatedId)
{
    QString sql = "INSERT INTO reader (name, phone, email, address, register_at, is_delete) "
                  "VALUES (?, ?, ?, ?, ?, ?)";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(reader.getName());
    query->addBindValue(reader.getPhone());
    query->addBindValue(reader.getEmail());
    query->addBindValue(reader.getAddress());
    query->addBindValue(reader.getRegisterAt());
    query->addBindValue(reader.getIsDelete() ? 1 : 0);

    if (!query->exec()) {
        qCritical() << "添加读者失败:" << query->lastError().text();
        return false;
    }

    generatedId = query->lastInsertId().toInt();
    return true;
}

bool ReaderDao::deleteReader(int id)
{
    QString sql = "UPDATE reader SET is_delete = 1, delete_at = NOW() WHERE id = ? AND is_delete = 0";
    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(id);

    if (!query->exec()) {
        qCritical() << "软删除读者失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

bool ReaderDao::deleteReader(const QString& name)
{
    QString sql = "UPDATE reader SET is_delete = 1, delete_at = NOW() WHERE name = ? AND is_delete = 0";
    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(name);

    if (!query->exec()) {
        qCritical() << "软删除读者失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

bool ReaderDao::updateReader(const Reader& reader)
{
    QString sql = "UPDATE reader SET name = ?, phone = ?, email = ?, address = ?, "
                  "is_delete = ? WHERE id = ?";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(reader.getName());
    query->addBindValue(reader.getPhone());
    query->addBindValue(reader.getEmail());
    query->addBindValue(reader.getAddress());
    query->addBindValue(reader.getIsDelete() ? 1 : 0);
    query->addBindValue(reader.getId());

    if (!query->exec()) {
        qCritical() << "更新读者失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

std::unique_ptr<Reader> ReaderDao::getReaderById(int id)
{
    QString sql = "SELECT id, name, phone, email, address, register_at, "
                  "is_delete, delete_at FROM reader WHERE id = ?";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return nullptr;
    }

    query->addBindValue(id);

    if (!query->exec()) {
        qCritical() << "查询读者失败:" << query->lastError().text();
        return nullptr;
    }

    if (query->next()) {
        return std::make_unique<Reader>(mapToReader(*query));
    }

    return nullptr;
}

std::unique_ptr<Reader> ReaderDao::getReaderByPhone(const QString& phone)
{
    QString sql = "SELECT id, name, phone, email, address, register_at, "
                  "is_delete, delete_at FROM reader WHERE phone = ?";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return nullptr;
    }

    query->addBindValue(phone);

    if (!query->exec()) {
        qCritical() << "查询读者失败:" << query->lastError().text();
        return nullptr;
    }

    if (query->next()) {
        return std::make_unique<Reader>(mapToReader(*query));
    }

    return nullptr;
}

QList<Reader> ReaderDao::getAllReaders(bool includeDeleted)
{
    QList<Reader> readers;
    QString sql = "SELECT id, name, phone, email, address, register_at, "
                  "is_delete, delete_at FROM reader " + buildWhereClause(includeDeleted, false);

    auto query = DBConnection::getInstance().executeQuery(sql);
    if (!query) {
        return readers;
    }

    while (query->next()) {
        readers.push_back(mapToReader(*query));
    }

    return readers;
}

QList<Reader> ReaderDao::searchReaders(const QString& keyWord, bool includeDeleted)
{
    QList<Reader> readers;
    QString sql = "SELECT id, name, phone, email, address, register_at, "
                  "is_delete, delete_at FROM reader WHERE (name LIKE ? OR phone LIKE ? OR email LIKE ?)";

    if (!includeDeleted) {
        sql += " AND is_delete = 0";
    }

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return readers;
    }

    QString pattern = "%" + keyWord + "%";
    query->addBindValue(pattern);
    query->addBindValue(pattern);
    query->addBindValue(pattern);

    if (!query->exec()) {
        qCritical() << "搜索读者失败:" << query->lastError().text();
        return readers;
    }

    while (query->next()) {
        readers.push_back(mapToReader(*query));
    }

    return readers;
}

QList<Reader> ReaderDao::getReadersByName(const QString& name, bool includeDeleted)
{
    QList<Reader> readers;
    QString sql = "SELECT id, name, phone, email, address, register_at, "
                  "is_delete, delete_at FROM reader WHERE name LIKE ? " + buildWhereClause(includeDeleted, true);

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return readers;
    }

    QString pattern = "%" + name + "%";
    query->addBindValue(pattern);

    if (!query->exec()) {
        qCritical() << "按姓名查询读者失败:" << query->lastError().text();
        return readers;
    }

    while (query->next()) {
        readers.push_back(mapToReader(*query));
    }

    return readers;
}

bool ReaderDao::batchAddReaders(const QList<Reader>& readers)
{
    auto& db = DBConnection::getInstance();

    if (!db.beginTransaction()) {
        return false;
    }

    QString sql = "INSERT INTO reader (name, phone, email, address, register_at, is_delete) "
                  "VALUES (?, ?, ?, ?, ?, ?)";

    for (const auto& reader : readers) {
        auto query = db.prepareQuery(sql);
        if (!query) {
            db.rollbackTransaction();
            return false;
        }

        query->addBindValue(reader.getName());
        query->addBindValue(reader.getPhone());
        query->addBindValue(reader.getEmail());
        query->addBindValue(reader.getAddress());
        query->addBindValue(reader.getRegisterAt());
        query->addBindValue(reader.getIsDelete() ? 1 : 0);

        if (!query->exec()) {
            qCritical() << "批量添加读者失败:" << query->lastError().text();
            db.rollbackTransaction();
            return false;
        }
    }

    return db.commitTransaction();
}

bool ReaderDao::batchDeleteReaders(const QList<int>& ids)
{
    if (ids.empty()) {
        return true;
    }

    auto& db = DBConnection::getInstance();

    if (!db.beginTransaction()) {
        return false;
    }

    QString sql = "UPDATE reader SET is_delete = 1, delete_at = NOW() WHERE id = ? AND is_delete = 0";

    for (int id : ids) {
        auto query = db.prepareQuery(sql);
        if (!query) {
            db.rollbackTransaction();
            return false;
        }

        query->addBindValue(id);

        if (!query->exec()) {
            qCritical() << "批量删除读者失败:" << query->lastError().text();
            db.rollbackTransaction();
            return false;
        }
    }

    return db.commitTransaction();
}

bool ReaderDao::restoreReader(int id)
{
    QString sql = "UPDATE reader SET is_delete = 0, delete_at = NULL WHERE id = ? AND is_delete = 1";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(id);

    if (!query->exec()) {
        qCritical() << "恢复读者失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

bool ReaderDao::restoreReader(const QString& phone)
{
    QString sql = "UPDATE reader SET is_delete = 0, delete_at = NULL WHERE phone = ? AND is_delete = 1";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(phone);

    if (!query->exec()) {
        qCritical() << "恢复读者失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

int ReaderDao::getReaderTotalCount(bool includeDeleted)
{
    QString sql = "SELECT COUNT(*) FROM reader " + buildWhereClause(includeDeleted, false);

    auto query = DBConnection::getInstance().executeQuery(sql);
    if (!query) {
        return 0;
    }

    if (query->next()) {
        return query->value(0).toInt();
    }

    return 0;
}

int ReaderDao::getActiveReaderCount()
{
    QString sql = "SELECT COUNT(*) FROM reader " + buildWhereClause(false, false);

    auto query = DBConnection::getInstance().executeQuery(sql);
    if (!query) {
        return 0;
    }

    if (query->next()) {
        return query->value(0).toInt();
    }

    return 0;
}

Reader ReaderDao::mapToReader(const QSqlQuery& query)
{
    Reader reader;
    reader.setId(query.value("id").toInt());
    reader.setName(query.value("name").toString());
    reader.setPhone(query.value("phone").toString());
    reader.setEmail(query.value("email").toString());
    reader.setAddress(query.value("address").toString());
    reader.setRegisterAt(query.value("register_at").toDateTime());
    reader.setIsDelete(query.value("is_delete").toInt() == 1);
    reader.setDeleteAt(query.value("delete_at").toDateTime());

    return reader;
}

QString ReaderDao::buildWhereClause(bool includeDeleted, bool hasWhere) const
{
    if (includeDeleted) {
        return "";
    }
    return hasWhere ? " AND is_delete = 0" : " WHERE is_delete = 0";
}
