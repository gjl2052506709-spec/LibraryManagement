#ifndef READER_H
#define READER_H

#include<QString>
#include<QDateTime>

class Reader
{
public:
    Reader();
    Reader(const QString& name, const QString& phone, const QString& email, const QString& address);

    //===getter方法===
    int getId() const;
    QString getName() const;
    QString getPhone() const;
    QString getEmail() const;
    QString getAddress() const;
    QDateTime getRegisterAt() const;
    bool getIsDelete() const;
    QDateTime getDeleteAt() const;

    //===setter方法===
    void setName(const QString& name);
    void setPhone(const QString& phone);
    void setEmail(const QString& email);
    void setAddress(const QString& address);

    //===业务方法===
    bool isActive() const;          // 是否有效（未被注销且未删除）
    QString getStatusText() const;  // 状态描述（正常/已注销/已删除）

    //===软删除===
    bool softDelete();              // 注销（软删除）
    void restore();                 // 恢复

private:
    int id;                 // 读者编号
    QString name;           // 读者姓名
    QString phone;          // 联系方式（电话）
    QString email;          // 电子邮箱
    QString address;        // 联系地址

    QDateTime registerAt;   // 注册时间
    bool isDelete;          // 软删除标记（注销）
    QDateTime deleteAt;     // 注销时间

    void setId(int id);     // 私有化id的setter
    void setRegisterAt(const QDateTime& registerAt);
    void setIsDelete(bool isDelete);
    void setDeleteAt(const QDateTime& deleteAt);
    friend class ReaderDao; // 友元
};

#endif // READER_H
