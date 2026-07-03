#include "bookservice.h"

// ==================== BookServiceException 实现 ====================

BookServiceException::BookServiceException(const QString& message)
    : m_message(message)
{
}

const char* BookServiceException::what() const noexcept
{
    return m_message.toUtf8().constData();
}

QString BookServiceException::getMessage() const
{
    return m_message;
}

// ==================== BorrowResult 实现 ====================

BookService::BorrowResult BookService::BorrowResult::makeSuccess(int recordId)
{
    BorrowResult result;
    result.IsSuccess = true;
    result.recordId = recordId;
    return result;
}

BookService::BorrowResult BookService::BorrowResult::makeFailure(const QString& error)
{
    BorrowResult result;
    result.IsSuccess = false;
    result.errorMessage = error;
    return result;
}

// ==================== BookService 实现 ====================

BookService::BookService()
    : m_bookDao(std::make_unique<BookDao>())
    , m_borrowRecordDao(std::make_unique<BorrowRecordDao>())
{
}

BookService::~BookService() = default;

// ==================== 图书基础操作 ====================

bool BookService::addBook(const Book& book, int& generatedId)
{
    qDebug() << "BookService::addBook - 添加图书:" << book.getTitle();

    // 1. 参数校验 - 基本校验由Book类的setter完成
    try {
        // 触发校验（复制一份进行校验）
        Book temp = book;
    } catch (const QString& e) {
        qCritical() << "添加图书校验失败:" << e;
        throw BookServiceException("图书数据校验失败: " + e);
    }

    // 2. 业务规则校验 - ISBN唯一性
    if (!isIsbnUnique(book.getIsbn())) {
        QString error = "ISBN已存在: " + book.getIsbn();
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 3. 调用DAO层添加
    if (!m_bookDao->addBook(book, generatedId)) {
        qCritical() << "添加图书到数据库失败";
        return false;
    }

    qDebug() << "图书添加成功，ID:" << generatedId;
    return true;
}

bool BookService::deleteBook(int id)
{
    qDebug() << "BookService::deleteBook - 删除图书 ID:" << id;

    // 1. 检查图书是否存在
    auto book = m_bookDao->getBookById(id);
    if (!book) {
        QString error = "图书不存在，ID: " + QString::number(id);
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 2. 检查是否已被删除
    if (book->getIsDelete()) {
        QString error = "图书已被删除，ID: " + QString::number(id);
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 3. 检查是否有未归还的借阅记录
    if (hasUnreturnedRecords(id)) {
        QString error = "图书有未归还的借阅记录，无法删除，ID: " + QString::number(id);
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 4. 执行软删除
    if (!m_bookDao->deleteBook(id)) {
        qCritical() << "软删除图书失败，ID:" << id;
        return false;
    }

    qDebug() << "图书删除成功，ID:" << id;
    return true;
}

bool BookService::deleteBookByIsbn(const QString& isbn)
{
    qDebug() << "BookService::deleteBookByIsbn - 删除图书 ISBN:" << isbn;

    // 1. 检查图书是否存在
    auto book = m_bookDao->getBookByIsbn(isbn);
    if (!book) {
        QString error = "图书不存在，ISBN: " + isbn;
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 2. 检查是否已被删除
    if (book->getIsDelete()) {
        QString error = "图书已被删除，ISBN: " + isbn;
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 3. 检查是否有未归还的借阅记录
    if (hasUnreturnedRecords(book->getId())) {
        QString error = "图书有未归还的借阅记录，无法删除，ISBN: " + isbn;
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 4. 执行软删除
    if (!m_bookDao->deleteBook(isbn)) {
        qCritical() << "软删除图书失败，ISBN:" << isbn;
        return false;
    }

    qDebug() << "图书删除成功，ISBN:" << isbn;
    return true;
}

bool BookService::updateBook(const Book& book)
{
    qDebug() << "BookService::updateBook - 更新图书 ID:" << book.getId();

    // 1. 参数校验
    if (book.getId() <= 0) {
        QString error = "图书ID无效";
        qCritical() << error;
        throw BookServiceException(error);
    }

    try {
        // 触发校验
        Book temp = book;
    } catch (const QString& e) {
        qCritical() << "更新图书校验失败:" << e;
        throw BookServiceException("图书数据校验失败: " + e);
    }

    // 2. 检查图书是否存在
    auto existingBook = m_bookDao->getBookById(book.getId());
    if (!existingBook) {
        QString error = "图书不存在，ID: " + QString::number(book.getId());
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 3. 检查ISBN是否被更改（不允许更改ISBN）
    if (existingBook->getIsbn() != book.getIsbn()) {
        QString error = "ISBN不可更改，原ISBN: " + existingBook->getIsbn() +
                        ", 新ISBN: " + book.getIsbn();
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 4. 检查总数量和可借数量是否合理
    if (book.getAvailableCount() > book.getTotalCount()) {
        QString error = "可借数量不能大于总数量";
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 5. 执行更新
    if (!m_bookDao->updateBook(book)) {
        qCritical() << "更新图书失败，ID:" << book.getId();
        return false;
    }

    qDebug() << "图书更新成功，ID:" << book.getId();
    return true;
}

bool BookService::restoreBook(int id)
{
    qDebug() << "BookService::restoreBook - 恢复图书 ID:" << id;
    return m_bookDao->restoreBook(id);
}

bool BookService::restoreBookByIsbn(const QString& isbn)
{
    qDebug() << "BookService::restoreBookByIsbn - 恢复图书 ISBN:" << isbn;
    return m_bookDao->restoreBook(isbn);
}

// ==================== 查询操作 ====================

std::unique_ptr<Book> BookService::getBookById(int id)
{
    qDebug() << "BookService::getBookById - ID:" << id;
    return m_bookDao->getBookById(id);
}

std::unique_ptr<Book> BookService::getBookByIsbn(const QString& isbn)
{
    qDebug() << "BookService::getBookByIsbn - ISBN:" << isbn;
    return m_bookDao->getBookByIsbn(isbn);
}

QList<Book> BookService::getAllBooks(bool includeDeleted)
{
    qDebug() << "BookService::getAllBooks - includeDeleted:" << includeDeleted;
    return m_bookDao->getAllBooks(includeDeleted);
}

QList<Book> BookService::searchBooks(const QString& keyword, bool includeDeleted)
{
    qDebug() << "BookService::searchBooks - keyword:" << keyword << ", includeDeleted:" << includeDeleted;

    if (keyword.trimmed().isEmpty()) {
        return QList<Book>();
    }

    return m_bookDao->searchBooks(keyword, includeDeleted);
}

QList<Book> BookService::getBooksByCategory(const QString& category, bool includeDeleted)
{
    qDebug() << "BookService::getBooksByCategory - category:" << category << ", includeDeleted:" << includeDeleted;

    if (category.trimmed().isEmpty()) {
        return QList<Book>();
    }

    return m_bookDao->getBooksByCategory(category, includeDeleted);
}

BookService::BookDetailInfo BookService::getBookDetailInfo(int bookId, int currentReaderId)
{
    qDebug() << "BookService::getBookDetailInfo - bookId:" << bookId;

    BookDetailInfo info;

    // 1. 获取图书基本信息
    auto book = m_bookDao->getBookById(bookId);
    if (!book) {
        qWarning() << "图书不存在，ID:" << bookId;
        return info;
    }
    info.book = *book;

    // 2. 获取当前借阅数量
    info.currentBorrowCount = getCurrentBorrowCount(bookId);

    // 3. 判断是否被当前读者借阅
    if (currentReaderId > 0) {
        info.isBorrowedByCurrentUser = isBookBorrowedByReader(bookId, currentReaderId);
    }

    // 4. 生成状态描述
    if (book->getIsDelete()) {
        info.statusDescription = "已删除";
    } else if (book->getAvailableCount() <= 0) {
        info.statusDescription = "无库存";
    } else if (book->getAvailableCount() <= book->getTotalCount() / 3) {
        info.statusDescription = QString("库存紧张（剩余%1本）").arg(book->getAvailableCount());
    } else {
        info.statusDescription = "可借";
    }

    return info;
}

QList<BorrowRecord> BookService::getBookBorrowHistory(int bookId, bool includeReturned)
{
    qDebug() << "BookService::getBookBorrowHistory - bookId:" << bookId;
    return m_borrowRecordDao->getBorrowRecordsByBookId(bookId, includeReturned);
}

QList<BorrowRecord> BookService::getCurrentBorrowRecordsByBook(int bookId)
{
    qDebug() << "BookService::getCurrentBorrowRecordsByBook - bookId:" << bookId;
    return m_borrowRecordDao->getBorrowRecordsByBookId(bookId, false);
}

// ==================== 库存操作 ====================

bool BookService::updateBookStock(int id, int totalCount, int availableCount)
{
    qDebug() << "BookService::updateBookStock - id:" << id
             << ", total:" << totalCount << ", available:" << availableCount;

    // 1. 参数校验
    if (totalCount < 0 || availableCount < 0) {
        QString error = "库存数量不能为负数";
        qCritical() << error;
        throw BookServiceException(error);
    }

    if (availableCount > totalCount) {
        QString error = "可借数量不能大于总数量";
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 2. 检查图书是否存在
    auto book = m_bookDao->getBookById(id);
    if (!book) {
        QString error = "图书不存在，ID: " + QString::number(id);
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 3. 如果减少总数量，需要检查是否小于当前借出数量
    int borrowedCount = book->getTotalCount() - book->getAvailableCount();
    if (totalCount < borrowedCount) {
        QString error = QString("总数量不能小于当前借出数量（已借出%1本）").arg(borrowedCount);
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 4. 执行更新
    if (!m_bookDao->updateBookCount(id, totalCount, availableCount)) {
        qCritical() << "更新库存失败，ID:" << id;
        return false;
    }

    qDebug() << "库存更新成功，ID:" << id;
    return true;
}

bool BookService::isBookAvailable(int bookId)
{
    auto book = m_bookDao->getBookById(bookId);
    if (!book || book->getIsDelete()) {
        return false;
    }
    return book->getAvailableCount() > 0;
}

QString BookService::getBookAvailabilityDescription(int bookId)
{
    auto book = m_bookDao->getBookById(bookId);
    if (!book) {
        return "图书不存在";
    }

    if (book->getIsDelete()) {
        return "已删除";
    }

    if (book->getAvailableCount() <= 0) {
        return "无库存";
    }

    if (book->getAvailableCount() <= book->getTotalCount() / 3) {
        return QString("库存紧张（剩余%1本）").arg(book->getAvailableCount());
    }

    return QString("可借（剩余%1本）").arg(book->getAvailableCount());
}

// ==================== 借还业务（核心） ====================

BookService::BorrowResult BookService::borrowBook(int bookId, int readerId)
{
    qDebug() << "BookService::borrowBook - bookId:" << bookId << ", readerId:" << readerId;

    // 1. 验证借书条件
    QString errorMessage;
    if (!validateBorrowConditions(bookId, readerId, errorMessage)) {
        qWarning() << "借书条件验证失败:" << errorMessage;
        return BorrowResult::makeFailure(errorMessage);
    }

    // 2. 执行借书操作（在DAO层的事务中完成）
    int recordId = -1;
    if (!m_borrowRecordDao->borrowBook(readerId, bookId, recordId)) {
        QString error = "借书操作失败，请检查数据库连接";
        qCritical() << error;
        return BorrowResult::makeFailure(error);
    }

    qDebug() << "借书成功，记录ID:" << recordId;
    return BorrowResult::makeSuccess(recordId);
}

bool BookService::returnBook(int recordId, double& fine)
{
    qDebug() << "BookService::returnBook - recordId:" << recordId;

    // 1. 获取借阅记录
    auto record = m_borrowRecordDao->getBorrowRecordById(recordId);
    if (!record) {
        QString error = "借阅记录不存在，ID: " + QString::number(recordId);
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 2. 检查是否已归还
    if (record->getStatus() == BorrowRecord::Status::RETURNED) {
        QString error = "该书籍已归还，记录ID: " + QString::number(recordId);
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 3. 计算罚款
    fine = record->calculateFine();

    // 4. 执行还书操作
    if (!m_borrowRecordDao->returnBook(recordId)) {
        qCritical() << "还书操作失败，记录ID:" << recordId;
        return false;
    }

    qDebug() << "还书成功，记录ID:" << recordId << ", 罚款:" << fine;
    return true;
}

bool BookService::renewBook(int recordId, QDateTime& newDueDate)
{
    qDebug() << "BookService::renewBook - recordId:" << recordId;

    // 1. 获取借阅记录
    auto record = m_borrowRecordDao->getBorrowRecordById(recordId);
    if (!record) {
        QString error = "借阅记录不存在，ID: " + QString::number(recordId);
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 2. 检查是否可以续借
    if (!record->canRenew()) {
        QString error = "该记录无法续借（已归还或已逾期）";
        qCritical() << error;
        throw BookServiceException(error);
    }

    // 3. 执行续借
    if (!m_borrowRecordDao->renewBook(recordId)) {
        qCritical() << "续借操作失败，记录ID:" << recordId;
        return false;
    }

    // 4. 获取更新后的记录
    auto updatedRecord = m_borrowRecordDao->getBorrowRecordById(recordId);
    if (updatedRecord) {
        newDueDate = updatedRecord->getDueDate();
    }

    qDebug() << "续借成功，记录ID:" << recordId << ", 新应还日期:" << newDueDate;
    return true;
}

// ==================== 统计操作 ====================

int BookService::getBookTotalCount(bool includeDeleted)
{
    return m_bookDao->getBookTotalCount(includeDeleted);
}

int BookService::getTotalAvailableCount()
{
    return m_bookDao->getBookAvailableCount();
}

int BookService::getBookCountByCategory(const QString& category, bool includeDeleted)
{
    qDebug() << "BookService::getBookCountByCategory - category:" << category;

    if (category.trimmed().isEmpty()) {
        return 0;
    }

    auto books = m_bookDao->getBooksByCategory(category, includeDeleted);
    return books.size();
}

bool BookService::batchAddBooks(const QList<Book>& books)
{
    qDebug() << "BookService::batchAddBooks - 数量:" << books.size();

    if (books.isEmpty()) {
        return true;
    }

    // 检查所有ISBN是否唯一（包括数据库中和列表中）
    QSet<QString> isbnSet;
    for (const auto& book : books) {
        const QString& isbn = book.getIsbn();
        if (isbnSet.contains(isbn)) {
            QString error = "批量添加中存在重复ISBN: " + isbn;
            qCritical() << error;
            throw BookServiceException(error);
        }
        if (!isIsbnUnique(isbn)) {
            QString error = "ISBN已存在: " + isbn;
            qCritical() << error;
            throw BookServiceException(error);
        }
        isbnSet.insert(isbn);
    }

    return m_bookDao->batchAddBooks(books);
}

bool BookService::batchDeleteBooks(const QList<int>& ids)
{
    qDebug() << "BookService::batchDeleteBooks - 数量:" << ids.size();

    if (ids.isEmpty()) {
        return true;
    }

    // 检查所有ID是否存在且可删除
    for (int id : ids) {
        auto book = m_bookDao->getBookById(id);
        if (!book) {
            QString error = "图书不存在，ID: " + QString::number(id);
            qCritical() << error;
            throw BookServiceException(error);
        }
        if (book->getIsDelete()) {
            QString error = "图书已被删除，ID: " + QString::number(id);
            qCritical() << error;
            throw BookServiceException(error);
        }
        if (hasUnreturnedRecords(id)) {
            QString error = "图书有未归还的借阅记录，无法删除，ID: " + QString::number(id);
            qCritical() << error;
            throw BookServiceException(error);
        }
    }

    return m_bookDao->batchDeleteBooks(ids);
}

// ==================== 私有辅助方法 ====================

bool BookService::isIsbnUnique(const QString& isbn, int excludeBookId)
{
    auto existing = m_bookDao->getBookByIsbn(isbn);
    if (!existing) {
        return true;
    }

    // 如果是更新操作，排除自身
    if (excludeBookId > 0 && existing->getId() == excludeBookId) {
        return true;
    }

    return false;
}

bool BookService::hasUnreturnedRecords(int bookId)
{
    auto records = m_borrowRecordDao->getBorrowRecordsByBookId(bookId, false);
    return !records.isEmpty();
}

bool BookService::isBookBorrowedByReader(int bookId, int readerId)
{
    auto records = m_borrowRecordDao->getBorrowRecordsByReaderId(readerId, false);
    for (const auto& record : records) {
        if (record.getBookId() == bookId) {
            return true;
        }
    }
    return false;
}

bool BookService::validateBorrowConditions(int bookId, int readerId, QString& errorMessage)
{
    // 1. 检查图书是否存在且可借
    auto book = m_bookDao->getBookById(bookId);
    if (!book) {
        errorMessage = "图书不存在";
        return false;
    }

    if (book->getIsDelete()) {
        errorMessage = "图书已被删除";
        return false;
    }

    if (book->getAvailableCount() <= 0) {
        errorMessage = "图书库存不足";
        return false;
    }

    // 2. 检查读者是否存在（需要ReaderDao，这里先留空，由上层调用时保证）
    // 实际使用中需要注入ReaderDao或通过其他方式检查

    // 3. 检查读者是否已借阅该书且未归还
    if (isBookBorrowedByReader(bookId, readerId)) {
        errorMessage = "您已借阅该书，请先归还";
        return false;
    }

    // 4. 检查读者借阅上限（需要ReaderService配合，这里暂不实现）
    // 实际使用中需要调用ReaderService检查

    return true;
}

int BookService::getCurrentBorrowCount(int bookId)
{
    auto records = m_borrowRecordDao->getBorrowRecordsByBookId(bookId, false);
    return records.size();
}