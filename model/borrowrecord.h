#ifndef BORROWRECORD_H
#define BORROWRECORD_H

#include<QString>
#include<QDateTime>

class BorrowRecord
{
public:
    // 借阅状态枚举
    enum class Status {
        BORROWED,   // 已借出
        RETURNED,   // 已归还
        OVERDUE     // 已逾期
    };

    // 构造函数
    BorrowRecord();
    BorrowRecord(int readerId, int bookId, const QDateTime& borrowDate);
    BorrowRecord(int id, int readerId, int bookId,
                 const QDateTime& borrowDate, const QDateTime& dueDate,
                 const QDateTime& returnDate, int status, double fine);

    // === Getter 方法 ===
    int getId() const;
    int getReaderId() const;
    int getBookId() const;
    QDateTime getBorrowDate() const;
    QDateTime getDueDate() const;
    QDateTime getReturnDate() const;
    Status getStatus() const;
    QString getStatusText() const;  // 状态文本描述
    double getFine() const;
    QString getReaderName() const;   // 扩展字段，用于显示
    QString getBookTitle() const;    // 扩展字段，用于显示

    // === Setter 方法 ===
    void setId(int id);
    void setReaderId(int readerId);
    void setBookId(int bookId);
    void setBorrowDate(const QDateTime& borrowDate);
    void setDueDate(const QDateTime& dueDate);
    void setReturnDate(const QDateTime& returnDate);
    void setStatus(Status status);
    void setFine(double fine);
    void setReaderName(const QString& name);   // 扩展字段
    void setBookTitle(const QString& title);   // 扩展字段

    // === 业务方法 ===
    bool isOverdue() const;                    // 是否逾期
    int getBorrowDays() const;                 // 获取借阅天数
    double calculateFine() const;              // 计算罚款（假设每天0.5元）
    bool canRenew() const;                     // 是否可以续借（未逾期且未归还）

    // 验证方法
    bool isValid() const;                      // 验证记录是否有效

private:
    int id;                    // 借阅记录ID
    int readerId;              // 读者ID
    int bookId;                // 图书ID
    QString readerName;        // 读者姓名（冗余字段，用于显示）
    QString bookTitle;         // 图书标题（冗余字段，用于显示）

    QDateTime borrowDate;      // 借阅日期
    QDateTime dueDate;         // 应还日期
    QDateTime returnDate;      // 实际归还日期

    Status status;             // 借阅状态
    double fine;               // 罚款金额

    // 常量定义
    static constexpr int DEFAULT_BORROW_DAYS = 30;  // 默认借阅天数30天
    static constexpr double DAILY_FINE_RATE = 0.5;  // 每天罚款0.5元
    static constexpr int MAX_RENEW_TIMES = 1;       // 最大续借次数
};

#endif // BORROWRECORD_H
