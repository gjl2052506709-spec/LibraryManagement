#ifndef BOOKDISPLAYWIDGET_H
#define BOOKDISPLAYWIDGET_H

#include <QWidget>
#include<QStandardItemModel>
#include<QMessageBox>

#include"service/bookservice.h"
#include"returndisplay.h"
#include"borrow.h"

namespace Ui {
class BookDisplayWidget;
}

class BookDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BookDisplayWidget(QWidget *parent = nullptr);
    ~BookDisplayWidget();

private slots:
    void on_searchBtn_clicked();

    void on_resetBtn_clicked();

    void on_categoryCombo_currentIndexChanged(int index);

    void on_prevPageBtn_clicked();

    void on_nextPageBtn_clicked();

    void on_pageSizeCombo_currentIndexChanged(int index);

    void on_borrowBtn_clicked();

    void on_returnBtn_clicked();

private:
    // 核心方法
    void loadBooks();
    void refreshDisplay();

    // 辅助方法
    void setupTable();
    void loadCategories();                    // 从数据库加载分类列表
    void updateTable(const QList<Book>& books);
    void updatePagination();
    void updateStatusBar();

    // 数据过滤
    QList<Book> filterBooks(const QList<Book>& books) const;  // 按分类过滤
    QList<Book> getCurrentPageBooks() const;
    int getTotalPages() const;

    // 工具方法
    QString getBookStatusText(const Book& book) const;
    QStandardItem* createItem(const QString& text) const;

private:
    Ui::BookDisplayWidget *ui;

    Borrow* m_borrow;
    ReturnDisplay* m_return;

    QScopedPointer<BookService> m_bookService;
    QScopedPointer<QStandardItemModel> m_tableModel;




    QList<Book> m_allBooks;          // 从数据库查询到的原始数据
    QList<Book> m_filteredBooks;     // 经过分类过滤后的数据
    QList<Book> m_currentPageBooks;  // 当前页显示的数据

    int m_currentPage;
    int m_pageSize;
    QString m_currentKeyword;
    QString m_currentCategory;

};

#endif // BOOKDISPLAYWIDGET_H
