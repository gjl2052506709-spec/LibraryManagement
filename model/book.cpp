#include "book.h"

Book::Book():id(-1) , totalCount(0) , availableCount(0) {}

Book::Book(const QString &author, const QString &title, const QString &isbn, const QString &category, int totalCount, int availableCount)
{
    setAuthor(author);
    setTitle(title);
    setIsbn(isbn);
    setCategory(category);
    setTotalCount(totalCount);
    setAvailableCount(availableCount);
    isDelete = false;
    deleteAt = QDateTime();
}

//===getter方法===
int Book::getId() const
{
    return this->id;
}
QString Book::getAuthor() const
{
    return this->author;
}
QString Book::getTitle() const
{
    return this->title;
}
QString Book::getIsbn() const
{
    return this->isbn ;
}
QString Book::getCategory() const
{
    return this->category;
}
int Book::getTotalCount() const
{
    return this->totalCount;
}
int Book::getAvailableCount() const
{
    return this->availableCount;
}
bool Book::getIsDelete() const
{
    return this->isDelete;
}
QDateTime Book::getDeleteAt() const
{
    return this->deleteAt;
}


//===setter方法===
void Book::setID(int id)
{
    this->id = id ;
}

void Book::setIsDelete(bool isdelete)
{
    this->isDelete = isdelete;
}

void Book::setDeleteAt(QDateTime deleteAt)
{
    this->deleteAt = deleteAt ;
}
void Book::setAuthor(const QString &author)
{
    if(author.trimmed().isEmpty()){
        throw QString("作者不能为空");
    }
    this->author = author.trimmed();
}
void Book::setTitle(const QString &title)
{
    if(title.trimmed().isEmpty()){
        throw QString("书名名字不能为空");
    }
    if(title.trimmed().length() > 100 ){
        throw QString("书名长度超过限制，限制为100长");
    }
    this->title = title.trimmed();
}
void Book::setIsbn(const QString &isbn)
{
    QString cleaned = isbn.trimmed();
    QString pre = cleaned;
    pre.remove('-');
    if(pre.length() != 13){
        throw QString("国际标准书号应为13位");
    }
    for(QChar c : pre){
        if(!c.isDigit()){
            throw QString("ISBN只能包含数字（0~9）和连字符（-）");
        }
    }
    this->isbn = cleaned;
}
void Book::setCategory(const QString &category)
{
    if(category.trimmed().isEmpty()){
        throw QString("分类不能为空");
    }
    this->category = category;
}
void Book::setTotalCount(int totalCount)
{
    if(totalCount < 0 ){
        throw QString("总数不能是负数");
    }
    this->totalCount = totalCount;

}
void Book::setAvailableCount(int availableCount)
{
    if(availableCount < 0 ){
        throw QString("可借阅数量不能为负数");
    }
    if(availableCount > totalCount){
        throw QString("可借阅数量不能超过总数");
    }
    this->availableCount = availableCount;
}


//===业务方法===
bool Book::borrow()
{
    if(availableCount <= 0 ){
        return false;
    }
    availableCount -- ;
    return true ;
}

bool Book::returnBook()
{
    if(availableCount >= totalCount){
        return false;
    }
    availableCount ++ ;
    return true ;
}

bool Book::isAvailable() const
{
    return availableCount>0;
}

QString Book::getStatusText() const
{
    if(availableCount == 0 ){
        return "无库存";
    }else if(availableCount <= totalCount/3 ) {
        QString str = QStringLiteral("库存紧张，剩余 %1 本书籍").arg(availableCount);
        return  str ;
    }else{
        return "可借";
    }
}


//===软删除===
bool Book::softDelete()
{
    if(totalCount - availableCount > 0){
        return false;
    }
    isDelete = true;
    deleteAt = QDateTime::currentDateTime();
    return true ;
}

void Book::restore()
{
    isDelete = false;
    deleteAt = QDateTime();
}



