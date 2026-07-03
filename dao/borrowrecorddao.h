#ifndef BORROWRECORDDAO_H
#define BORROWRECORDDAO_H

#include"model/borrowrecord.h"
#include"db/dbconnection.h"

class BorrowRecordDao
{
public:
    BorrowRecordDao();

    // 增加
    bool addBorrowRecord(const BorrowRecord& record, int& generatedId);

    // 删除（物理删除，一般不建议使用，保留用于特殊场景）
    bool deleteBorrowRecord(int id);

    // 修改
    bool updateBorrowRecord(const BorrowRecord& record);

    // 查询
    std::unique_ptr<BorrowRecord> getBorrowRecordById(int id);
    QList<BorrowRecord> getBorrowRecordsByReaderId(int readerId, bool includeReturned = false);
    QList<BorrowRecord> getBorrowRecordsByBookId(int bookId, bool includeReturned = false);
    QList<BorrowRecord> getBorrowRecordsByStatus(BorrowRecord::Status status);
    QList<BorrowRecord> getAllBorrowRecords(bool includeReturned = false);
    QList<BorrowRecord> getOverdueRecords();
    QList<BorrowRecord> getBorrowRecordsByDateRange(const QDateTime& startDate, const QDateTime& endDate);

    // 借还操作
    bool borrowBook(int readerId, int bookId, int& recordId);
    bool returnBook(int recordId);
    bool renewBook(int recordId);  // 续借

    // 批量操作
    bool batchAddBorrowRecords(const QList<BorrowRecord>& records);
    bool batchDeleteBorrowRecords(const QList<int>& ids);

    // 统计
    int getBorrowRecordTotalCount(bool includeReturned = false);
    int getBorrowRecordCountByReader(int readerId, bool includeReturned = false);
    int getBorrowRecordCountByBook(int bookId, bool includeReturned = false);
    int getOverdueCount();
    double getTotalFineByReader(int readerId);

private:
    // 辅助方法
    BorrowRecord mapToBorrowRecord(const QSqlQuery& query);
    QString buildWhereClause(bool includeReturned, bool hasWhere) const;
    QString getStatusWhereClause(BorrowRecord::Status status, bool hasWhere) const;
};

#endif // BORROWRECORDDAO_H
