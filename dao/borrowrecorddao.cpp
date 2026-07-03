#include "borrowrecorddao.h"

BorrowRecordDao::BorrowRecordDao() {}

bool BorrowRecordDao::addBorrowRecord(const BorrowRecord& record, int& generatedId)
{
    QString sql = "INSERT INTO borrow_record (reader_id, book_id, borrow_date, due_date, "
                  "return_date, status, fine) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?)";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(record.getReaderId());
    query->addBindValue(record.getBookId());
    query->addBindValue(record.getBorrowDate());
    query->addBindValue(record.getDueDate());
    query->addBindValue(record.getReturnDate());
    query->addBindValue(static_cast<int>(record.getStatus()));
    query->addBindValue(record.getFine());

    if (!query->exec()) {
        qCritical() << "添加借阅记录失败:" << query->lastError().text();
        return false;
    }

    generatedId = query->lastInsertId().toInt();
    return true;
}

bool BorrowRecordDao::deleteBorrowRecord(int id)
{
    QString sql = "DELETE FROM borrow_record WHERE id = ?";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(id);

    if (!query->exec()) {
        qCritical() << "删除借阅记录失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

bool BorrowRecordDao::updateBorrowRecord(const BorrowRecord& record)
{
    QString sql = "UPDATE borrow_record SET reader_id = ?, book_id = ?, borrow_date = ?, "
                  "due_date = ?, return_date = ?, status = ?, fine = ? "
                  "WHERE id = ?";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(record.getReaderId());
    query->addBindValue(record.getBookId());
    query->addBindValue(record.getBorrowDate());
    query->addBindValue(record.getDueDate());
    query->addBindValue(record.getReturnDate());
    query->addBindValue(static_cast<int>(record.getStatus()));
    query->addBindValue(record.getFine());
    query->addBindValue(record.getId());

    if (!query->exec()) {
        qCritical() << "更新借阅记录失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

std::unique_ptr<BorrowRecord> BorrowRecordDao::getBorrowRecordById(int id)
{
    QString sql = "SELECT id, reader_id, book_id, borrow_date, due_date, return_date, "
                  "status, fine FROM borrow_record WHERE id = ?";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return nullptr;
    }

    query->addBindValue(id);

    if (!query->exec()) {
        qCritical() << "查询借阅记录失败:" << query->lastError().text();
        return nullptr;
    }

    if (query->next()) {
        return std::make_unique<BorrowRecord>(mapToBorrowRecord(*query));
    }

    return nullptr;
}

QList<BorrowRecord> BorrowRecordDao::getBorrowRecordsByReaderId(int readerId, bool includeReturned)
{
    QList<BorrowRecord> records;
    QString sql = "SELECT id, reader_id, book_id, borrow_date, due_date, return_date, "
                  "status, fine FROM borrow_record WHERE reader_id = ? " +
                  buildWhereClause(includeReturned, true);

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return records;
    }

    query->addBindValue(readerId);

    if (!query->exec()) {
        qCritical() << "按读者ID查询借阅记录失败:" << query->lastError().text();
        return records;
    }

    while (query->next()) {
        records.push_back(mapToBorrowRecord(*query));
    }

    return records;
}

QList<BorrowRecord> BorrowRecordDao::getBorrowRecordsByBookId(int bookId, bool includeReturned)
{
    QList<BorrowRecord> records;
    QString sql = "SELECT id, reader_id, book_id, borrow_date, due_date, return_date, "
                  "status, fine FROM borrow_record WHERE book_id = ? " +
                  buildWhereClause(includeReturned, true);

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return records;
    }

    query->addBindValue(bookId);

    if (!query->exec()) {
        qCritical() << "按书籍ID查询借阅记录失败:" << query->lastError().text();
        return records;
    }

    while (query->next()) {
        records.push_back(mapToBorrowRecord(*query));
    }

    return records;
}

QList<BorrowRecord> BorrowRecordDao::getBorrowRecordsByStatus(BorrowRecord::Status status)
{
    QList<BorrowRecord> records;
    QString sql = "SELECT id, reader_id, book_id, borrow_date, due_date, return_date, "
                  "status, fine FROM borrow_record WHERE status = ?";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return records;
    }

    query->addBindValue(static_cast<int>(status));

    if (!query->exec()) {
        qCritical() << "按状态查询借阅记录失败:" << query->lastError().text();
        return records;
    }

    while (query->next()) {
        records.push_back(mapToBorrowRecord(*query));
    }

    return records;
}

QList<BorrowRecord> BorrowRecordDao::getAllBorrowRecords(bool includeReturned)
{
    QList<BorrowRecord> records;
    QString sql = "SELECT id, reader_id, book_id, borrow_date, due_date, return_date, "
                  "status, fine FROM borrow_record " + buildWhereClause(includeReturned, false);

    auto query = DBConnection::getInstance().executeQuery(sql);
    if (!query) {
        return records;
    }

    while (query->next()) {
        records.push_back(mapToBorrowRecord(*query));
    }

    return records;
}

QList<BorrowRecord> BorrowRecordDao::getOverdueRecords()
{
    QList<BorrowRecord> records;
    QString sql = "SELECT id, reader_id, book_id, borrow_date, due_date, return_date, "
                  "status, fine FROM borrow_record WHERE status = ? OR (status = ? AND due_date < NOW())";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return records;
    }

    query->addBindValue(static_cast<int>(BorrowRecord::Status::OVERDUE));
    query->addBindValue(static_cast<int>(BorrowRecord::Status::BORROWED));

    if (!query->exec()) {
        qCritical() << "查询逾期记录失败:" << query->lastError().text();
        return records;
    }

    while (query->next()) {
        records.push_back(mapToBorrowRecord(*query));
    }

    return records;
}

QList<BorrowRecord> BorrowRecordDao::getBorrowRecordsByDateRange(const QDateTime& startDate, const QDateTime& endDate)
{
    QList<BorrowRecord> records;
    QString sql = "SELECT id, reader_id, book_id, borrow_date, due_date, return_date, "
                  "status, fine FROM borrow_record WHERE borrow_date BETWEEN ? AND ?";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return records;
    }

    query->addBindValue(startDate);
    query->addBindValue(endDate);

    if (!query->exec()) {
        qCritical() << "按日期范围查询借阅记录失败:" << query->lastError().text();
        return records;
    }

    while (query->next()) {
        records.push_back(mapToBorrowRecord(*query));
    }

    return records;
}

bool BorrowRecordDao::borrowBook(int readerId, int bookId, int& recordId)
{
    auto& db = DBConnection::getInstance();

    if (!db.beginTransaction()) {
        return false;
    }

    // 检查书籍是否可借
    QString checkSql = "SELECT available_count FROM book WHERE id = ? AND available_count > 0 AND is_delete = 0";
    auto checkQuery = db.prepareQuery(checkSql);
    if (!checkQuery) {
        db.rollbackTransaction();
        return false;
    }

    checkQuery->addBindValue(bookId);
    if (!checkQuery->exec() || !checkQuery->next()) {
        qCritical() << "书籍不可借或无库存";
        db.rollbackTransaction();
        return false;
    }

    // 创建借阅记录
    BorrowRecord record(readerId, bookId, QDateTime::currentDateTime());
    int generatedId;
    if (!addBorrowRecord(record, generatedId)) {
        db.rollbackTransaction();
        return false;
    }

    // 更新书籍库存
    QString updateSql = "UPDATE book SET available_count = available_count - 1 "
                        "WHERE id = ? AND available_count > 0 AND is_delete = 0";
    auto updateQuery = db.prepareQuery(updateSql);
    if (!updateQuery) {
        db.rollbackTransaction();
        return false;
    }

    updateQuery->addBindValue(bookId);
    if (!updateQuery->exec() || updateQuery->numRowsAffected() <= 0) {
        qCritical() << "更新书籍库存失败";
        db.rollbackTransaction();
        return false;
    }

    if (!db.commitTransaction()) {
        return false;
    }

    recordId = generatedId;
    return true;
}

bool BorrowRecordDao::returnBook(int recordId)
{
    auto& db = DBConnection::getInstance();

    if (!db.beginTransaction()) {
        return false;
    }

    // 获取借阅记录
    auto record = getBorrowRecordById(recordId);
    if (!record) {
        qCritical() << "借阅记录不存在";
        db.rollbackTransaction();
        return false;
    }

    if (record->getStatus() == BorrowRecord::Status::RETURNED) {
        qCritical() << "该书籍已归还";
        db.rollbackTransaction();
        return false;
    }

    // 更新借阅记录
    QDateTime now = QDateTime::currentDateTime();
    QString updateRecordSql = "UPDATE borrow_record SET return_date = ?, status = ?, fine = ? "
                              "WHERE id = ?";

    auto updateRecordQuery = db.prepareQuery(updateRecordSql);
    if (!updateRecordQuery) {
        db.rollbackTransaction();
        return false;
    }

    double fine = record->calculateFine();
    updateRecordQuery->addBindValue(now);
    updateRecordQuery->addBindValue(static_cast<int>(BorrowRecord::Status::RETURNED));
    updateRecordQuery->addBindValue(fine);
    updateRecordQuery->addBindValue(recordId);

    if (!updateRecordQuery->exec() || updateRecordQuery->numRowsAffected() <= 0) {
        qCritical() << "更新借阅记录失败";
        db.rollbackTransaction();
        return false;
    }

    // 更新书籍库存
    QString updateBookSql = "UPDATE book SET available_count = available_count + 1 "
                            "WHERE id = ? AND available_count < total_count AND is_delete = 0";
    auto updateBookQuery = db.prepareQuery(updateBookSql);
    if (!updateBookQuery) {
        db.rollbackTransaction();
        return false;
    }

    updateBookQuery->addBindValue(record->getBookId());
    if (!updateBookQuery->exec() || updateBookQuery->numRowsAffected() <= 0) {
        qCritical() << "更新书籍库存失败";
        db.rollbackTransaction();
        return false;
    }

    return db.commitTransaction();
}

bool BorrowRecordDao::renewBook(int recordId)
{
    auto record = getBorrowRecordById(recordId);
    if (!record) {
        qCritical() << "借阅记录不存在";
        return false;
    }

    if (!record->canRenew()) {
        qCritical() << "该记录无法续借（已归还或已逾期）";
        return false;
    }

    // 续借：延长应还日期30天
    QDateTime newDueDate = record->getDueDate().addDays(30);

    QString sql = "UPDATE borrow_record SET due_date = ? WHERE id = ? AND status = ?";
    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(newDueDate);
    query->addBindValue(recordId);
    query->addBindValue(static_cast<int>(BorrowRecord::Status::BORROWED));

    if (!query->exec()) {
        qCritical() << "续借失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

bool BorrowRecordDao::batchAddBorrowRecords(const QList<BorrowRecord>& records)
{
    auto& db = DBConnection::getInstance();

    if (!db.beginTransaction()) {
        return false;
    }

    QString sql = "INSERT INTO borrow_record (reader_id, book_id, borrow_date, due_date, "
                  "return_date, status, fine) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?)";

    for (const auto& record : records) {
        auto query = db.prepareQuery(sql);
        if (!query) {
            db.rollbackTransaction();
            return false;
        }

        query->addBindValue(record.getReaderId());
        query->addBindValue(record.getBookId());
        query->addBindValue(record.getBorrowDate());
        query->addBindValue(record.getDueDate());
        query->addBindValue(record.getReturnDate());
        query->addBindValue(static_cast<int>(record.getStatus()));
        query->addBindValue(record.getFine());

        if (!query->exec()) {
            qCritical() << "批量添加借阅记录失败:" << query->lastError().text();
            db.rollbackTransaction();
            return false;
        }
    }

    return db.commitTransaction();
}

bool BorrowRecordDao::batchDeleteBorrowRecords(const QList<int>& ids)
{
    if (ids.empty()) {
        return true;
    }

    auto& db = DBConnection::getInstance();

    if (!db.beginTransaction()) {
        return false;
    }

    QString sql = "DELETE FROM borrow_record WHERE id = ?";

    for (int id : ids) {
        auto query = db.prepareQuery(sql);
        if (!query) {
            db.rollbackTransaction();
            return false;
        }

        query->addBindValue(id);

        if (!query->exec()) {
            qCritical() << "批量删除借阅记录失败:" << query->lastError().text();
            db.rollbackTransaction();
            return false;
        }
    }

    return db.commitTransaction();
}

int BorrowRecordDao::getBorrowRecordTotalCount(bool includeReturned)
{
    QString sql = "SELECT COUNT(*) FROM borrow_record " + buildWhereClause(includeReturned, false);

    auto query = DBConnection::getInstance().executeQuery(sql);
    if (!query) {
        return 0;
    }

    if (query->next()) {
        return query->value(0).toInt();
    }

    return 0;
}

int BorrowRecordDao::getBorrowRecordCountByReader(int readerId, bool includeReturned)
{
    QString sql = "SELECT COUNT(*) FROM borrow_record WHERE reader_id = ? " +
                  buildWhereClause(includeReturned, true);

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return 0;
    }

    query->addBindValue(readerId);

    if (!query->exec()) {
        qCritical() << "统计读者借阅记录失败:" << query->lastError().text();
        return 0;
    }

    if (query->next()) {
        return query->value(0).toInt();
    }

    return 0;
}

int BorrowRecordDao::getBorrowRecordCountByBook(int bookId, bool includeReturned)
{
    QString sql = "SELECT COUNT(*) FROM borrow_record WHERE book_id = ? " +
                  buildWhereClause(includeReturned, true);

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return 0;
    }

    query->addBindValue(bookId);

    if (!query->exec()) {
        qCritical() << "统计书籍借阅记录失败:" << query->lastError().text();
        return 0;
    }

    if (query->next()) {
        return query->value(0).toInt();
    }

    return 0;
}

int BorrowRecordDao::getOverdueCount()
{
    QString sql = "SELECT COUNT(*) FROM borrow_record WHERE status = ? OR (status = ? AND due_date < NOW())";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return 0;
    }

    query->addBindValue(static_cast<int>(BorrowRecord::Status::OVERDUE));
    query->addBindValue(static_cast<int>(BorrowRecord::Status::BORROWED));

    if (!query->exec()) {
        qCritical() << "统计逾期记录失败:" << query->lastError().text();
        return 0;
    }

    if (query->next()) {
        return query->value(0).toInt();
    }

    return 0;
}

double BorrowRecordDao::getTotalFineByReader(int readerId)
{
    QString sql = "SELECT SUM(fine) FROM borrow_record WHERE reader_id = ? AND status = ?";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return 0.0;
    }

    query->addBindValue(readerId);
    query->addBindValue(static_cast<int>(BorrowRecord::Status::RETURNED));

    if (!query->exec()) {
        qCritical() << "统计读者罚款失败:" << query->lastError().text();
        return 0.0;
    }

    if (query->next()) {
        return query->value(0).toDouble();
    }

    return 0.0;
}

BorrowRecord BorrowRecordDao::mapToBorrowRecord(const QSqlQuery& query)
{
    BorrowRecord record;

    record.setId(query.value("id").toInt());
    record.setReaderId(query.value("reader_id").toInt());
    record.setBookId(query.value("book_id").toInt());
    record.setBorrowDate(query.value("borrow_date").toDateTime());
    record.setDueDate(query.value("due_date").toDateTime());
    record.setReturnDate(query.value("return_date").toDateTime());
    record.setStatus(static_cast<BorrowRecord::Status>(query.value("status").toInt()));
    record.setFine(query.value("fine").toDouble());

    return record;
}

QString BorrowRecordDao::buildWhereClause(bool includeReturned, bool hasWhere) const
{
    if (includeReturned) {
        return "";
    }
    // 对于借阅记录，"未归还" 意味着状态为 BORROWED 或 OVERDUE
    QString condition = "status IN (0, 2)";  // 0=BORROWED, 2=OVERDUE
    return hasWhere ? " AND " + condition : " WHERE " + condition;
}

QString BorrowRecordDao::getStatusWhereClause(BorrowRecord::Status status, bool hasWhere) const
{
    QString condition = "status = " + QString::number(static_cast<int>(status));
    return hasWhere ? " AND " + condition : " WHERE " + condition;
}