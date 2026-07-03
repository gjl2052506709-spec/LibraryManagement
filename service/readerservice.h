#ifndef READERSERVICE_H
#define READERSERVICE_H

#include"dao/readerdao.h"
#include"dao/borrowrecorddao.h"

/**
 * @brief 读者业务异常类
 */
class ReaderServiceException : public std::exception
{
public:
    explicit ReaderServiceException(const QString& message);
    const char* what() const noexcept override;
    QString getMessage() const;

private:
    QString m_message;
};

/**
 * @brief 读者业务服务类
 *
 * 提供读者相关的业务逻辑处理，包括：
 * - 读者的增删改查（含业务规则校验）
 * - 读者借阅历史查询
 * - 读者借阅权限控制
 * - 读者借阅统计
 */
class ReaderService
{
public:
    /**
     * @brief 读者借阅统计信息
     */
    struct BorrowStatistics {
        int totalBorrowCount;      // 总借阅次数
        int currentBorrowCount;    // 当前借阅数量（未归还）
        int overdueCount;          // 逾期数量
        double totalFine;          // 总罚款金额
        int returnedCount;         // 已归还数量

        BorrowStatistics()
            : totalBorrowCount(0)
            , currentBorrowCount(0)
            , overdueCount(0)
            , totalFine(0.0)
            , returnedCount(0)
        {}
    };

    /**
     * @brief 读者详细信息（包含借阅状态）
     */
    struct ReaderDetailInfo {
        Reader reader;                     // 读者基本信息
        int currentBorrowCount;            // 当前借阅数量
        int overdueCount;                  // 逾期数量
        double totalFine;                  // 总罚款金额
        bool canBorrow;                    // 是否可借书
        QString statusDescription;         // 状态描述
        QString borrowLimitDescription;    // 借阅限额描述
        QList<BorrowRecord> currentBorrowRecords;  // 当前借阅记录（最多5条）

        ReaderDetailInfo()
            : currentBorrowCount(0)
            , overdueCount(0)
            , totalFine(0.0)
            , canBorrow(false)
        {}
    };

    /**
     * @brief 读者借阅限额状态
     */
    struct BorrowLimitStatus {
        bool canBorrow;              // 是否可以借书
        int currentBorrowCount;      // 当前借阅数量
        int maxBorrowLimit;          // 最大借阅限额
        int remainingLimit;          // 剩余可借数量
        bool hasOverdue;             // 是否有逾期记录
        bool isDeleted;              // 是否已注销
        QString reason;              // 不可借原因（当 canBorrow == false 时）

        BorrowLimitStatus()
            : canBorrow(false)
            , currentBorrowCount(0)
            , maxBorrowLimit(ReaderService::MAX_BORROW_LIMIT)
            , remainingLimit(0)
            , hasOverdue(false)
            , isDeleted(false)
        {}
    };

    explicit ReaderService();
    ~ReaderService();

    // ==================== 读者基础操作 ====================

    /**
     * @brief 添加读者
     * @param reader 读者对象
     * @param generatedId 返回生成的ID
     * @return true-成功, false-失败
     * @throws ReaderServiceException 电话已存在或数据无效
     */
    bool addReader(const Reader& reader, int& generatedId);

    /**
     * @brief 软删除读者（通过ID）
     * @param id 读者ID
     * @return true-成功, false-失败
     * @throws ReaderServiceException 读者不存在、已被删除或有未归还的借阅
     */
    bool deleteReader(int id);

    /**
     * @brief 软删除读者（通过姓名）
     * @param name 读者姓名
     * @return true-成功, false-失败
     * @throws ReaderServiceException 读者不存在、已被删除或有未归还的借阅
     * @note 如果有多个同名读者，只删除第一个匹配的
     */
    bool deleteReaderByName(const QString& name);

    /**
     * @brief 更新读者信息
     * @param reader 读者对象（必须包含有效的ID）
     * @return true-成功, false-失败
     * @throws ReaderServiceException 读者不存在或电话冲突
     * @note 注册时间不可更改
     */
    bool updateReader(const Reader& reader);

    /**
     * @brief 恢复已删除的读者（通过ID）
     * @param id 读者ID
     * @return true-成功, false-失败
     */
    bool restoreReader(int id);

    /**
     * @brief 恢复已删除的读者（通过电话）
     * @param phone 读者电话
     * @return true-成功, false-失败
     */
    bool restoreReaderByPhone(const QString& phone);

    // ==================== 查询操作 ====================

    /**
     * @brief 根据ID查询读者
     * @param id 读者ID
     * @return 读者指针，不存在返回nullptr
     */
    std::unique_ptr<Reader> getReaderById(int id);

    /**
     * @brief 根据电话查询读者
     * @param phone 读者电话
     * @return 读者指针，不存在返回nullptr
     */
    std::unique_ptr<Reader> getReaderByPhone(const QString& phone);

    /**
     * @brief 获取所有读者
     * @param includeDeleted 是否包含已删除的读者
     * @return 读者列表
     */
    QList<Reader> getAllReaders(bool includeDeleted = false);

    /**
     * @brief 搜索读者
     * @param keyword 关键词（匹配姓名、电话、邮箱）
     * @param includeDeleted 是否包含已删除的读者
     * @return 读者列表
     */
    QList<Reader> searchReaders(const QString& keyword, bool includeDeleted = false);

    /**
     * @brief 按姓名模糊查询读者
     * @param name 姓名关键词
     * @param includeDeleted 是否包含已删除的读者
     * @return 读者列表
     */
    QList<Reader> getReadersByName(const QString& name, bool includeDeleted = false);

    /**
     * @brief 获取读者详细信息（包含借阅统计）
     * @param readerId 读者ID
     * @return 读者详细信息
     */
    ReaderDetailInfo getReaderDetailInfo(int readerId);

    // ==================== 借阅相关 ====================

    /**
     * @brief 获取读者的借阅历史
     * @param readerId 读者ID
     * @param includeReturned 是否包含已归还的记录
     * @return 借阅记录列表
     */
    QList<BorrowRecord> getReaderBorrowHistory(int readerId, bool includeReturned = true);

    /**
     * @brief 获取读者当前借阅记录（未归还）
     * @param readerId 读者ID
     * @return 借阅记录列表
     */
    QList<BorrowRecord> getCurrentBorrowRecords(int readerId);

    /**
     * @brief 获取读者借阅统计信息
     * @param readerId 读者ID
     * @return 借阅统计信息
     */
    BorrowStatistics getReaderBorrowStatistics(int readerId);

    /**
     * @brief 检查读者是否可以借书
     * @param readerId 读者ID
     * @param errorMessage 返回错误信息
     * @return true-可以借书, false-不可以借书
     */
    bool canReaderBorrow(int readerId, QString& errorMessage);

    /**
     * @brief 获取读者借阅限额状态
     * @param readerId 读者ID
     * @return 借阅限额状态
     */
    BorrowLimitStatus getReaderBorrowLimitStatus(int readerId);

    // ==================== 统计操作 ====================

    /**
     * @brief 获取读者总数
     * @param includeDeleted 是否包含已删除的读者
     * @return 读者总数
     */
    int getReaderTotalCount(bool includeDeleted = false);

    /**
     * @brief 获取活跃读者数量（未注销）
     * @return 活跃读者数量
     */
    int getActiveReaderCount();

    /**
     * @brief 获取读者的借阅总数
     * @param readerId 读者ID
     * @param includeReturned 是否包含已归还的记录
     * @return 借阅总数
     */
    int getReaderBorrowCount(int readerId, bool includeReturned = false);

    // ==================== 批量操作 ====================

    /**
     * @brief 批量添加读者
     * @param readers 读者列表
     * @return true-全部成功, false-有失败
     * @throws ReaderServiceException 存在重复电话
     */
    bool batchAddReaders(const QList<Reader>& readers);

    /**
     * @brief 批量删除读者
     * @param ids 读者ID列表
     * @return true-全部成功, false-有失败
     * @throws ReaderServiceException 存在有未归还借阅的读者
     */
    bool batchDeleteReaders(const QList<int>& ids);

private:
    // ==================== 私有辅助方法 ====================

    /**
     * @brief 验证读者数据
     * @param reader 读者对象
     * @throws ReaderServiceException 数据无效
     */
    void validateReaderData(const Reader& reader);

    /**
     * @brief 验证电话唯一性
     * @param phone 待验证的电话
     * @param excludeReaderId 排除的读者ID（用于更新时排除自身）
     * @return true-唯一, false-已存在
     */
    bool isPhoneUnique(const QString& phone, int excludeReaderId = -1);

    /**
     * @brief 检查读者是否有未归还的借阅记录
     * @param readerId 读者ID
     * @return true-有未归还, false-无未归还
     */
    bool hasUnreturnedRecords(int readerId);

    /**
     * @brief 获取读者当前借阅数量（未归还）
     * @param readerId 读者ID
     * @return 当前借阅数量
     */
    int getCurrentBorrowCount(int readerId);

    /**
     * @brief 获取读者逾期数量
     * @param readerId 读者ID
     * @return 逾期数量
     */
    int getOverdueCount(int readerId);

    /**
     * @brief 计算读者的总罚款金额
     * @param readerId 读者ID
     * @return 总罚款金额
     */
    double calculateTotalFine(int readerId);

    // ==================== 成员变量 ====================

    std::unique_ptr<ReaderDao> m_readerDao;
    std::unique_ptr<BorrowRecordDao> m_borrowRecordDao;

    // ==================== 常量配置 ====================

    /**
     * @brief 最大借阅数量限制
     */
    static constexpr int MAX_BORROW_LIMIT = 5;

    /**
     * @brief 默认借阅天数
     */
    static constexpr int MAX_BORROW_DAYS = 30;

    /**
     * @brief 每日罚款金额（元）
     */
    static constexpr double DAILY_FINE_RATE = 0.5;

    /**
     * @brief 姓名最大长度
     */
    static constexpr int MAX_NAME_LENGTH = 50;

    /**
     * @brief 电话最大长度
     */
    static constexpr int MAX_PHONE_LENGTH = 20;

    /**
     * @brief 邮箱最大长度
     */
    static constexpr int MAX_EMAIL_LENGTH = 100;

    /**
     * @brief 地址最大长度
     */
    static constexpr int MAX_ADDRESS_LENGTH = 200;
};

#endif // READERSERVICE_H
