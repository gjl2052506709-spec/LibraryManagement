#ifndef BOOKSERVICE_H
#define BOOKSERVICE_H

#include"dao/bookdao.h"
#include"dao/borrowrecorddao.h"

/**
 * @brief 图书业务异常类
 */
class BookServiceException : public std::exception
{
public:
    explicit BookServiceException(const QString& message);
    const char* what() const noexcept override;
    QString getMessage() const;

private:
    QString m_message;
};

/**
 * @brief 图书业务服务类
 *
 * 提供图书相关的业务逻辑处理，包括：
 * - 图书的增删改查（含业务规则校验）
 * - 借书、还书、续借业务
 * - 库存管理
 * - 图书状态查询
 */
class BookService
{
public:
    /**
     * @brief 借阅结果结构体
     */
    struct BorrowResult {
        bool IsSuccess;          // 是否成功
        int recordId;          // 借阅记录ID（成功时有效）
        QString errorMessage;  // 错误信息（失败时有效）
        double fine;           // 罚款金额（还书时使用）

        BorrowResult() : IsSuccess(false), recordId(-1), fine(0.0) {}
        static BorrowResult makeSuccess(int recordId);
        static BorrowResult makeFailure(const QString& error);
    };

    /**
     * @brief 图书信息查询结果（包含借阅状态）
     */
    struct BookDetailInfo {
        Book book;                     // 图书基本信息
        int currentBorrowCount;        // 当前被借出数量
        bool isBorrowedByCurrentUser;  // 是否被当前读者借阅（需要传入readerId）
        QString statusDescription;     // 状态描述

        BookDetailInfo() : currentBorrowCount(0), isBorrowedByCurrentUser(false) {}
    };

    explicit BookService();
    ~BookService();

    // ==================== 图书基础操作 ====================

    /**
     * @brief 添加图书
     * @param book 图书对象
     * @param generatedId 返回生成的ID
     * @return true-成功, false-失败
     * @throws BookServiceException ISBN已存在或数据无效
     */
    bool addBook(const Book& book, int& generatedId);

    /**
     * @brief 软删除图书（通过ID）
     * @param id 图书ID
     * @return true-成功, false-失败
     * @throws BookServiceException 图书不存在或已被借出
     */
    bool deleteBook(int id);

    /**
     * @brief 软删除图书（通过ISBN）
     * @param isbn 国际标准书号
     * @return true-成功, false-失败
     * @throws BookServiceException 图书不存在或已被借出
     */
    bool deleteBookByIsbn(const QString& isbn);

    /**
     * @brief 更新图书信息
     * @param book 图书对象（必须包含有效的ID）
     * @return true-成功, false-失败
     * @throws BookServiceException 图书不存在或ISBN冲突
     * @note ISBN不可更改，如需更改请使用删除+新增
     */
    bool updateBook(const Book& book);

    /**
     * @brief 恢复已删除的图书（通过ID）
     * @param id 图书ID
     * @return true-成功, false-失败
     */
    bool restoreBook(int id);

    /**
     * @brief 恢复已删除的图书（通过ISBN）
     * @param isbn 国际标准书号
     * @return true-成功, false-失败
     */
    bool restoreBookByIsbn(const QString& isbn);

    // ==================== 查询操作 ====================

    /**
     * @brief 根据ID查询图书
     * @param id 图书ID
     * @return 图书指针，不存在返回nullptr
     */
    std::unique_ptr<Book> getBookById(int id);

    /**
     * @brief 根据ISBN查询图书
     * @param isbn 国际标准书号
     * @return 图书指针，不存在返回nullptr
     */
    std::unique_ptr<Book> getBookByIsbn(const QString& isbn);

    /**
     * @brief 获取所有图书
     * @param includeDeleted 是否包含已删除的图书
     * @return 图书列表
     */
    QList<Book> getAllBooks(bool includeDeleted = false);

    /**
     * @brief 搜索图书
     * @param keyword 关键词（匹配标题、作者、ISBN、分类）
     * @param includeDeleted 是否包含已删除的图书
     * @return 图书列表
     */
    QList<Book> searchBooks(const QString& keyword, bool includeDeleted = false);

    /**
     * @brief 按分类获取图书
     * @param category 分类名称
     * @param includeDeleted 是否包含已删除的图书
     * @return 图书列表
     */
    QList<Book> getBooksByCategory(const QString& category, bool includeDeleted = false);

    /**
     * @brief 获取图书详细信息（包含借阅统计）
     * @param bookId 图书ID
     * @param currentReaderId 当前读者ID（可选，用于判断是否被当前读者借阅）
     * @return 图书详细信息
     */
    BookDetailInfo getBookDetailInfo(int bookId, int currentReaderId = -1);

    /**
     * @brief 获取图书的借阅历史
     * @param bookId 图书ID
     * @param includeReturned 是否包含已归还的记录
     * @return 借阅记录列表
     */
    QList<BorrowRecord> getBookBorrowHistory(int bookId, bool includeReturned = true);

    /**
     * @brief 获取图书当前借阅记录（未归还）
     * @param bookId 图书ID
     * @return 借阅记录列表
     */
    QList<BorrowRecord> getCurrentBorrowRecordsByBook(int bookId);

    // ==================== 库存操作 ====================

    /**
     * @brief 更新图书库存
     * @param id 图书ID
     * @param totalCount 总数量
     * @param availableCount 可借数量
     * @return true-成功, false-失败
     * @throws BookServiceException 库存数据无效
     */
    bool updateBookStock(int id, int totalCount, int availableCount);

    /**
     * @brief 检查图书是否可借
     * @param bookId 图书ID
     * @return true-可借, false-不可借
     */
    bool isBookAvailable(int bookId);

    /**
     * @brief 获取图书的可用状态描述
     * @param bookId 图书ID
     * @return 状态描述字符串
     */
    QString getBookAvailabilityDescription(int bookId);

    // ==================== 借还业务（核心） ====================

    /**
     * @brief 借书
     * @param bookId 图书ID
     * @param readerId 读者ID
     * @return 借阅结果
     */
    BorrowResult borrowBook(int bookId, int readerId);

    /**
     * @brief 还书
     * @param recordId 借阅记录ID
     * @param fine 返回计算的罚款金额
     * @return true-成功, false-失败
     */
    bool returnBook(int recordId, double& fine);

    /**
     * @brief 续借
     * @param recordId 借阅记录ID
     * @param newDueDate 返回新的应还日期
     * @return true-成功, false-失败
     */
    bool renewBook(int recordId, QDateTime& newDueDate);

    // ==================== 统计操作 ====================

    /**
     * @brief 获取图书总数
     * @param includeDeleted 是否包含已删除的图书
     * @return 图书总数
     */
    int getBookTotalCount(bool includeDeleted = false);

    /**
     * @brief 获取所有图书的可借总数
     * @return 可借总数
     */
    int getTotalAvailableCount();

    /**
     * @brief 获取分类下的图书数量
     * @param category 分类名称
     * @param includeDeleted 是否包含已删除的图书
     * @return 图书数量
     */
    int getBookCountByCategory(const QString& category, bool includeDeleted = false);

    /**
     * @brief 批量添加图书
     * @param books 图书列表
     * @return true-全部成功, false-有失败
     */
    bool batchAddBooks(const QList<Book>& books);

    /**
     * @brief 批量删除图书
     * @param ids 图书ID列表
     * @return true-全部成功, false-有失败
     */
    bool batchDeleteBooks(const QList<int>& ids);

private:
    // ==================== 私有辅助方法 ====================

    /**
     * @brief 验证ISBN唯一性
     * @param isbn 待验证的ISBN
     * @param excludeBookId 排除的图书ID（用于更新时排除自身）
     * @return true-唯一, false-已存在
     */
    bool isIsbnUnique(const QString& isbn, int excludeBookId = -1);

    /**
     * @brief 检查图书是否有未归还的借阅记录
     * @param bookId 图书ID
     * @return true-有未归还, false-无未归还
     */
    bool hasUnreturnedRecords(int bookId);

    /**
     * @brief 检查读者是否已借阅该书且未归还
     * @param bookId 图书ID
     * @param readerId 读者ID
     * @return true-已借阅, false-未借阅
     */
    bool isBookBorrowedByReader(int bookId, int readerId);

    /**
     * @brief 验证借书条件
     * @param bookId 图书ID
     * @param readerId 读者ID
     * @param errorMessage 返回错误信息
     * @return true-验证通过, false-验证失败
     */
    bool validateBorrowConditions(int bookId, int readerId, QString& errorMessage);

    /**
     * @brief 获取图书的当前借阅数量
     * @param bookId 图书ID
     * @return 当前借阅数量
     */
    int getCurrentBorrowCount(int bookId);

    // ==================== 成员变量 ====================

    std::unique_ptr<BookDao> m_bookDao;
    std::unique_ptr<BorrowRecordDao> m_borrowRecordDao;

    // 可配置常量
    static constexpr int MAX_BORROW_DAYS = 30;        // 默认借阅天数
    static constexpr double DAILY_FINE_RATE = 0.5;    // 每日罚款金额
    static constexpr int MAX_RENEW_TIMES = 1;         // 最大续借次数
};


#endif // BOOKSERVICE_H
