#include "borrow.h"
#include "ui_borrow.h"

Borrow::Borrow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Borrow)
    , m_borrowService(new BorrowRecordService())
    , m_bookService(new BookService())
{
    ui->setupUi(this);
    setupUI();
}

Borrow::~Borrow()
{
    delete ui;
}

void Borrow::setupUI()
{
    // 设置窗口标题
    setWindowTitle("借书");

    // 设置提示信息
    ui->label_4->setText("读者编号为必填，书籍编号和书籍名称至少填写一个");
    ui->label_4->setStyleSheet("color: gray;");

    // 设置输入框提示
    ui->readerIDInput->setPlaceholderText("请输入读者ID");
    ui->bookIDInput->setPlaceholderText("请输入书籍ID（可选）");
    ui->bookTitleInput->setPlaceholderText("请输入书籍名称（可选）");
}

void Borrow::on_buttonBox_accepted()
{
    int readerId = 0;
    int bookId = 0;
    QString bookTitle;

    // 1. 验证输入
    if (!validateInput(readerId, bookId, bookTitle)) {
        return;
    }

    // 2. 如果输入的是书名，查找书籍
    if (bookId == 0 && !bookTitle.isEmpty()) {
        if (!findBookByTitle(bookTitle, bookId)) {
            return;
        }
    }

    // 3. 执行借书操作
    auto result = m_borrowService->borrowBook(bookId, readerId);

    // 4. 显示结果
    showBorrowResult(result);

    // 5. 如果借书成功，关闭对话框
    if (result.isSuccess) {
        accept();
    }
}

void Borrow::on_buttonBox_rejected()
{
    reject();
}

bool Borrow::validateInput(int& readerId, int& bookId, QString& bookTitle)
{
    // 获取输入
    QString readerIdStr = ui->readerIDInput->text().trimmed();
    QString bookIdStr = ui->bookIDInput->text().trimmed();
    bookTitle = ui->bookTitleInput->text().trimmed();

    // 验证读者ID
    if (readerIdStr.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入读者编号！");
        ui->readerIDInput->setFocus();
        return false;
    }

    bool ok;
    readerId = readerIdStr.toInt(&ok);
    if (!ok || readerId <= 0) {
        QMessageBox::warning(this, "输入错误", "读者编号必须为正整数！");
        ui->readerIDInput->clear();
        ui->readerIDInput->setFocus();
        return false;
    }

    // 验证书籍编号（如果有输入）
    if (!bookIdStr.isEmpty()) {
        bookId = bookIdStr.toInt(&ok);
        if (!ok || bookId <= 0) {
            QMessageBox::warning(this, "输入错误", "书籍编号必须为正整数！");
            ui->bookIDInput->clear();
            ui->bookIDInput->setFocus();
            return false;
        }
        // 如果输入了有效的书籍ID，不需要再通过书名查找
        return true;
    }

    // 如果书籍ID为空，检查书名是否填写
    if (bookTitle.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请至少填写书籍编号或书籍名称！");
        ui->bookTitleInput->setFocus();
        return false;
    }

    // 验证书名长度
    if (bookTitle.length() < 1) {
        QMessageBox::warning(this, "输入错误", "书籍名称不能为空！");
        ui->bookTitleInput->setFocus();
        return false;
    }

    return true;
}

bool Borrow::findBookByTitle(const QString& title, int& bookId)
{
    try {
        // 搜索书籍
        auto books = m_bookService->searchBooks(title, false);

        if (books.isEmpty()) {
            QMessageBox::warning(this, "查找失败", QString("未找到书名包含 \"%1\" 的书籍！").arg(title));
            ui->bookTitleInput->setFocus();
            ui->bookTitleInput->selectAll();
            return false;
        }

        // 如果只找到一本，直接使用
        if (books.size() == 1) {
            bookId = books.first().getId();
            return true;
        }

        // 如果找到多本，让用户选择
        QStringList items;
        for (const auto& book : books) {
            QString info = QString("ID:%1 | %2 | %3 | 可借:%4")
                               .arg(book.getId())
                               .arg(book.getTitle())
                               .arg(book.getAuthor())
                               .arg(book.getAvailableCount());
            items.append(info);
        }

        bool ok;
        QString selected = QInputDialog::getItem(
            this,
            "选择书籍",
            QString("找到 %1 本包含 \"%2\" 的书籍，请选择：").arg(books.size()).arg(title),
            items,
            0,
            false,
            &ok
            );

        if (!ok || selected.isEmpty()) {
            return false;
        }

        // 从选中的字符串中提取ID
        // 格式: "ID:123 | 书名 | 作者 | 可借:X"
        int idStart = selected.indexOf("ID:") + 3;
        int idEnd = selected.indexOf(" |", idStart);
        if (idStart > 0 && idEnd > idStart) {
            QString idStr = selected.mid(idStart, idEnd - idStart);
            bookId = idStr.toInt();
            return true;
        }

        QMessageBox::warning(this, "错误", "无法解析选中的书籍信息！");
        return false;

    } catch (const BookServiceException& e) {
        QMessageBox::critical(this, "查找书籍失败", e.getMessage());
        return false;
    }
}

void Borrow::showBorrowResult(const BorrowRecordService::BorrowResult& result)
{
    if (result.isSuccess) {
        QMessageBox::information(
            this,
            "借书成功",
            QString("借书成功！\n借阅记录ID: %1\n请按时归还，逾期将产生罚款。")
                .arg(result.recordId)
            );
    } else {
        QMessageBox::warning(
            this,
            "借书失败",
            QString("借书失败：\n%1").arg(result.errorMessage)
            );
    }
}