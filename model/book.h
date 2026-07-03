#ifndef BOOK_H
#define BOOK_H

#include<QString>
#include<QDateTime>

class Book
{
public:
    Book();
    Book(const QString& author , const QString& title ,
         const QString& isbn , const QString& category ,
         int totalCount , int availableCount);

    //===getter方法===
    int getId() const;
    QString getAuthor() const;
    QString getTitle() const;
    QString getIsbn() const;
    QString getCategory() const;
    int getTotalCount() const;
    int getAvailableCount() const;
    bool getIsDelete() const;
    QDateTime getDeleteAt() const;

    //===setter方法===
    void setAuthor(const QString& author );
    void setTitle(const QString& title );
    void setIsbn(const QString& isbn );
    void setCategory(const QString& category );
    void setTotalCount(int totalCount);
    void setAvailableCount(int availableCount);

    //===业务方法===
    bool borrow(); // 能否借书
    bool returnBook();// 还书
    bool isAvailable() const;//是否可借书
    QString getStatusText() const;//库存描述

    //===软删除===
    bool softDelete();//软删除
    void restore();//删除恢复

private:
    int id;//书籍编号
    QString author;//书籍作者
    QString title;//书籍标题（书名）
    QString isbn;//国际标准书号
    QString category;//书籍类型
    int totalCount;//书籍总数
    int availableCount;//在库书籍数

    bool isDelete;//软删除
    QDateTime deleteAt;//删除时间

    void setID(int id) ;//私有化id的setter方法
    void setIsDelete(bool isdelete) ;
    void setDeleteAt(QDateTime deleteAt) ;
    friend class BookDao;//友元
};

#endif // BOOK_H
