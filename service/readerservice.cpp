#include "readerservice.h"

// ==================== ReaderServiceException 实现 ====================

ReaderServiceException::ReaderServiceException(const QString& message)
    : m_message(message)
{
}

const char* ReaderServiceException::what() const noexcept
{
    return m_message.toUtf8().constData();
}

QString ReaderServiceException::getMessage() const
{
    return m_message;
}

// ==================== ReaderService 实现 ====================

ReaderService::ReaderService()
    : m_readerDao(std::make_unique<ReaderDao>())
    , m_borrowRecordDao(std::make_unique<BorrowRecordDao>())
{
}

ReaderService::~ReaderService() = default;

// ==================== 读者基础操作 ====================

bool ReaderService::addReader(const Reader& reader, int& generatedId)
{
    qDebug() << "ReaderService::addReader - 添加读者:" << reader.getName();

    // 1. 校验读者数据
    validateReaderData(reader);

    // 2. 检查电话唯一性
    if (!isPhoneUnique(reader.getPhone())) {
        QString error = "电话已存在: " + reader.getPhone();
        qCritical() << error;
        throw ReaderServiceException(error);
    }

    // 3. 调用DAO层添加
    if (!m_readerDao->addReader(reader, generatedId)) {
        qCritical() << "添加读者到数据库失败";
        return false;
    }

    qDebug() << "读者添加成功，ID:" << generatedId;
    return true;
}

bool ReaderService::deleteReader(int id)
{
    qDebug() << "ReaderService::deleteReader - 删除读者 ID:" << id;

    // 1. 检查读者是否存在
    auto reader = m_readerDao->getReaderById(id);
    if (!reader) {
        QString error = "读者不存在，ID: " + QString::number(id);
        qCritical() << error;
        throw ReaderServiceException(error);
    }

    // 2. 检查是否已被删除
    if (reader->getIsDelete()) {
        QString error = "读者已被删除，ID: " + QString::number(id);
        qCritical() << error;
        throw ReaderServiceException(error);
    }

    // 3. 检查是否有未归还的借阅记录
    if (hasUnreturnedRecords(id)) {
        QString error = "读者有未归还的借阅记录，无法删除，ID: " + QString::number(id);
        qCritical() << error;
        throw ReaderServiceException(error);
    }

    // 4. 执行软删除
    if (!m_readerDao->deleteReader(id)) {
        qCritical() << "软删除读者失败，ID:" << id;
        return false;
    }

    qDebug() << "读者删除成功，ID:" << id;
    return true;
}

bool ReaderService::deleteReaderByName(const QString& name)
{
    qDebug() << "ReaderService::deleteReaderByName - 删除读者 姓名:" << name;

    // 1. 根据姓名查找读者（取第一个匹配的）
    auto readers = m_readerDao->getReadersByName(name, false);
    if (readers.isEmpty()) {
        QString error = "读者不存在，姓名: " + name;
        qCritical() << error;
        throw ReaderServiceException(error);
    }

    const Reader& reader = readers.first();

    // 2. 检查是否有未归还的借阅记录
    if (hasUnreturnedRecords(reader.getId())) {
        QString error = "读者有未归还的借阅记录，无法删除，姓名: " + name;
        qCritical() << error;
        throw ReaderServiceException(error);
    }

    // 3. 执行软删除
    if (!m_readerDao->deleteReader(name)) {
        qCritical() << "软删除读者失败，姓名:" << name;
        return false;
    }

    qDebug() << "读者删除成功，姓名:" << name;
    return true;
}

bool ReaderService::updateReader(const Reader& reader)
{
    qDebug() << "ReaderService::updateReader - 更新读者 ID:" << reader.getId();

    // 1. 参数校验
    if (reader.getId() <= 0) {
        QString error = "读者ID无效";
        qCritical() << error;
        throw ReaderServiceException(error);
    }

    // 2. 校验读者数据
    validateReaderData(reader);

    // 3. 检查读者是否存在
    auto existingReader = m_readerDao->getReaderById(reader.getId());
    if (!existingReader) {
        QString error = "读者不存在，ID: " + QString::number(reader.getId());
        qCritical() << error;
        throw ReaderServiceException(error);
    }

    // 4. 检查电话唯一性（排除自身）
    if (!isPhoneUnique(reader.getPhone(), reader.getId())) {
        QString error = "电话已被其他读者使用: " + reader.getPhone();
        qCritical() << error;
        throw ReaderServiceException(error);
    }

    // 5. 不允许修改注册时间（保持原值）
    // 注意：Reader对象的registerAt字段由DAO层管理，Service层不修改

    // 6. 执行更新
    if (!m_readerDao->updateReader(reader)) {
        qCritical() << "更新读者失败，ID:" << reader.getId();
        return false;
    }

    qDebug() << "读者更新成功，ID:" << reader.getId();
    return true;
}

bool ReaderService::restoreReader(int id)
{
    qDebug() << "ReaderService::restoreReader - 恢复读者 ID:" << id;
    return m_readerDao->restoreReader(id);
}

bool ReaderService::restoreReaderByPhone(const QString& phone)
{
    qDebug() << "ReaderService::restoreReaderByPhone - 恢复读者 电话:" << phone;
    return m_readerDao->restoreReader(phone);
}

// ==================== 查询操作 ====================

std::unique_ptr<Reader> ReaderService::getReaderById(int id)
{
    qDebug() << "ReaderService::getReaderById - ID:" << id;
    return m_readerDao->getReaderById(id);
}

std::unique_ptr<Reader> ReaderService::getReaderByPhone(const QString& phone)
{
    qDebug() << "ReaderService::getReaderByPhone - 电话:" << phone;
    return m_readerDao->getReaderByPhone(phone);
}

QList<Reader> ReaderService::getAllReaders(bool includeDeleted)
{
    qDebug() << "ReaderService::getAllReaders - includeDeleted:" << includeDeleted;
    return m_readerDao->getAllReaders(includeDeleted);
}

QList<Reader> ReaderService::searchReaders(const QString& keyword, bool includeDeleted)
{
    qDebug() << "ReaderService::searchReaders - keyword:" << keyword << ", includeDeleted:" << includeDeleted;

    if (keyword.trimmed().isEmpty()) {
        return QList<Reader>();
    }

    return m_readerDao->searchReaders(keyword, includeDeleted);
}

QList<Reader> ReaderService::getReadersByName(const QString& name, bool includeDeleted)
{
    qDebug() << "ReaderService::getReadersByName - name:" << name << ", includeDeleted:" << includeDeleted;

    if (name.trimmed().isEmpty()) {
        return QList<Reader>();
    }

    return m_readerDao->getReadersByName(name, includeDeleted);
}

ReaderService::ReaderDetailInfo ReaderService::getReaderDetailInfo(int readerId)
{
    qDebug() << "ReaderService::getReaderDetailInfo - readerId:" << readerId;

    ReaderDetailInfo info;

    // 1. 获取读者基本信息
    auto reader = m_readerDao->getReaderById(readerId);
    if (!reader) {
        qWarning() << "读者不存在，ID:" << readerId;
        return info;
    }
    info.reader = *reader;

    // 2. 获取借阅统计
    auto stats = getReaderBorrowStatistics(readerId);
    info.currentBorrowCount = stats.currentBorrowCount;
    info.overdueCount = stats.overdueCount;
    info.totalFine = stats.totalFine;

    // 3. 获取当前借阅记录（最多5条）
    info.currentBorrowRecords = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);

    // 4. 判断是否可以借书
    QString errorMessage;
    info.canBorrow = canReaderBorrow(readerId, errorMessage);

    // 5. 生成状态描述
    if (reader->getIsDelete()) {
        info.statusDescription = "已注销";
    } else if (info.overdueCount > 0) {
        info.statusDescription = QString("有%1本图书逾期").arg(info.overdueCount);
    } else if (info.currentBorrowCount >= MAX_BORROW_LIMIT) {
        info.statusDescription = QString("已借满（%1/%2本）").arg(info.currentBorrowCount).arg(MAX_BORROW_LIMIT);
    } else if (info.currentBorrowCount > 0) {
        info.statusDescription = QString("已借%1本").arg(info.currentBorrowCount);
    } else {
        info.statusDescription = "正常";
    }

    // 6. 生成借阅限额描述
    int remaining = MAX_BORROW_LIMIT - info.currentBorrowCount;
    if (reader->getIsDelete()) {
        info.borrowLimitDescription = "已注销，不可借书";
    } else if (info.overdueCount > 0) {
        info.borrowLimitDescription = "有逾期记录，请先归还逾期图书";
    } else {
        info.borrowLimitDescription = QString("可借%1本（限额%2本）").arg(remaining).arg(MAX_BORROW_LIMIT);
    }

    return info;
}

// ==================== 借阅相关 ====================

QList<BorrowRecord> ReaderService::getReaderBorrowHistory(int readerId, bool includeReturned)
{
    qDebug() << "ReaderService::getReaderBorrowHistory - readerId:" << readerId
             << ", includeReturned:" << includeReturned;
    return m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, includeReturned);
}

QList<BorrowRecord> ReaderService::getCurrentBorrowRecords(int readerId)
{
    qDebug() << "ReaderService::getCurrentBorrowRecords - readerId:" << readerId;
    return m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);
}

ReaderService::BorrowStatistics ReaderService::getReaderBorrowStatistics(int readerId)
{
    qDebug() << "ReaderService::getReaderBorrowStatistics - readerId:" << readerId;

    BorrowStatistics stats;

    // 获取所有借阅记录（包含已归还）
    auto allRecords = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, true);

    // 获取当前借阅记录（未归还）
    auto currentRecords = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);

    stats.totalBorrowCount = allRecords.size();
    stats.currentBorrowCount = currentRecords.size();
    stats.returnedCount = stats.totalBorrowCount - stats.currentBorrowCount;

    // 统计逾期数量和总罚款
    for (const auto& record : allRecords) {
        // 统计逾期
        if (record.getStatus() == BorrowRecord::Status::OVERDUE ||
            (record.getStatus() == BorrowRecord::Status::BORROWED &&
             QDateTime::currentDateTime() > record.getDueDate())) {
            stats.overdueCount++;
        }

        // 统计罚款（只统计已归还的罚款）
        if (record.getStatus() == BorrowRecord::Status::RETURNED) {
            stats.totalFine += record.getFine();
        }
    }

    // 对于当前借阅但已逾期的，实时计算罚款（但不计入totalFine，因为还未归还）
    for (const auto& record : currentRecords) {
        if (QDateTime::currentDateTime() > record.getDueDate()) {
            // 这已经在overdueCount中统计了
        }
    }

    return stats;
}

bool ReaderService::canReaderBorrow(int readerId, QString& errorMessage)
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
    if (getOverdueCount(readerId) > 0) {
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

ReaderService::BorrowLimitStatus ReaderService::getReaderBorrowLimitStatus(int readerId)
{
    qDebug() << "ReaderService::getReaderBorrowLimitStatus - readerId:" << readerId;

    BorrowLimitStatus status;

    // 获取读者信息
    auto reader = m_readerDao->getReaderById(readerId);
    if (!reader) {
        status.reason = "读者不存在";
        return status;
    }

    status.isDeleted = reader->getIsDelete();
    status.currentBorrowCount = getCurrentBorrowCount(readerId);
    status.hasOverdue = (getOverdueCount(readerId) > 0);
    status.remainingLimit = MAX_BORROW_LIMIT - status.currentBorrowCount;

    // 综合判断是否可以借书
    if (status.isDeleted) {
        status.canBorrow = false;
        status.reason = "读者已注销";
    } else if (status.hasOverdue) {
        status.canBorrow = false;
        status.reason = "有逾期未归还的图书";
    } else if (status.remainingLimit <= 0) {
        status.canBorrow = false;
        status.reason = QString("已达到借阅上限（%1本）").arg(MAX_BORROW_LIMIT);
    } else {
        status.canBorrow = true;
        status.reason = "可以借书";
    }

    return status;
}

// ==================== 统计操作 ====================

int ReaderService::getReaderTotalCount(bool includeDeleted)
{
    return m_readerDao->getReaderTotalCount(includeDeleted);
}

int ReaderService::getActiveReaderCount()
{
    return m_readerDao->getActiveReaderCount();
}

int ReaderService::getReaderBorrowCount(int readerId, bool includeReturned)
{
    auto records = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, includeReturned);
    return records.size();
}

// ==================== 批量操作 ====================

bool ReaderService::batchAddReaders(const QList<Reader>& readers)
{
    qDebug() << "ReaderService::batchAddReaders - 数量:" << readers.size();

    if (readers.isEmpty()) {
        return true;
    }

    // 1. 验证所有读者数据
    QSet<QString> phoneSet;
    for (const auto& reader : readers) {
        try {
            validateReaderData(reader);
        } catch (const ReaderServiceException& e) {
            qCritical() << "批量添加读者校验失败:" << e.getMessage();
            throw;
        }

        // 检查列表内是否有重复电话
        const QString& phone = reader.getPhone();
        if (phoneSet.contains(phone)) {
            QString error = "批量添加中存在重复电话: " + phone;
            qCritical() << error;
            throw ReaderServiceException(error);
        }
        phoneSet.insert(phone);

        // 检查数据库是否已存在
        if (!isPhoneUnique(phone)) {
            QString error = "电话已存在: " + phone;
            qCritical() << error;
            throw ReaderServiceException(error);
        }
    }

    // 2. 执行批量添加
    return m_readerDao->batchAddReaders(readers);
}

bool ReaderService::batchDeleteReaders(const QList<int>& ids)
{
    qDebug() << "ReaderService::batchDeleteReaders - 数量:" << ids.size();

    if (ids.isEmpty()) {
        return true;
    }

    // 1. 验证所有读者
    for (int id : ids) {
        auto reader = m_readerDao->getReaderById(id);
        if (!reader) {
            QString error = "读者不存在，ID: " + QString::number(id);
            qCritical() << error;
            throw ReaderServiceException(error);
        }
        if (reader->getIsDelete()) {
            QString error = "读者已被删除，ID: " + QString::number(id);
            qCritical() << error;
            throw ReaderServiceException(error);
        }
        if (hasUnreturnedRecords(id)) {
            QString error = "读者有未归还的借阅记录，无法删除，ID: " + QString::number(id);
            qCritical() << error;
            throw ReaderServiceException(error);
        }
    }

    // 2. 执行批量删除
    return m_readerDao->batchDeleteReaders(ids);
}

// ==================== 私有辅助方法 ====================

void ReaderService::validateReaderData(const Reader& reader)
{
    // 校验姓名
    QString name = reader.getName().trimmed();
    if (name.isEmpty()) {
        throw ReaderServiceException("读者姓名不能为空");
    }
    if (name.length() > MAX_NAME_LENGTH) {
        throw ReaderServiceException(QString("姓名长度超过限制（最大%1个字符）").arg(MAX_NAME_LENGTH));
    }

    // 校验电话
    QString phone = reader.getPhone().trimmed();
    if (phone.isEmpty()) {
        throw ReaderServiceException("联系方式不能为空");
    }
    // 简单校验：只允许数字、+、空格
    for (QChar c : phone) {
        if (!c.isDigit() && c != '+' && c != ' ') {
            throw ReaderServiceException("联系方式包含无效字符");
        }
    }
    if (phone.length() < 7 || phone.length() > MAX_PHONE_LENGTH) {
        throw ReaderServiceException(QString("联系方式长度应在7~%1位之间").arg(MAX_PHONE_LENGTH));
    }

    // 校验邮箱（可选字段）
    QString email = reader.getEmail().trimmed();
    if (!email.isEmpty()) {
        if (!email.contains('@') || !email.contains('.')) {
            throw ReaderServiceException("邮箱格式不正确，应包含@和.");
        }
        if (email.length() > MAX_EMAIL_LENGTH) {
            throw ReaderServiceException(QString("邮箱长度超过限制（最大%1个字符）").arg(MAX_EMAIL_LENGTH));
        }
    }

    // 校验地址（可选字段）
    QString address = reader.getAddress().trimmed();
    if (!address.isEmpty() && address.length() > MAX_ADDRESS_LENGTH) {
        throw ReaderServiceException(QString("地址长度超过限制（最大%1个字符）").arg(MAX_ADDRESS_LENGTH));
    }
}

bool ReaderService::isPhoneUnique(const QString& phone, int excludeReaderId)
{
    auto existing = m_readerDao->getReaderByPhone(phone);
    if (!existing) {
        return true;
    }

    // 如果是更新操作，排除自身
    if (excludeReaderId > 0 && existing->getId() == excludeReaderId) {
        return true;
    }

    return false;
}

bool ReaderService::hasUnreturnedRecords(int readerId)
{
    auto records = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);
    return !records.isEmpty();
}

int ReaderService::getCurrentBorrowCount(int readerId)
{
    auto records = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);
    return records.size();
}

int ReaderService::getOverdueCount(int readerId)
{
    auto records = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);

    int overdueCount = 0;
    QDateTime now = QDateTime::currentDateTime();

    for (const auto& record : records) {
        // 状态为OVERDUE，或者已借出但超过应还日期
        if (record.getStatus() == BorrowRecord::Status::OVERDUE ||
            (record.getStatus() == BorrowRecord::Status::BORROWED &&
             record.getDueDate() < now)) {
            overdueCount++;
        }
    }

    return overdueCount;
}

double ReaderService::calculateTotalFine(int readerId)
{
    // 使用DAO层已有的方法
    return m_borrowRecordDao->getTotalFineByReader(readerId);
}

