#include "bookdisplaywidget.h"
#include "ui_bookdisplaywidget.h"

BookDisplayWidget::BookDisplayWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::BookDisplayWidget)
    , m_bookService(new BookService())
    , m_tableModel(new QStandardItemModel(this))
    , m_currentPage(0)
    , m_pageSize(10)
    , m_currentCategory("全部")
{
    ui->setupUi(this);

    // 设置分页大小下拉框
    ui->pageSizeCombo->addItems({"5", "10", "20", "50"});
    ui->pageSizeCombo->setCurrentText("10");

    // 设置页码标签默认显示
    ui->pageLabel->setText("第 0 页 / 共 0 页");

    setupTable();
    loadCategories();
    loadBooks();
    updateStatusBar();
}

BookDisplayWidget::~BookDisplayWidget()
{
    delete ui;
}

void BookDisplayWidget::setupTable()
{
    // 设置表头
    QStringList headers = {"ID", "书名", "作者", "ISBN", "分类", "总数", "可借", "状态"};
    m_tableModel->setHorizontalHeaderLabels(headers);

    ui->tableView->setModel(m_tableModel.data());

    // 设置列宽
    ui->tableView->setColumnWidth(0, 50);
    ui->tableView->setColumnWidth(1, 180);
    ui->tableView->setColumnWidth(2, 120);
    ui->tableView->setColumnWidth(3, 130);
    ui->tableView->setColumnWidth(4, 100);
    ui->tableView->setColumnWidth(5, 60);
    ui->tableView->setColumnWidth(6, 60);
    ui->tableView->setColumnWidth(7, 100);

    // 设置选择模式：单选整行
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // 禁止编辑
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 交替行颜色
    ui->tableView->setAlternatingRowColors(true);

    // 隐藏垂直表头
    ui->tableView->verticalHeader()->setVisible(false);
}

void BookDisplayWidget::loadCategories()
{
    ui->categoryCombo->clear();
    ui->categoryCombo->addItem("全部");

    try {
        auto allBooks = m_bookService->getAllBooks(false);
        QSet<QString> categories;

        for (const auto& book : allBooks) {
            QString category = book.getCategory().trimmed();
            if (!category.isEmpty()) {
                categories.insert(category);
            }
        }

        // 排序后添加到下拉框
        QStringList sortedCategories = categories.values();
        std::sort(sortedCategories.begin(), sortedCategories.end());
        ui->categoryCombo->addItems(sortedCategories);

        qDebug() << "加载分类完成，共" << categories.size() << "个分类";

    } catch (const BookServiceException& e) {
        QMessageBox::warning(this, "加载分类失败", e.getMessage());
    }
}

// ==================== 数据加载方法 ====================

void BookDisplayWidget::loadBooks()
{
    try {
        // 第一步：根据关键词获取数据
        if (!m_currentKeyword.trimmed().isEmpty()) {
            m_allBooks = m_bookService->searchBooks(m_currentKeyword, false);
            qDebug() << "搜索关键词:" << m_currentKeyword << "，找到" << m_allBooks.size() << "本书";
        } else {
            m_allBooks = m_bookService->getAllBooks(false);
            qDebug() << "加载所有图书，共" << m_allBooks.size() << "本";
        }

        // 第二步：根据分类筛选（在内存中过滤）
        m_filteredBooks = filterBooks(m_allBooks);
        qDebug() << "分类筛选后剩余" << m_filteredBooks.size() << "本";

        // 重置到第一页
        m_currentPage = 0;
        refreshDisplay();

    } catch (const BookServiceException& e) {
        QMessageBox::critical(this, "加载图书失败", e.getMessage());
        m_allBooks.clear();
        m_filteredBooks.clear();
        refreshDisplay();
    }
}

void BookDisplayWidget::refreshDisplay()
{
    // 获取当前页数据
    m_currentPageBooks = getCurrentPageBooks();

    // 更新表格
    updateTable(m_currentPageBooks);

    // 更新分页控件
    updatePagination();

    // 更新状态栏
    updateStatusBar();
}

// ==================== 数据过滤与分页 ====================

QList<Book> BookDisplayWidget::filterBooks(const QList<Book>& books) const
{
    // 如果分类是 "全部" 或空，不过滤
    if (m_currentCategory.isEmpty() || m_currentCategory == "全部") {
        return books;
    }

    QList<Book> result;
    for (const auto& book : books) {
        if (book.getCategory().trimmed() == m_currentCategory) {
            result.append(book);
        }
    }
    return result;
}

int BookDisplayWidget::getTotalPages() const
{
    if (m_filteredBooks.isEmpty()) {
        return 1;
    }
    return (m_filteredBooks.size() + m_pageSize - 1) / m_pageSize;
}

QList<Book> BookDisplayWidget::getCurrentPageBooks() const
{
    if (m_filteredBooks.isEmpty()) {
        return QList<Book>();
    }

    int totalPages = getTotalPages();
    int currentPage = m_currentPage;

    // 边界检查
    if (currentPage >= totalPages) {
        currentPage = totalPages - 1;
    }
    if (currentPage < 0) {
        currentPage = 0;
    }

    int start = currentPage * m_pageSize;
    int end = qMin(start + m_pageSize, m_filteredBooks.size());

    return m_filteredBooks.mid(start, end - start);
}

// ==================== UI 更新方法 ====================

void BookDisplayWidget::updateTable(const QList<Book>& books)
{
    m_tableModel->removeRows(0, m_tableModel->rowCount());

    for (const auto& book : books) {
        QList<QStandardItem*> row;

        row.append(createItem(QString::number(book.getId())));
        row.append(createItem(book.getTitle()));
        row.append(createItem(book.getAuthor()));
        row.append(createItem(book.getIsbn()));
        row.append(createItem(book.getCategory()));
        row.append(createItem(QString::number(book.getTotalCount())));
        row.append(createItem(QString::number(book.getAvailableCount())));
        row.append(createItem(getBookStatusText(book)));

        // 根据状态设置颜色
        if (book.getAvailableCount() <= 0) {
            row.last()->setForeground(QBrush(Qt::red));
        } else if (book.getAvailableCount() <= book.getTotalCount() / 3) {
            row.last()->setForeground(QBrush(QColor(255, 165, 0))); // 橙色
        } else {
            row.last()->setForeground(QBrush(Qt::darkGreen));
        }

        m_tableModel->appendRow(row);
    }
}

QStandardItem* BookDisplayWidget::createItem(const QString& text) const
{
    auto item = new QStandardItem(text);
    item->setTextAlignment(Qt::AlignCenter);
    item->setEditable(false);
    return item;
}

void BookDisplayWidget::updatePagination()
{
    int totalPages = getTotalPages();
    int currentPage = m_currentPage;

    // 边界检查
    if (currentPage >= totalPages) {
        currentPage = totalPages - 1;
        m_currentPage = currentPage;
    }
    if (currentPage < 0) {
        currentPage = 0;
        m_currentPage = 0;
    }

    // 更新按钮状态
    ui->prevPageBtn->setEnabled(currentPage > 0);
    ui->nextPageBtn->setEnabled(currentPage < totalPages - 1);

    // 更新页码标签
    if (m_filteredBooks.isEmpty()) {
        ui->pageLabel->setText("第 0 页 / 共 0 页");
    } else {
        ui->pageLabel->setText(QString("第 %1 页 / 共 %2 页")
                                   .arg(currentPage + 1)
                                   .arg(totalPages));
    }
}

void BookDisplayWidget::updateStatusBar()
{
    int total = m_filteredBooks.size();
    int start = m_currentPage * m_pageSize + 1;
    int end = qMin(start + m_pageSize - 1, total);

    QString status;
    if (total == 0) {
        status = "共 0 本书";
    } else {
        status = QString("共 %1 本书  |  显示第 %2 - %3 本  |  每页 %4 条")
                     .arg(total)
                     .arg(start)
                     .arg(end)
                     .arg(m_pageSize);

        // 如果有搜索关键词，显示搜索信息
        if (!m_currentKeyword.trimmed().isEmpty()) {
            status += QString("  |  搜索: \"%1\"").arg(m_currentKeyword);
        }
        // 如果有分类筛选，显示分类信息
        if (!m_currentCategory.isEmpty() && m_currentCategory != "全部") {
            status += QString("  |  分类: %1").arg(m_currentCategory);
        }
    }

    ui->statusLabel->setText(status);
}

QString BookDisplayWidget::getBookStatusText(const Book& book) const
{
    if (book.getAvailableCount() <= 0) {
        return "无库存";
    } else if (book.getAvailableCount() <= book.getTotalCount() / 3) {
        return QString("库存紧张(%1)").arg(book.getAvailableCount());
    } else {
        return "可借";
    }
}


void BookDisplayWidget::on_searchBtn_clicked()
{
    m_currentKeyword = ui->searchLineEdit->text().trimmed();
    m_currentPage = 0;
    loadBooks();
}


void BookDisplayWidget::on_resetBtn_clicked()
{
    // 清空搜索框
    ui->searchLineEdit->clear();

    // 重置分类下拉框到 "全部"
    ui->categoryCombo->setCurrentIndex(0);

    // 清空状态变量
    m_currentKeyword.clear();
    m_currentCategory = "全部";
    m_currentPage = 0;

    // 重新加载
    loadBooks();
}


void BookDisplayWidget::on_categoryCombo_currentIndexChanged(int index)
{
    Q_UNUSED(index)

    // 获取当前选中的分类名称
    m_currentCategory = ui->categoryCombo->currentText();

    // 重新加载数据（会触发过滤）
    m_currentPage = 0;
    loadBooks();
}


void BookDisplayWidget::on_prevPageBtn_clicked()
{
    if (m_currentPage > 0) {
        m_currentPage--;
        refreshDisplay();
    }
}


void BookDisplayWidget::on_nextPageBtn_clicked()
{
    if (m_currentPage < getTotalPages() - 1) {
        m_currentPage++;
        refreshDisplay();
    }
}


void BookDisplayWidget::on_pageSizeCombo_currentIndexChanged(int index)
{
    Q_UNUSED(index)

    bool ok;
    int newSize = ui->pageSizeCombo->currentText().toInt(&ok);
    if (ok && newSize > 0 && newSize != m_pageSize) {
        m_pageSize = newSize;
        m_currentPage = 0;
        refreshDisplay();
    }
}

