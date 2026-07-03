#ifndef READERDAO_H
#define READERDAO_H

#include"model/reader.h"
#include"db/dbconnection.h"

class ReaderDao
{
public:
    ReaderDao();
    // 增加
    bool addReader(const Reader& reader, int& generatedId);

    // 删除
    bool deleteReader(int id);
    bool deleteReader(const QString& name);

    // 修改
    bool updateReader(const Reader& reader);

    // 查询
    std::unique_ptr<Reader> getReaderById(int id);
    std::unique_ptr<Reader> getReaderByPhone(const QString& phone);
    QList<Reader> getAllReaders(bool includeDeleted);
    QList<Reader> searchReaders(const QString& keyWord, bool includeDeleted = false);
    QList<Reader> getReadersByName(const QString& name, bool includeDeleted);

    // 批量操作
    bool batchAddReaders(const QList<Reader>& readers);
    bool batchDeleteReaders(const QList<int>& ids);

    // 恢复删除
    bool restoreReader(int id);
    bool restoreReader(const QString& phone);

    // 统计
    int getReaderTotalCount(bool includeDeleted = false);
    int getActiveReaderCount();

private:
    // 辅助方法
    Reader mapToReader(const QSqlQuery& query);
    QString buildWhereClause(bool includeDeleted, bool hasWhere) const;
};

#endif // READERDAO_H
