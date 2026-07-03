#ifndef RETURNDISPLAY_H
#define RETURNDISPLAY_H

#include <QDialog>
#include<QMessageBox>

#include "ui_returndisplay.h"
#include"service/borrowrecordservice.h"
#include"service/bookservice.h"

namespace Ui {
class ReturnDisplay;
}

class ReturnDisplay : public QDialog
{
    Q_OBJECT

public:
    explicit ReturnDisplay(QWidget *parent = nullptr);
    ~ReturnDisplay();

private slots:
    void on_searchBtn_clicked();
    void on_returnBookBtn_clicked();
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_listWidget_itemSelectionChanged();  // 新增：选择变化时更新按钮状态

private:
    // 核心方法
    bool validateReaderId(int& readerId);
    void loadBorrowRecords(int readerId);
    void refreshList();
    void clearList();

    // 还书操作
    void returnSelectedBook();

    // 工具方法
    QString formatRecordDisplay(const BorrowRecord& record) const;
    int getSelectedRecordId() const;
    void updateReturnButtonState();

    // UI初始化
    void setupUI();

private:
    Ui::ReturnDisplay *ui;

    QScopedPointer<BorrowRecordService> m_borrowService;
    QScopedPointer<BookService> m_bookService;

    QList<BorrowRecord> m_currentRecords;  // 当前显示的借阅记录
    int m_currentReaderId;                 // 当前查询的读者ID
};

#endif // RETURNDISPLAY_H
