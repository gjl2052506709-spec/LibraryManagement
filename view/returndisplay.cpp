#include "returndisplay.h"

ReturnDisplay::ReturnDisplay(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ReturnDisplay)
    , m_borrowService(new BorrowRecordService())
    , m_bookService(new BookService())
    , m_currentReaderId(-1)
{
    ui->setupUi(this);
    setupUI();
}

ReturnDisplay::~ReturnDisplay()
{
    delete ui;
}

void ReturnDisplay::setupUI()
{
    setWindowTitle("还书");

    // 设置输入框提示
    ui->IDInput->setPlaceholderText("请输入读者编号");

    // 设置列表选择模式为单选
    ui->listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->listWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 连接选择变化信号
    connect(ui->listWidget, &QListWidget::itemSelectionChanged,
            this, &ReturnDisplay::on_listWidget_itemSelectionChanged);

    // 双击列表项也可以触发还书
    connect(ui->listWidget, &QListWidget::itemDoubleClicked,
            this, &ReturnDisplay::returnSelectedBook);

    // 默认禁用还书按钮
    ui->returnBookBtn->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    // 清空列表
    clearList();
}

bool ReturnDisplay::validateReaderId(int& readerId)
{
    QString idStr = ui->IDInput->text().trimmed();

    if (idStr.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入读者编号！");
        ui->IDInput->setFocus();
        return false;
    }

    bool ok;
    readerId = idStr.toInt(&ok);
    if (!ok || readerId <= 0) {
        QMessageBox::warning(this, "输入错误", "读者编号必须为正整数！");
        ui->IDInput->clear();
        ui->IDInput->setFocus();
        return false;
    }

    return true;
}

void ReturnDisplay::loadBorrowRecords(int readerId)
{
    try {
        // 获取该读者的当前借阅记录（未归还）
        m_currentRecords = m_borrowService->getRecordsByReaderId(readerId, false);
        m_currentReaderId = readerId;

        refreshList();

        // 更新按钮状态
        updateReturnButtonState();

        // 显示提示信息
        if (m_currentRecords.isEmpty()) {
            // 添加提示项
            auto item = new QListWidgetItem("该读者当前没有借阅记录", ui->listWidget);
            item->setTextAlignment(Qt::AlignCenter);
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);  // 不可选择
            ui->returnBookBtn->setEnabled(false);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        } else {
            ui->returnBookBtn->setEnabled(true);
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);  // 需要选中才能启用
        }

        qDebug() << "加载读者" << readerId << "的借阅记录，共" << m_currentRecords.size() << "条";

    } catch (const BorrowRecordServiceException& e) {
        QMessageBox::critical(this, "加载失败", e.getMessage());
        clearList();
    }
}

void ReturnDisplay::refreshList()
{
    ui->listWidget->clear();

    if (m_currentRecords.isEmpty()) {
        return;
    }

    // 按借阅日期排序（最新的在前）
    QList<BorrowRecord> sortedRecords = m_currentRecords;
    std::sort(sortedRecords.begin(), sortedRecords.end(),
              [](const BorrowRecord& a, const BorrowRecord& b) {
                  return a.getBorrowDate() > b.getBorrowDate();
              });

    for (const auto& record : sortedRecords) {
        QString displayText = formatRecordDisplay(record);
        auto item = new QListWidgetItem(displayText, ui->listWidget);
        item->setData(Qt::UserRole, record.getId());  // 存储记录ID

        // 如果是逾期记录，用红色标记
        if (record.isOverdue()) {
            item->setForeground(Qt::red);
        }
    }
}

void ReturnDisplay::clearList()
{
    ui->listWidget->clear();
    m_currentRecords.clear();
    m_currentReaderId = -1;
    ui->returnBookBtn->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

QString ReturnDisplay::formatRecordDisplay(const BorrowRecord& record) const
{
    // 获取书籍信息
    QString bookTitle = "未知书籍";
    QString bookAuthor = "";

    try {
        auto book = m_bookService->getBookById(record.getBookId());
        if (book) {
            bookTitle = book->getTitle();
            bookAuthor = book->getAuthor();
        }
    } catch (...) {
        // 忽略异常
    }

    // 格式化日期
    QString borrowDate = record.getBorrowDate().toString("yyyy-MM-dd");
    QString dueDate = record.getDueDate().toString("yyyy-MM-dd");

    // 判断是否逾期
    bool isOverdue = record.isOverdue();
    QString status = isOverdue ? "⚠️ 已逾期" : "正常";

    // 构建显示文本
    return QString("[%1] %2 | %3 | 借出:%4 | 应还:%5 | %6")
        .arg(record.getId())
        .arg(bookTitle)
        .arg(bookAuthor)
        .arg(borrowDate)
        .arg(dueDate)
        .arg(status);
}

int ReturnDisplay::getSelectedRecordId() const
{
    auto selectedItems = ui->listWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return -1;
    }

    return selectedItems.first()->data(Qt::UserRole).toInt();
}

void ReturnDisplay::updateReturnButtonState()
{
    bool hasRecords = !m_currentRecords.isEmpty();
    bool hasSelection = !ui->listWidget->selectedItems().isEmpty();

    ui->returnBookBtn->setEnabled(hasRecords && hasSelection);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(hasRecords && hasSelection);
}

void ReturnDisplay::returnSelectedBook()
{
    int recordId = getSelectedRecordId();
    if (recordId <= 0) {
        QMessageBox::warning(this, "提示", "请选择要归还的书籍！");
        return;
    }

    // 确认对话框
    auto reply = QMessageBox::question(
        this,
        "确认还书",
        "确定要归还选中的书籍吗？\n如果逾期将产生罚款。",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply != QMessageBox::Yes) {
        return;
    }

    try {
        // 执行还书操作
        auto result = m_borrowService->returnBook(recordId);

        if (result.isSuccess) {
            QString message = QString("还书成功！\n");
            if (result.fine > 0) {
                message += QString("产生罚款：¥%1").arg(result.fine, 0, 'f', 2);
            } else {
                message += "无罚款，感谢您按时归还！";
            }

            QMessageBox::information(this, "还书成功", message);

            // 刷新列表
            if (m_currentReaderId > 0) {
                loadBorrowRecords(m_currentReaderId);
            }
        } else {
            QMessageBox::warning(this, "还书失败", result.errorMessage);
        }

    } catch (const BorrowRecordServiceException& e) {
        QMessageBox::critical(this, "操作失败", e.getMessage());
    }
}

// ==================== 信号槽实现 ====================

void ReturnDisplay::on_searchBtn_clicked()
{
    int readerId;
    if (!validateReaderId(readerId)) {
        return;
    }

    loadBorrowRecords(readerId);
}

void ReturnDisplay::on_returnBookBtn_clicked()
{
    returnSelectedBook();
}

void ReturnDisplay::on_buttonBox_accepted()
{
    // 点击OK按钮时，如果有选中的记录，执行还书
    if (getSelectedRecordId() > 0) {
        returnSelectedBook();
    } else {
        // 如果没有选中，但列表不为空，提示用户选择
        if (!m_currentRecords.isEmpty()) {
            QMessageBox::information(this, "提示", "请先在列表中选择要归还的书籍！");
        }
    }
}

void ReturnDisplay::on_buttonBox_rejected()
{
    reject();
}

void ReturnDisplay::on_listWidget_itemSelectionChanged()
{
    updateReturnButtonState();
}