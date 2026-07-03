#include "bookdao.h"

BookDao::BookDao() {}

bool BookDao::addBook(const Book &book , int& generatedId)
{
    QString sql = "INSERT INTO book (author, title, isbn, category, total_count, available_count, is_delete) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?)";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(book.getAuthor());
    query->addBindValue(book.getTitle());
    query->addBindValue(book.getIsbn());
    query->addBindValue(book.getCategory());
    query->addBindValue(book.getTotalCount());
    query->addBindValue(book.getAvailableCount());
    query->addBindValue(book.getIsDelete() ? 1 : 0);

    if (!query->exec()) {
        qCritical() << "添加书籍失败:" << query->lastError().text();
        return false;
    }

    generatedId = query->lastInsertId().toInt();
    return true;
}

bool BookDao::deleteBook(int id)
{
    QString sql = "UPDATE book SET is_delete = 1, delete_at = NOW() WHERE id = ? AND is_delete = 0";
    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(id);

    if (!query->exec()) {
        qCritical() << "软删除书籍失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

bool BookDao::deleteBook(const QString &isbn)
{
    QString sql = "UPDATE book SET is_delete = 1, delete_at = NOW() WHERE isbn = ? AND is_delete = 0";
    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(isbn);

    if (!query->exec()) {
        qCritical() << "软删除书籍失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

bool BookDao::updateBook(const Book &book)
{
    QString sql = "UPDATE book SET author = ?, title = ?, isbn = ?, category = ?, "
                  "total_count = ?, available_count = ?, is_delete = ? "
                  "WHERE id = ?";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(book.getAuthor());
    query->addBindValue(book.getTitle());
    query->addBindValue(book.getIsbn());
    query->addBindValue(book.getCategory());
    query->addBindValue(book.getTotalCount());
    query->addBindValue(book.getAvailableCount());
    query->addBindValue(book.getIsDelete() ? 1 : 0);
    query->addBindValue(book.getId());

    if (!query->exec()) {
        qCritical() << "更新书籍失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

std::unique_ptr<Book> BookDao::getBookById(int id)
{
    QString sql = "SELECT id, author, title, isbn, category, total_count, available_count, "
                  "is_delete, delete_at FROM book WHERE id = ?";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return nullptr;
    }

    query->addBindValue(id);

    if (!query->exec()) {
        qCritical() << "查询书籍失败:" << query->lastError().text();
        return nullptr;
    }

    if (query->next()) {
        return std::make_unique<Book>(mapToBook(*query));
    }

    return nullptr;
}

std::unique_ptr<Book> BookDao::getBookByIsbn(const QString &isbn)
{
    QString sql = "SELECT id, author, title, isbn, category, total_count, available_count, "
                  "is_delete, delete_at FROM book WHERE isbn = ?";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return nullptr;
    }

    query->addBindValue(isbn);

    if (!query->exec()) {
        qCritical() << "查询书籍失败:" << query->lastError().text();
        return nullptr;
    }

    if (query->next()) {
        return std::make_unique<Book>(mapToBook(*query));
    }

    return nullptr;
}

QList<Book> BookDao::getAllBooks(bool includeDeleted)
{
    QList<Book> books;
    QString sql = "SELECT id, author, title, isbn, category, total_count, available_count, "
                  "is_delete, delete_at FROM book " + buildWhereClause(includeDeleted , false);

    auto query = DBConnection::getInstance().executeQuery(sql);
    if (!query) {
        return books;
    }

    while (query->next()) {
        books.push_back(mapToBook(*query));
    }

    return books;
}

QList<Book> BookDao::searchBooks(const QString &keyWord, bool includeDeleted)
{
    QList<Book> books;
    QString sql = "SELECT id, author, title, isbn, category, total_count, available_count, "
                  "is_delete, delete_at FROM book WHERE (author LIKE ? OR title LIKE ? OR isbn LIKE ? OR category LIKE ?)";

    if (!includeDeleted) {
        sql += " AND is_delete = 0";
    }

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return books;
    }

    QString pattern = "%" + keyWord + "%";
    query->addBindValue(pattern);
    query->addBindValue(pattern);
    query->addBindValue(pattern);
    query->addBindValue(pattern);

    if (!query->exec()) {
        qCritical() << "搜索书籍失败:" << query->lastError().text();
        return books;
    }

    while (query->next()) {
        books.push_back(mapToBook(*query));
    }

    return books;
}

QList<Book> BookDao::getBooksByCategory(const QString &category , bool includeDeleted)
{
    QList<Book> books;
    QString sql = "SELECT id, author, title, isbn, category, total_count, available_count, "
                  "is_delete, delete_at FROM book WHERE category = ? " + buildWhereClause(includeDeleted,true);

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return books;
    }

    query->addBindValue(category);

    if (!query->exec()) {
        qCritical() << "按分类查询书籍失败:" << query->lastError().text();
        return books;
    }

    while (query->next()) {
        books.push_back(mapToBook(*query));
    }

    return books;
}

bool BookDao::batchAddBooks(const QList<Book> &books)
{
    auto& db = DBConnection::getInstance();

    if (!db.beginTransaction()) {
        return false;
    }

    QString sql = "INSERT INTO book (author, title, isbn, category, total_count, available_count, is_delete) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?)";

    for (const auto& book : books) {
        auto query = db.prepareQuery(sql);
        if (!query) {
            db.rollbackTransaction();
            return false;
        }

        query->addBindValue(book.getAuthor());
        query->addBindValue(book.getTitle());
        query->addBindValue(book.getIsbn());
        query->addBindValue(book.getCategory());
        query->addBindValue(book.getTotalCount());
        query->addBindValue(book.getAvailableCount());
        query->addBindValue(book.getIsDelete() ? 1 : 0);

        if (!query->exec()) {
            qCritical() << "批量添加书籍失败:" << query->lastError().text();
            db.rollbackTransaction();
            return false;
        }
    }

    return db.commitTransaction();
}

bool BookDao::batchDeleteBooks(const QList<int> &ids)
{
    if (ids.empty()) {
        return true;
    }

    auto& db = DBConnection::getInstance();

    if (!db.beginTransaction()) {
        return false;
    }

    QString sql = "UPDATE book SET is_delete = 1, delete_at = NOW() WHERE id = ? AND is_delete = 0";

    for (int id : ids) {
        auto query = db.prepareQuery(sql);
        if (!query) {
            db.rollbackTransaction();
            return false;
        }

        query->addBindValue(id);

        if (!query->exec()) {
            qCritical() << "批量删除书籍失败:" << query->lastError().text();
            db.rollbackTransaction();
            return false;
        }
    }

    return db.commitTransaction();
}

bool BookDao::updateBookCount(int id, int totalCount, int availableCount)
{
    QString sql = "UPDATE book SET total_count = ?, available_count = ? WHERE id = ? AND is_delete = 0";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(totalCount);
    query->addBindValue(availableCount);
    query->addBindValue(id);

    if (!query->exec()) {
        qCritical() << "更新库存失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

bool BookDao::borrowBook(int id)
{
    QString sql = "UPDATE book SET available_count = available_count - 1 "
                  "WHERE id = ? AND available_count > 0 AND is_delete = 0";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(id);

    if (!query->exec()) {
        qCritical() << "借书失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

bool BookDao::returnBook(int id)
{
    QString sql = "UPDATE book SET available_count = available_count + 1 "
                  "WHERE id = ? AND available_count < total_count AND is_delete = 0";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(id);

    if (!query->exec()) {
        qCritical() << "还书失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

bool BookDao::restoreBook(int id)
{
    QString sql = "UPDATE book SET is_delete = 0, delete_at = NULL WHERE id = ? AND is_delete = 1";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(id);

    if (!query->exec()) {
        qCritical() << "恢复书籍失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

bool BookDao::restoreBook(const QString &isbn)
{
    QString sql = "UPDATE book SET is_delete = 0, delete_at = NULL WHERE isbn = ? AND is_delete = 1";

    auto query = DBConnection::getInstance().prepareQuery(sql);
    if (!query) {
        return false;
    }

    query->addBindValue(isbn);

    if (!query->exec()) {
        qCritical() << "恢复书籍失败:" << query->lastError().text();
        return false;
    }

    return query->numRowsAffected() > 0;
}

int BookDao::getBookTotalCount(bool includeDeleted)
{
    QString sql = "SELECT COUNT(*) FROM book " + buildWhereClause(includeDeleted,false);

    auto query = DBConnection::getInstance().executeQuery(sql);
    if (!query) {
        return 0;
    }

    if (query->next()) {
        return query->value(0).toInt();
    }

    return 0;
}

int BookDao::getBookAvailableCount()
{
    QString sql = "SELECT SUM(available_count) FROM book " + buildWhereClause(false,false);

    auto query = DBConnection::getInstance().executeQuery(sql);
    if (!query) {
        return 0;
    }

    if (query->next()) {
        return query->value(0).toInt();
    }

    return 0;
}

Book BookDao::mapToBook(const QSqlQuery &query)
{
    Book book;
    book.setID(query.value("id").toInt());
    book.setAuthor(query.value("author").toString());
    book.setTitle(query.value("title").toString());
    book.setIsbn(query.value("isbn").toString());
    book.setCategory(query.value("category").toString());
    book.setTotalCount(query.value("total_count").toInt());
    book.setAvailableCount(query.value("available_count").toInt());
    book.setIsDelete(query.value("is_delete").toInt() == 1);
    book.setDeleteAt(query.value("delete_at").toDateTime());

    return book;
}

QString BookDao::buildWhereClause(bool includeDeleted , bool hasWhere = false) const
{
    if (includeDeleted) {
        return "";
    }
    return hasWhere ? " AND is_delete = 0" : " WHERE is_delete = 0";
}


