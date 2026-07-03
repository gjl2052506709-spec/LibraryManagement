#include "reader.h"

Reader::Reader() {}

Reader::Reader(const QString& name, const QString& phone, const QString& email, const QString& address)
{
    setName(name);
    setPhone(phone);
    setEmail(email);
    setAddress(address);
    registerAt = QDateTime::currentDateTime();
    isDelete = false;
}

//===getter方法===
int Reader::getId() const
{
    return id;
}

QString Reader::getName() const
{
    return name;
}

QString Reader::getPhone() const
{
    return phone;
}

QString Reader::getEmail() const
{
    return email;
}

QString Reader::getAddress() const
{
    return address;
}

QDateTime Reader::getRegisterAt() const
{
    return registerAt;
}

bool Reader::getIsDelete() const
{
    return isDelete;
}

QDateTime Reader::getDeleteAt() const
{
    return deleteAt;
}

//===setter方法===
void Reader::setId(int id)
{
    this->id = id;
}

void Reader::setRegisterAt(const QDateTime &registerAt)
{
    this->registerAt = registerAt;
}

void Reader::setIsDelete(bool isDelete)
{
    this->isDelete = isDelete;
}

void Reader::setDeleteAt(const QDateTime &deleteAt)
{
    this->deleteAt = deleteAt;
}

void Reader::setName(const QString& name)
{
    if (name.trimmed().isEmpty()) {
        throw QString("读者姓名不能为空");
    }
    if (name.trimmed().length() > 50) {
        throw QString("姓名长度超过限制，限制为50个字符");
    }
    this->name = name.trimmed();
}

void Reader::setPhone(const QString& phone)
{
    QString cleaned = phone.trimmed();
    if (cleaned.isEmpty()) {
        throw QString("联系方式不能为空");
    }
    // 简单校验：只允许数字、+、-、(、)、空格
    bool valid = std::all_of(cleaned.begin(), cleaned.end(), [](QChar c) {
        return c.isDigit() || c == '+' || c == ' ';
    });

    if (!valid) {
        throw QString("联系方式包含无效字符");
    }

    if (cleaned.length() < 7 || cleaned.length() > 20) {
        throw QString("联系方式长度应在7~20位之间");
    }
    this->phone = cleaned;
}

void Reader::setEmail(const QString& email)
{
    QString cleaned = email.trimmed();
    if (cleaned.isEmpty()) {
        this->email = QString();  // 允许为空
        return;
    }
    // 简单邮箱格式校验：包含@和.
    if (!cleaned.contains('@') || !cleaned.contains('.')) {
        throw QString("邮箱格式不正确，应包含@和.");
    }
    if (cleaned.length() > 100) {
        throw QString("邮箱长度超过限制，限制为100个字符");
    }
    this->email = cleaned;
}

void Reader::setAddress(const QString& address)
{
    QString cleaned = address.trimmed();
    if (cleaned.isEmpty()) {
        this->address = QString();  // 允许为空
        return;
    }
    if (cleaned.length() > 200) {
        throw QString("地址长度超过限制，限制为200个字符");
    }
    this->address = cleaned;
}

//===业务方法===
bool Reader::isActive() const
{
    return !isDelete;
}

QString Reader::getStatusText() const
{
    if (isDelete) {
        return "已注销";
    }
    return "正常";
}

//===软删除===
bool Reader::softDelete()
{
    if (isDelete) {
        return false;  // 已经注销
    }
    isDelete = true;
    deleteAt = QDateTime::currentDateTime();
    return true;
}

void Reader::restore()
{
    isDelete = false;
    deleteAt = QDateTime();
}
