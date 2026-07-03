#include "borrowrecord.h"

BorrowRecord::BorrowRecord()
    : id(-1)
    , readerId(-1)
    , bookId(-1)
    , status(Status::BORROWED)
    , fine(0.0)
{
    borrowDate = QDateTime::currentDateTime();
    dueDate = borrowDate.addDays(DEFAULT_BORROW_DAYS);
}

BorrowRecord::BorrowRecord(int readerId, int bookId, const QDateTime& borrowDate)
    : id(-1)
    , readerId(readerId)
    , bookId(bookId)
    , borrowDate(borrowDate)
    , status(Status::BORROWED)
    , fine(0.0)
{
    this->dueDate = borrowDate.addDays(DEFAULT_BORROW_DAYS);
}

BorrowRecord::BorrowRecord(int id, int readerId, int bookId,
                           const QDateTime& borrowDate, const QDateTime& dueDate,
                           const QDateTime& returnDate, int status, double fine)
    : id(id)
    , readerId(readerId)
    , bookId(bookId)
    , borrowDate(borrowDate)
    , dueDate(dueDate)
    , returnDate(returnDate)
    , status(static_cast<Status>(status))
    , fine(fine)
{
}

// === Getter 方法 ===
int BorrowRecord::getId() const { return id; }
int BorrowRecord::getReaderId() const { return readerId; }
int BorrowRecord::getBookId() const { return bookId; }
QDateTime BorrowRecord::getBorrowDate() const { return borrowDate; }
QDateTime BorrowRecord::getDueDate() const { return dueDate; }
QDateTime BorrowRecord::getReturnDate() const { return returnDate; }
BorrowRecord::Status BorrowRecord::getStatus() const { return status; }
double BorrowRecord::getFine() const { return fine; }
QString BorrowRecord::getReaderName() const { return readerName; }
QString BorrowRecord::getBookTitle() const { return bookTitle; }

QString BorrowRecord::getStatusText() const
{
    switch(status) {
    case Status::BORROWED:
        return "已借出";
    case Status::RETURNED:
        return "已归还";
    case Status::OVERDUE:
        return "已逾期";
    default:
        return "未知状态";
    }
}

// === Setter 方法 ===
void BorrowRecord::setId(int id) { this->id = id; }
void BorrowRecord::setReaderId(int readerId) { this->readerId = readerId; }
void BorrowRecord::setBookId(int bookId) { this->bookId = bookId; }
void BorrowRecord::setBorrowDate(const QDateTime& borrowDate) { this->borrowDate = borrowDate; }
void BorrowRecord::setDueDate(const QDateTime& dueDate) { this->dueDate = dueDate; }
void BorrowRecord::setReturnDate(const QDateTime& returnDate) { this->returnDate = returnDate; }
void BorrowRecord::setStatus(Status status) { this->status = status; }
void BorrowRecord::setFine(double fine) { this->fine = fine; }
void BorrowRecord::setReaderName(const QString& name) { readerName = name; }
void BorrowRecord::setBookTitle(const QString& title) { bookTitle = title; }

// === 业务方法 ===
bool BorrowRecord::isOverdue() const
{
    if (status == Status::RETURNED) {
        return returnDate > dueDate;
    }
    return QDateTime::currentDateTime() > dueDate;
}

int BorrowRecord::getBorrowDays() const
{
    QDateTime endDate = (status == Status::RETURNED) ? returnDate : QDateTime::currentDateTime();
    return borrowDate.daysTo(endDate);
}

double BorrowRecord::calculateFine() const
{
    if (status == Status::RETURNED && returnDate > dueDate) {
        int overdueDays = dueDate.daysTo(returnDate);
        return overdueDays * DAILY_FINE_RATE;
    }
    return 0.0;
}

bool BorrowRecord::canRenew() const
{
    // 只有已借出且未逾期的记录才能续借
    return status == Status::BORROWED && !isOverdue();
}

bool BorrowRecord::isValid() const
{
    return readerId > 0 && bookId > 0 && borrowDate.isValid();
}
