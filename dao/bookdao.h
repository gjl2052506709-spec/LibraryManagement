#ifndef BOOKDAO_H
#define BOOKDAO_H

#include"model/book.h"
#include"db/dbconnection.h"

class BookDao
{
public:
    BookDao();

    //增加
    bool addBook(const Book& book , int& generatedId);

    //删除
    bool deleteBook(int id);
    bool deleteBook(const QString& isbn);

    //修改
    bool updateBook(const Book& book);

    //查询
    std::unique_ptr<Book> getBookById(int id);
    std::unique_ptr<Book> getBookByIsbn(const QString& isbn);
    QList<Book> getAllBooks(bool includeDeleted);
    QList<Book> searchBooks(const QString& keyWord , bool includeDeleted = false );
    QList<Book> getBooksByCategory(const QString& category , bool includeDeleted );

    //批量操作
    bool batchAddBooks(const QList<Book>& books );
    bool batchDeleteBooks(const QList<int>& ids );

    //库存相关
    bool updateBookCount(int id , int totalCount , int availableCount );
    bool borrowBook(int id ) ;
    bool returnBook(int id ) ;

    //恢复删除
    bool restoreBook(int id );
    bool restoreBook(const QString& isbn ) ;

    //统计
    int getBookTotalCount(bool includeDeleted = false) ;
    int getBookAvailableCount() ;

private:
    //辅助方法
    Book mapToBook(const QSqlQuery& query);//将结果变成book实例
    QString buildWhereClause(bool includeDeleted , bool hasWhere) const ;//关于是否包含删除书籍
    bool executeBookOperation(const QString& sql , const Book& book ) ;//关于数据库语句是否成功


};

#endif // BOOKDAO_H
