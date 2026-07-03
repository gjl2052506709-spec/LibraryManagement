#ifndef BORROWRECORDSERVICE_H
#define BORROWRECORDSERVICE_H

#include"dao/borrowrecorddao.h"
#include"dao/bookdao.h"
#include"dao/readerdao.h"

/**
 * @brief 借阅记录业务异常类
 */
class BorrowRecordServiceException : public std::exception
{
public:
    explicit BorrowRecordServiceException(const QString& message);
    const char* what() const noexcept override;
    QString getMessage() const;

private:
    QString m_message;
};

/**
 * @brief 借阅记录业务服务类
 *
 * 提供借阅记录相关的业务逻辑处理，包括：
 * - 借阅记录的增删改查（含业务规则校验）
 * - 借书、还书、续借核心业务（带事务）
 * - 逾期管理
 * - 罚款计算与统计
 */
class BorrowRecordService
{
public:
    /**
     * @brief 借书结果结构体
     */
    struct BorrowResult {
        bool isSuccess;          // 是否成功
        int recordId;            // 借阅记录ID（成功时有效）
        QString errorMessage;    // 错误信息（失败时有效）

        BorrowResult() : isSuccess(false), recordId(-1) {}

        static BorrowResult success(int recordId) {
            BorrowResult result;
            result.isSuccess = true;
            result.recordId = recordId;
            return result;
        }

        static BorrowResult failure(const QString& error) {
            BorrowResult result;
            result.isSuccess = false;
            result.errorMessage = error;
            return result;
        }
    };

    /**
     * @brief 还书结果结构体
     */
    struct ReturnResult {
        bool isSuccess;          // 是否成功
        double fine;             // 罚款金额（成功时有效）
        QString errorMessage;    // 错误信息（失败时有效）

        ReturnResult() : isSuccess(false), fine(0.0) {}

        static ReturnResult success(double fine) {
            ReturnResult result;
            result.isSuccess = true;
            result.fine = fine;
            return result;
        }

        static ReturnResult failure(const QString& error) {
            ReturnResult result;
            result.isSuccess = false;
            result.errorMessage = error;
            return result;
        }
    };

    /**
     * @brief 续借结果结构体
     */
    struct RenewResult {
        bool isSuccess;          // 是否成功
        QDateTime newDueDate;    // 新的应还日期（成功时有效）
        QString errorMessage;    // 错误信息（失败时有效）

        RenewResult() : isSuccess(false) {}

        static RenewResult success(const QDateTime& newDueDate) {
            RenewResult result;
            result.isSuccess = true;
            result.newDueDate = newDueDate;
            return result;
        }

        static RenewResult failure(const QString& error) {
            RenewResult result;
            result.isSuccess = false;
            result.errorMessage = error;
            return result;
        }
    };

    /**
     * @brief 借阅记录详细信息（包含读者姓名、图书标题）
     */
    struct BorrowRecordDetail {
        BorrowRecord record;         // 基础借阅记录
        QString readerName;          // 读者姓名
        QString bookTitle;           // 图书标题
        QString bookAuthor;          // 图书作者
        QString bookIsbn;            // 图书ISBN
        int remainingStock;          // 图书剩余库存（仅当前查询时有效）

        BorrowRecordDetail() : remainingStock(0) {}
    };

    /**
     * @brief 借阅统计信息
     */
    struct BorrowStatistics {
        int totalBorrowCount;        // 总借阅次数
        int currentBorrowCount;      // 当前借阅数量（未归还）
        int overdueCount;            // 逾期数量
        int returnedCount;           // 已归还数量
        double totalFine;            // 总罚款金额（已归还的）
        double pendingFine;          // 待缴纳罚款（逾期未还的）
        QDateTime statisticsTime;    // 统计时间

        BorrowStatistics()
            : totalBorrowCount(0)
            , currentBorrowCount(0)
            , overdueCount(0)
            , returnedCount(0)
            , totalFine(0.0)
            , pendingFine(0.0)
            , statisticsTime(QDateTime::currentDateTime())
        {}
    };

    explicit BorrowRecordService();
    ~BorrowRecordService();

    // ==================== 基础CRUD操作 ====================

    /**
     * @brief 添加借阅记录（管理用，一般使用 borrowBook）
     * @param record 借阅记录对象
     * @param generatedId 返回生成的ID
     * @return true-成功, false-失败
     */
    bool addBorrowRecord(const BorrowRecord& record, int& generatedId);

    /**
     * @brief 删除借阅记录（物理删除，谨慎使用）
     * @param id 借阅记录ID
     * @return true-成功, false-失败
     * @throws BorrowRecordServiceException 记录不存在或已被归还（不允许删除已归还记录）
     */
    bool deleteBorrowRecord(int id);

    /**
     * @brief 更新借阅记录（管理用）
     * @param record 借阅记录对象
     * @return true-成功, false-失败
     * @throws BorrowRecordServiceException 记录不存在或数据无效
     */
    bool updateBorrowRecord(const BorrowRecord& record);

    /**
     * @brief 获取借阅记录详细信息（含读者姓名、图书标题）
     * @param id 借阅记录ID
     * @return 借阅记录详细信息
     */
    std::unique_ptr<BorrowRecordDetail> getBorrowRecordDetailById(int id);

    // ==================== 查询操作 ====================

    /**
     * @brief 根据ID查询借阅记录
     * @param id 借阅记录ID
     * @return 借阅记录指针
     */
    std::unique_ptr<BorrowRecord> getBorrowRecordById(int id);

    /**
     * @brief 获取读者的借阅记录
     * @param readerId 读者ID
     * @param includeReturned 是否包含已归还的记录
     * @return 借阅记录列表
     */
    QList<BorrowRecord> getRecordsByReaderId(int readerId, bool includeReturned = false);

    /**
     * @brief 获取读者的借阅记录（含详细信息）
     * @param readerId 读者ID
     * @param includeReturned 是否包含已归还的记录
     * @return 借阅记录详细信息列表
     */
    QList<BorrowRecordDetail> getRecordsDetailByReaderId(int readerId, bool includeReturned = false);

    /**
     * @brief 获取图书的借阅记录
     * @param bookId 图书ID
     * @param includeReturned 是否包含已归还的记录
     * @return 借阅记录列表
     */
    QList<BorrowRecord> getRecordsByBookId(int bookId, bool includeReturned = false);

    /**
     * @brief 获取图书的借阅记录（含详细信息）
     * @param bookId 图书ID
     * @param includeReturned 是否包含已归还的记录
     * @return 借阅记录详细信息列表
     */
    QList<BorrowRecordDetail> getRecordsDetailByBookId(int bookId, bool includeReturned = false);

    /**
     * @brief 按状态获取借阅记录
     * @param status 借阅状态
     * @return 借阅记录列表
     */
    QList<BorrowRecord> getRecordsByStatus(BorrowRecord::Status status);

    /**
     * @brief 获取所有借阅记录
     * @param includeReturned 是否包含已归还的记录
     * @return 借阅记录列表
     */
    QList<BorrowRecord> getAllRecords(bool includeReturned = false);

    /**
     * @brief 获取所有逾期记录
     * @return 借阅记录列表
     */
    QList<BorrowRecord> getOverdueRecords();

    /**
     * @brief 获取所有逾期记录（含详细信息）
     * @return 借阅记录详细信息列表
     */
    QList<BorrowRecordDetail> getOverdueRecordsDetail();

    /**
     * @brief 按日期范围获取借阅记录
     * @param startDate 开始日期
     * @param endDate 结束日期
     * @return 借阅记录列表
     */
    QList<BorrowRecord> getRecordsByDateRange(const QDateTime& startDate, const QDateTime& endDate);

    // ==================== 核心借还业务 ====================

    /**
     * @brief 借书（核心业务，带事务）
     * @param bookId 图书ID
     * @param readerId 读者ID
     * @return 借阅结果
     */
    BorrowResult borrowBook(int bookId, int readerId);

    /**
     * @brief 还书（核心业务，带事务）
     * @param recordId 借阅记录ID
     * @return 还书结果（包含罚款金额）
     */
    ReturnResult returnBook(int recordId);

    /**
     * @brief 还书（带罚款金额参数，用于手动调整罚款）
     * @param recordId 借阅记录ID
     * @param manualFine 手动指定的罚款金额（-1表示自动计算）
     * @return 还书结果（包含罚款金额）
     */
    ReturnResult returnBook(int recordId, double manualFine);

    /**
     * @brief 续借（核心业务）
     * @param recordId 借阅记录ID
     * @return 续借结果
     */
    RenewResult renewBook(int recordId);

    /**
     * @brief 批量续借（同一读者的所有未归还记录）
     * @param readerId 读者ID
     * @param successCount 返回成功续借的数量
     * @param failedIds 返回失败的记录ID列表
     * @return 至少成功续借1条返回true
     */
    bool batchRenewBooks(int readerId, int& successCount, QList<int>& failedIds);

    // ==================== 批量操作 ====================

    /**
     * @brief 批量添加借阅记录
     * @param records 借阅记录列表
     * @return true-全部成功, false-有失败
     */
    bool batchAddRecords(const QList<BorrowRecord>& records);

    /**
     * @brief 批量删除借阅记录
     * @param ids 借阅记录ID列表
     * @return true-全部成功, false-有失败
     */
    bool batchDeleteRecords(const QList<int>& ids);

    // ==================== 统计操作 ====================

    /**
     * @brief 获取借阅记录总数
     * @param includeReturned 是否包含已归还的记录
     * @return 记录总数
     */
    int getTotalCount(bool includeReturned = false);

    /**
     * @brief 获取读者的借阅次数
     * @param readerId 读者ID
     * @param includeReturned 是否包含已归还的记录
     * @return 借阅次数
     */
    int getCountByReader(int readerId, bool includeReturned = false);

    /**
     * @brief 获取图书的被借次数
     * @param bookId 图书ID
     * @param includeReturned 是否包含已归还的记录
     * @return 被借次数
     */
    int getCountByBook(int bookId, bool includeReturned = false);

    /**
     * @brief 获取逾期记录数量
     * @return 逾期记录数量
     */
    int getOverdueCount();

    /**
     * @brief 获取读者的总罚款金额（已归还且已产生罚款的记录）
     * @param readerId 读者ID
     * @return 总罚款金额
     */
    double getTotalFineByReader(int readerId);

    /**
     * @brief 获取系统的总罚款金额（所有已归还记录）
     * @return 总罚款金额
     */
    double getTotalFine();

    /**
     * @brief 获取逾期待缴罚款总额（所有逾期未还的记录，按当前时间计算）
     * @return 待缴罚款总额
     */
    double getPendingFineTotal();

    /**
     * @brief 获取读者借阅统计信息
     * @param readerId 读者ID
     * @return 借阅统计信息
     */
    BorrowStatistics getReaderStatistics(int readerId);

    /**
     * @brief 获取系统整体借阅统计信息
     * @return 借阅统计信息
     */
    BorrowStatistics getSystemStatistics();

    // ==================== 业务校验方法 ====================

    /**
     * @brief 检查读者是否可以借书
     * @param readerId 读者ID
     * @param errorMessage 返回错误信息
     * @return true-可以借书, false-不可以借书
     */
    bool canReaderBorrow(int readerId, QString& errorMessage);

    /**
     * @brief 检查图书是否可以借阅
     * @param bookId 图书ID
     * @param errorMessage 返回错误信息
     * @return true-可以借阅, false-不可以借阅
     */
    bool canBookBeBorrowed(int bookId, QString& errorMessage);

    /**
     * @brief 检查读者是否已借阅该书且未归还
     * @param bookId 图书ID
     * @param readerId 读者ID
     * @return true-已借阅, false-未借阅
     */
    bool isBookBorrowedByReader(int bookId, int readerId);

    /**
     * @brief 计算指定记录的罚款金额
     * @param recordId 借阅记录ID
     * @return 罚款金额
     */
    double calculateFine(int recordId);

    /**
     * @brief 计算指定读者的待缴罚款（所有逾期未还记录）
     * @param readerId 读者ID
     * @return 待缴罚款总额
     */
    double calculatePendingFine(int readerId);

    /**
     * @brief 检查读者是否有逾期未还的图书
     * @param readerId 读者ID
     * @return true-有逾期, false-无逾期
     */
    bool hasOverdueRecords(int readerId);

private:
    // ==================== 私有辅助方法 ====================

    /**
     * @brief 验证借书条件
     * @param bookId 图书ID
     * @param readerId 读者ID
     * @param errorMessage 返回错误信息
     * @return true-验证通过, false-验证失败
     */
    bool validateBorrowConditions(int bookId, int readerId, QString& errorMessage);

    /**
     * @brief 验证借阅记录数据
     * @param record 借阅记录对象
     * @throws BorrowRecordServiceException 数据无效
     */
    void validateRecordData(const BorrowRecord& record);

    /**
     * @brief 填充借阅记录详细信息
     * @param records 借阅记录列表
     * @return 借阅记录详细信息列表
     */
    QList<BorrowRecordDetail> enrichRecordsWithDetail(const QList<BorrowRecord>& records);

    /**
     * @brief 填充单条借阅记录详细信息
     * @param record 借阅记录
     * @return 借阅记录详细信息
     */
    BorrowRecordDetail enrichRecordWithDetail(const BorrowRecord& record);

    /**
     * @brief 获取读者当前借阅数量
     * @param readerId 读者ID
     * @return 当前借阅数量
     */
    int getCurrentBorrowCount(int readerId);

    /**
     * @brief 获取读者逾期数量
     * @param readerId 读者ID
     * @return 逾期数量
     */
    int getReaderOverdueCount(int readerId);

    /**
     * @brief 计算逾期罚款（按天计算，每天0.5元）
     * @param dueDate 应还日期
     * @param returnDate 实际归还日期（如果已还）
     * @param isReturned 是否已归还
     * @return 罚款金额
     */
    double calculateFineByDates(const QDateTime& dueDate, const QDateTime& returnDate, bool isReturned);

    // ==================== 成员变量 ====================

    std::unique_ptr<BorrowRecordDao> m_borrowRecordDao;
    std::unique_ptr<BookDao> m_bookDao;
    std::unique_ptr<ReaderDao> m_readerDao;

    // ==================== 常量配置 ====================

    static constexpr int DEFAULT_BORROW_DAYS = 30;        // 默认借阅天数
    static constexpr int MAX_BORROW_LIMIT = 5;            // 读者最大借阅数量
    static constexpr double DAILY_FINE_RATE = 0.5;        // 每日罚款金额（元）
    static constexpr int MAX_RENEW_TIMES = 1;             // 最大续借次数
    static constexpr int MAX_DISPLAY_RECORDS = 100;       // 最大显示记录数
};


#endif // BORROWRECORDSERVICE_H
