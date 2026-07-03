#include "borrowrecordservice.h"

// ==================== BorrowRecordServiceException 实现 ====================

BorrowRecordServiceException::BorrowRecordServiceException(const QString& message)
    : m_message(message)
{
}

const char* BorrowRecordServiceException::what() const noexcept
{
    return m_message.toUtf8().constData();
}

QString BorrowRecordServiceException::getMessage() const
{
    return m_message;
}

// ==================== BorrowRecordService 实现 ====================

BorrowRecordService::BorrowRecordService()
    : m_borrowRecordDao(std::make_unique<BorrowRecordDao>())
    , m_bookDao(std::make_unique<BookDao>())
    , m_readerDao(std::make_unique<ReaderDao>())
{
}

BorrowRecordService::~BorrowRecordService() = default;

// ==================== 基础CRUD操作 ====================

bool BorrowRecordService::addBorrowRecord(const BorrowRecord& record, int& generatedId)
{
    qDebug() << "BorrowRecordService::addBorrowRecord - 添加借阅记录";

    // 1. 校验数据
    validateRecordData(record);

    // 2. 检查读者是否存在
    auto reader = m_readerDao->getReaderById(record.getReaderId());
    if (!reader) {
        qCritical() << "读者不存在，ID:" << record.getReaderId();
        return false;
    }

    // 3. 检查图书是否存在
    auto book = m_bookDao->getBookById(record.getBookId());
    if (!book) {
        qCritical() << "图书不存在，ID:" << record.getBookId();
        return false;
    }

    // 4. 调用DAO层添加
    if (!m_borrowRecordDao->addBorrowRecord(record, generatedId)) {
        qCritical() << "添加借阅记录失败";
        return false;
    }

    qDebug() << "借阅记录添加成功，ID:" << generatedId;
    return true;
}

bool BorrowRecordService::deleteBorrowRecord(int id)
{
    qDebug() << "BorrowRecordService::deleteBorrowRecord - 删除记录 ID:" << id;

    // 1. 检查记录是否存在
    auto record = m_borrowRecordDao->getBorrowRecordById(id);
    if (!record) {
        QString error = "借阅记录不存在，ID: " + QString::number(id);
        qCritical() << error;
        throw BorrowRecordServiceException(error);
    }

    // 2. 不允许删除已归还的记录（保留历史数据）
    if (record->getStatus() == BorrowRecord::Status::RETURNED) {
        QString error = "已归还的记录不可删除，ID: " + QString::number(id);
        qCritical() << error;
        throw BorrowRecordServiceException(error);
    }

    // 3. 执行删除
    if (!m_borrowRecordDao->deleteBorrowRecord(id)) {
        qCritical() << "删除借阅记录失败，ID:" << id;
        return false;
    }

    qDebug() << "借阅记录删除成功，ID:" << id;
    return true;
}

bool BorrowRecordService::updateBorrowRecord(const BorrowRecord& record)
{
    qDebug() << "BorrowRecordService::updateBorrowRecord - 更新记录 ID:" << record.getId();

    // 1. 参数校验
    if (record.getId() <= 0) {
        QString error = "借阅记录ID无效";
        qCritical() << error;
        throw BorrowRecordServiceException(error);
    }

    // 2. 校验数据
    validateRecordData(record);

    // 3. 检查记录是否存在
    auto existing = m_borrowRecordDao->getBorrowRecordById(record.getId());
    if (!existing) {
        QString error = "借阅记录不存在，ID: " + QString::number(record.getId());
        qCritical() << error;
        throw BorrowRecordServiceException(error);
    }

    // 4. 检查状态变更是否合法
    if (existing->getStatus() == BorrowRecord::Status::RETURNED &&
        record.getStatus() != BorrowRecord::Status::RETURNED) {
        QString error = "已归还的记录不可修改为未归还状态";
        qCritical() << error;
        throw BorrowRecordServiceException(error);
    }

    // 5. 执行更新
    if (!m_borrowRecordDao->updateBorrowRecord(record)) {
        qCritical() << "更新借阅记录失败，ID:" << record.getId();
        return false;
    }

    qDebug() << "借阅记录更新成功，ID:" << record.getId();
    return true;
}

std::unique_ptr<BorrowRecordService::BorrowRecordDetail>
BorrowRecordService::getBorrowRecordDetailById(int id)
{
    qDebug() << "BorrowRecordService::getBorrowRecordDetailById - ID:" << id;

    auto record = m_borrowRecordDao->getBorrowRecordById(id);
    if (!record) {
        return nullptr;
    }

    auto detail = std::make_unique<BorrowRecordDetail>();
    detail->record = *record;
    detail->remainingStock = -1;  // 未知

    // 填充读者信息
    auto reader = m_readerDao->getReaderById(record->getReaderId());
    if (reader) {
        detail->readerName = reader->getName();
    }

    // 填充图书信息
    auto book = m_bookDao->getBookById(record->getBookId());
    if (book) {
        detail->bookTitle = book->getTitle();
        detail->bookAuthor = book->getAuthor();
        detail->bookIsbn = book->getIsbn();
        detail->remainingStock = book->getAvailableCount();
    }

    return detail;
}

// ==================== 查询操作 ====================

std::unique_ptr<BorrowRecord> BorrowRecordService::getBorrowRecordById(int id)
{
    qDebug() << "BorrowRecordService::getBorrowRecordById - ID:" << id;
    return m_borrowRecordDao->getBorrowRecordById(id);
}

QList<BorrowRecord> BorrowRecordService::getRecordsByReaderId(int readerId, bool includeReturned)
{
    qDebug() << "BorrowRecordService::getRecordsByReaderId - readerId:" << readerId;
    return m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, includeReturned);
}

QList<BorrowRecordService::BorrowRecordDetail>
BorrowRecordService::getRecordsDetailByReaderId(int readerId, bool includeReturned)
{
    qDebug() << "BorrowRecordService::getRecordsDetailByReaderId - readerId:" << readerId;

    auto records = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, includeReturned);
    return enrichRecordsWithDetail(records);
}

QList<BorrowRecord> BorrowRecordService::getRecordsByBookId(int bookId, bool includeReturned)
{
    qDebug() << "BorrowRecordService::getRecordsByBookId - bookId:" << bookId;
    return m_borrowRecordDao->getBorrowRecordsByBookId(bookId, includeReturned);
}

QList<BorrowRecordService::BorrowRecordDetail>
BorrowRecordService::getRecordsDetailByBookId(int bookId, bool includeReturned)
{
    qDebug() << "BorrowRecordService::getRecordsDetailByBookId - bookId:" << bookId;

    auto records = m_borrowRecordDao->getBorrowRecordsByBookId(bookId, includeReturned);
    return enrichRecordsWithDetail(records);
}

QList<BorrowRecord> BorrowRecordService::getRecordsByStatus(BorrowRecord::Status status)
{
    qDebug() << "BorrowRecordService::getRecordsByStatus - status:" << static_cast<int>(status);
    return m_borrowRecordDao->getBorrowRecordsByStatus(status);
}

QList<BorrowRecord> BorrowRecordService::getAllRecords(bool includeReturned)
{
    qDebug() << "BorrowRecordService::getAllRecords - includeReturned:" << includeReturned;
    return m_borrowRecordDao->getAllBorrowRecords(includeReturned);
}

QList<BorrowRecord> BorrowRecordService::getOverdueRecords()
{
    qDebug() << "BorrowRecordService::getOverdueRecords";
    return m_borrowRecordDao->getOverdueRecords();
}

QList<BorrowRecordService::BorrowRecordDetail>
BorrowRecordService::getOverdueRecordsDetail()
{
    qDebug() << "BorrowRecordService::getOverdueRecordsDetail";

    auto records = m_borrowRecordDao->getOverdueRecords();
    return enrichRecordsWithDetail(records);
}

QList<BorrowRecord> BorrowRecordService::getRecordsByDateRange(
    const QDateTime& startDate, const QDateTime& endDate)
{
    qDebug() << "BorrowRecordService::getRecordsByDateRange";
    return m_borrowRecordDao->getBorrowRecordsByDateRange(startDate, endDate);
}

// ==================== 核心借还业务 ====================

BorrowRecordService::BorrowResult BorrowRecordService::borrowBook(int bookId, int readerId)
{
    qDebug() << "BorrowRecordService::borrowBook - bookId:" << bookId << ", readerId:" << readerId;

    // 1. 验证借书条件
    QString errorMessage;
    if (!validateBorrowConditions(bookId, readerId, errorMessage)) {
        qWarning() << "借书条件验证失败:" << errorMessage;
        return BorrowResult::failure(errorMessage);
    }

    // 2. 执行借书操作（在DAO层的事务中完成）
    int recordId = -1;
    if (!m_borrowRecordDao->borrowBook(readerId, bookId, recordId)) {
        QString error = "借书操作失败，请检查数据库连接";
        qCritical() << error;
        return BorrowResult::failure(error);
    }

    qDebug() << "借书成功，记录ID:" << recordId;
    return BorrowResult::success(recordId);
}

BorrowRecordService::ReturnResult BorrowRecordService::returnBook(int recordId)
{
    return returnBook(recordId, -1.0);  // -1 表示自动计算
}

BorrowRecordService::ReturnResult BorrowRecordService::returnBook(int recordId, double manualFine)
{
    qDebug() << "BorrowRecordService::returnBook - recordId:" << recordId
             << ", manualFine:" << manualFine;

    // 1. 获取借阅记录
    auto record = m_borrowRecordDao->getBorrowRecordById(recordId);
    if (!record) {
        QString error = "借阅记录不存在，ID: " + QString::number(recordId);
        qCritical() << error;
        return ReturnResult::failure(error);
    }

    // 2. 检查是否已归还
    if (record->getStatus() == BorrowRecord::Status::RETURNED) {
        QString error = "该书籍已归还，记录ID: " + QString::number(recordId);
        qCritical() << error;
        return ReturnResult::failure(error);
    }

    // 3. 计算罚款
    double fine;
    if (manualFine >= 0) {
        fine = manualFine;
    } else {
        fine = record->calculateFine();
    }

    // 4. 执行还书操作
    if (!m_borrowRecordDao->returnBook(recordId)) {
        QString error = "还书操作失败，记录ID: " + QString::number(recordId);
        qCritical() << error;
        return ReturnResult::failure(error);
    }

    qDebug() << "还书成功，记录ID:" << recordId << ", 罚款:" << fine;
    return ReturnResult::success(fine);
}

BorrowRecordService::RenewResult BorrowRecordService::renewBook(int recordId)
{
    qDebug() << "BorrowRecordService::renewBook - recordId:" << recordId;

    // 1. 获取借阅记录
    auto record = m_borrowRecordDao->getBorrowRecordById(recordId);
    if (!record) {
        QString error = "借阅记录不存在，ID: " + QString::number(recordId);
        qCritical() << error;
        return RenewResult::failure(error);
    }

    // 2. 检查是否可以续借
    if (!record->canRenew()) {
        QString error = "该记录无法续借（已归还或已逾期）";
        qCritical() << error;
        return RenewResult::failure(error);
    }

    // 3. 执行续借
    if (!m_borrowRecordDao->renewBook(recordId)) {
        QString error = "续借操作失败，记录ID: " + QString::number(recordId);
        qCritical() << error;
        return RenewResult::failure(error);
    }

    // 4. 获取更新后的记录
    auto updatedRecord = m_borrowRecordDao->getBorrowRecordById(recordId);
    QDateTime newDueDate;
    if (updatedRecord) {
        newDueDate = updatedRecord->getDueDate();
    }

    qDebug() << "续借成功，记录ID:" << recordId << ", 新应还日期:" << newDueDate;
    return RenewResult::success(newDueDate);
}

bool BorrowRecordService::batchRenewBooks(int readerId, int& successCount, QList<int>& failedIds)
{
    qDebug() << "BorrowRecordService::batchRenewBooks - readerId:" << readerId;

    successCount = 0;
    failedIds.clear();

    // 1. 获取读者所有未归还的记录
    auto records = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);
    if (records.isEmpty()) {
        qDebug() << "读者没有未归还的借阅记录";
        return true;
    }

    // 2. 逐条尝试续借
    for (const auto& record : records) {
        if (record.canRenew()) {
            if (m_borrowRecordDao->renewBook(record.getId())) {
                successCount++;
            } else {
                failedIds.append(record.getId());
            }
        } else {
            failedIds.append(record.getId());
        }
    }

    qDebug() << "批量续借完成，成功:" << successCount << ", 失败:" << failedIds.size();
    return successCount > 0;
}

// ==================== 批量操作 ====================

bool BorrowRecordService::batchAddRecords(const QList<BorrowRecord>& records)
{
    qDebug() << "BorrowRecordService::batchAddRecords - 数量:" << records.size();

    if (records.isEmpty()) {
        return true;
    }

    // 1. 验证所有记录
    for (const auto& record : records) {
        try {
            validateRecordData(record);
        } catch (const BorrowRecordServiceException& e) {
            qCritical() << "批量添加借阅记录校验失败:" << e.getMessage();
            throw;
        }
    }

    // 2. 执行批量添加
    return m_borrowRecordDao->batchAddBorrowRecords(records);
}

bool BorrowRecordService::batchDeleteRecords(const QList<int>& ids)
{
    qDebug() << "BorrowRecordService::batchDeleteRecords - 数量:" << ids.size();

    if (ids.isEmpty()) {
        return true;
    }

    // 1. 验证所有记录
    for (int id : ids) {
        auto record = m_borrowRecordDao->getBorrowRecordById(id);
        if (!record) {
            QString error = "借阅记录不存在，ID: " + QString::number(id);
            qCritical() << error;
            throw BorrowRecordServiceException(error);
        }
        if (record->getStatus() == BorrowRecord::Status::RETURNED) {
            QString error = "已归还的记录不可删除，ID: " + QString::number(id);
            qCritical() << error;
            throw BorrowRecordServiceException(error);
        }
    }

    // 2. 执行批量删除
    return m_borrowRecordDao->batchDeleteBorrowRecords(ids);
}

// ==================== 统计操作 ====================

int BorrowRecordService::getTotalCount(bool includeReturned)
{
    return m_borrowRecordDao->getBorrowRecordTotalCount(includeReturned);
}

int BorrowRecordService::getCountByReader(int readerId, bool includeReturned)
{
    return m_borrowRecordDao->getBorrowRecordCountByReader(readerId, includeReturned);
}

int BorrowRecordService::getCountByBook(int bookId, bool includeReturned)
{
    return m_borrowRecordDao->getBorrowRecordCountByBook(bookId, includeReturned);
}

int BorrowRecordService::getOverdueCount()
{
    return m_borrowRecordDao->getOverdueCount();
}

double BorrowRecordService::getTotalFineByReader(int readerId)
{
    return m_borrowRecordDao->getTotalFineByReader(readerId);
}

double BorrowRecordService::getTotalFine()
{
    // 获取所有已归还的记录，累加罚款
    auto records = m_borrowRecordDao->getAllBorrowRecords(true);
    double total = 0.0;
    for (const auto& record : records) {
        if (record.getStatus() == BorrowRecord::Status::RETURNED) {
            total += record.getFine();
        }
    }
    return total;
}

double BorrowRecordService::getPendingFineTotal()
{
    auto records = m_borrowRecordDao->getOverdueRecords();
    double total = 0.0;
    QDateTime now = QDateTime::currentDateTime();

    for (const auto& record : records) {
        if (record.getStatus() != BorrowRecord::Status::RETURNED) {
            // 计算逾期天数
            int overdueDays = record.getDueDate().daysTo(now);
            if (overdueDays > 0) {
                total += overdueDays * DAILY_FINE_RATE;
            }
        }
    }
    return total;
}

BorrowRecordService::BorrowStatistics BorrowRecordService::getReaderStatistics(int readerId)
{
    qDebug() << "BorrowRecordService::getReaderStatistics - readerId:" << readerId;

    BorrowStatistics stats;
    stats.statisticsTime = QDateTime::currentDateTime();

    // 获取所有记录（包含已归还）
    auto allRecords = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, true);
    stats.totalBorrowCount = allRecords.size();

    // 获取当前借阅记录（未归还）
    auto currentRecords = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);
    stats.currentBorrowCount = currentRecords.size();
    stats.returnedCount = stats.totalBorrowCount - stats.currentBorrowCount;

    QDateTime now = QDateTime::currentDateTime();

    // 统计逾期和罚款
    for (const auto& record : allRecords) {
        // 统计逾期
        if (record.getStatus() == BorrowRecord::Status::OVERDUE ||
            (record.getStatus() == BorrowRecord::Status::BORROWED &&
             now > record.getDueDate())) {
            stats.overdueCount++;
        }

        // 统计已归还的罚款
        if (record.getStatus() == BorrowRecord::Status::RETURNED) {
            stats.totalFine += record.getFine();
        }
    }

    // 计算待缴罚款（逾期未还）
    for (const auto& record : currentRecords) {
        if (now > record.getDueDate()) {
            int overdueDays = record.getDueDate().daysTo(now);
            stats.pendingFine += overdueDays * DAILY_FINE_RATE;
        }
    }

    return stats;
}

BorrowRecordService::BorrowStatistics BorrowRecordService::getSystemStatistics()
{
    qDebug() << "BorrowRecordService::getSystemStatistics";

    BorrowStatistics stats;
    stats.statisticsTime = QDateTime::currentDateTime();

    // 获取所有记录（包含已归还）
    auto allRecords = m_borrowRecordDao->getAllBorrowRecords(true);
    stats.totalBorrowCount = allRecords.size();

    // 获取当前借阅记录（未归还）
    auto currentRecords = m_borrowRecordDao->getAllBorrowRecords(false);
    stats.currentBorrowCount = currentRecords.size();
    stats.returnedCount = stats.totalBorrowCount - stats.currentBorrowCount;

    QDateTime now = QDateTime::currentDateTime();

    // 统计逾期和罚款
    for (const auto& record : allRecords) {
        if (record.getStatus() == BorrowRecord::Status::OVERDUE ||
            (record.getStatus() == BorrowRecord::Status::BORROWED &&
             now > record.getDueDate())) {
            stats.overdueCount++;
        }

        if (record.getStatus() == BorrowRecord::Status::RETURNED) {
            stats.totalFine += record.getFine();
        }
    }

    // 计算待缴罚款（逾期未还）
    for (const auto& record : currentRecords) {
        if (now > record.getDueDate()) {
            int overdueDays = record.getDueDate().daysTo(now);
            stats.pendingFine += overdueDays * DAILY_FINE_RATE;
        }
    }

    return stats;
}

// ==================== 业务校验方法 ====================

bool BorrowRecordService::canReaderBorrow(int readerId, QString& errorMessage)
{
    // 1. 检查读者是否存在
    auto reader = m_readerDao->getReaderById(readerId);
    if (!reader) {
        errorMessage = "读者不存在";
        return false;
    }

    // 2. 检查是否已注销
    if (reader->getIsDelete()) {
        errorMessage = "读者已注销，无法借书";
        return false;
    }

    // 3. 检查是否有逾期记录
    if (hasOverdueRecords(readerId)) {
        errorMessage = "有逾期未归还的图书，请先归还并缴纳罚款";
        return false;
    }

    // 4. 检查是否达到借阅上限
    int currentCount = getCurrentBorrowCount(readerId);
    if (currentCount >= MAX_BORROW_LIMIT) {
        errorMessage = QString("已达到借阅上限（%1本），请先归还部分图书").arg(MAX_BORROW_LIMIT);
        return false;
    }

    return true;
}

bool BorrowRecordService::canBookBeBorrowed(int bookId, QString& errorMessage)
{
    // 1. 检查图书是否存在
    auto book = m_bookDao->getBookById(bookId);
    if (!book) {
        errorMessage = "图书不存在";
        return false;
    }

    // 2. 检查是否已删除
    if (book->getIsDelete()) {
        errorMessage = "图书已被删除";
        return false;
    }

    // 3. 检查库存
    if (book->getAvailableCount() <= 0) {
        errorMessage = "图书库存不足";
        return false;
    }

    return true;
}

bool BorrowRecordService::isBookBorrowedByReader(int bookId, int readerId)
{
    auto records = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);
    for (const auto& record : records) {
        if (record.getBookId() == bookId) {
            return true;
        }
    }
    return false;
}

double BorrowRecordService::calculateFine(int recordId)
{
    auto record = m_borrowRecordDao->getBorrowRecordById(recordId);
    if (!record) {
        return 0.0;
    }
    return record->calculateFine();
}

double BorrowRecordService::calculatePendingFine(int readerId)
{
    auto records = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);
    double total = 0.0;
    QDateTime now = QDateTime::currentDateTime();

    for (const auto& record : records) {
        if (now > record.getDueDate()) {
            int overdueDays = record.getDueDate().daysTo(now);
            total += overdueDays * DAILY_FINE_RATE;
        }
    }
    return total;
}

bool BorrowRecordService::hasOverdueRecords(int readerId)
{
    auto records = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);
    QDateTime now = QDateTime::currentDateTime();

    for (const auto& record : records) {
        if (record.getStatus() == BorrowRecord::Status::OVERDUE ||
            (record.getStatus() == BorrowRecord::Status::BORROWED &&
             now > record.getDueDate())) {
            return true;
        }
    }
    return false;
}

// ==================== 私有辅助方法 ====================

bool BorrowRecordService::validateBorrowConditions(int bookId, int readerId, QString& errorMessage)
{
    // 1. 检查图书是否可以借阅
    if (!canBookBeBorrowed(bookId, errorMessage)) {
        return false;
    }

    // 2. 检查读者是否可以借书
    if (!canReaderBorrow(readerId, errorMessage)) {
        return false;
    }

    // 3. 检查读者是否已借阅该书且未归还
    if (isBookBorrowedByReader(bookId, readerId)) {
        errorMessage = "您已借阅该书，请先归还";
        return false;
    }

    return true;
}

void BorrowRecordService::validateRecordData(const BorrowRecord& record)
{
    if (record.getReaderId() <= 0) {
        throw BorrowRecordServiceException("读者ID无效");
    }

    if (record.getBookId() <= 0) {
        throw BorrowRecordServiceException("图书ID无效");
    }

    if (!record.getBorrowDate().isValid()) {
        throw BorrowRecordServiceException("借阅日期无效");
    }

    if (!record.getDueDate().isValid()) {
        throw BorrowRecordServiceException("应还日期无效");
    }

    if (record.getDueDate() < record.getBorrowDate()) {
        throw BorrowRecordServiceException("应还日期不能早于借阅日期");
    }

    if (record.getFine() < 0) {
        throw BorrowRecordServiceException("罚款金额不能为负数");
    }
}

QList<BorrowRecordService::BorrowRecordDetail>
BorrowRecordService::enrichRecordsWithDetail(const QList<BorrowRecord>& records)
{
    QList<BorrowRecordDetail> details;

    // 缓存读者和图书信息，减少数据库查询
    QMap<int, QString> readerNameCache;
    QMap<int, QPair<QString, QString>> bookInfoCache;  // title, author

    for (const auto& record : records) {
        BorrowRecordDetail detail;
        detail.record = record;

        // 获取读者姓名
        if (!readerNameCache.contains(record.getReaderId())) {
            auto reader = m_readerDao->getReaderById(record.getReaderId());
            if (reader) {
                readerNameCache[record.getReaderId()] = reader->getName();
            } else {
                readerNameCache[record.getReaderId()] = "未知读者";
            }
        }
        detail.readerName = readerNameCache[record.getReaderId()];

        // 获取图书信息
        if (!bookInfoCache.contains(record.getBookId())) {
            auto book = m_bookDao->getBookById(record.getBookId());
            if (book) {
                bookInfoCache[record.getBookId()] =
                    qMakePair(book->getTitle(), book->getAuthor());
            } else {
                bookInfoCache[record.getBookId()] =
                    qMakePair("未知图书", "未知作者");
            }
        }
        detail.bookTitle = bookInfoCache[record.getBookId()].first;
        detail.bookAuthor = bookInfoCache[record.getBookId()].second;

        details.append(detail);
    }

    return details;
}

BorrowRecordService::BorrowRecordDetail
BorrowRecordService::enrichRecordWithDetail(const BorrowRecord& record)
{
    BorrowRecordDetail detail;
    detail.record = record;

    // 获取读者姓名
    auto reader = m_readerDao->getReaderById(record.getReaderId());
    detail.readerName = reader ? reader->getName() : "未知读者";

    // 获取图书信息
    auto book = m_bookDao->getBookById(record.getBookId());
    if (book) {
        detail.bookTitle = book->getTitle();
        detail.bookAuthor = book->getAuthor();
        detail.bookIsbn = book->getIsbn();
        detail.remainingStock = book->getAvailableCount();
    } else {
        detail.bookTitle = "未知图书";
        detail.bookAuthor = "未知作者";
        detail.remainingStock = -1;
    }

    return detail;
}

int BorrowRecordService::getCurrentBorrowCount(int readerId)
{
    auto records = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);
    return records.size();
}

int BorrowRecordService::getReaderOverdueCount(int readerId)
{
    auto records = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);
    int count = 0;
    QDateTime now = QDateTime::currentDateTime();

    for (const auto& record : records) {
        if (record.getStatus() == BorrowRecord::Status::OVERDUE ||
            (record.getStatus() == BorrowRecord::Status::BORROWED &&
             now > record.getDueDate())) {
            count++;
        }
    }
    return count;
}

double BorrowRecordService::calculateFineByDates(
    const QDateTime& dueDate, const QDateTime& returnDate, bool isReturned)
{
    if (!isReturned) {
        return 0.0;
    }

    if (returnDate > dueDate) {
        int overdueDays = dueDate.daysTo(returnDate);
        return overdueDays * DAILY_FINE_RATE;
    }
    return 0.0;
}
