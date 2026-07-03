#ifndef BORROW_H
#define BORROW_H

#include <QDialog>
#include<QMessageBox>
#include <QInputDialog>

#include"service/borrowrecordservice.h"
#include"service/bookservice.h"

namespace Ui {
class Borrow;
}

class Borrow : public QDialog
{
    Q_OBJECT

public:
    explicit Borrow(QWidget *parent = nullptr);
    ~Borrow();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    // 核心方法
    bool validateInput(int& readerId, int& bookId, QString& bookTitle);
    bool findBookByTitle(const QString& title, int& bookId);
    void showBorrowResult(const BorrowRecordService::BorrowResult& result);

    // UI初始化
    void setupUI();

private:
    Ui::Borrow *ui;

    QScopedPointer<BorrowRecordService> m_borrowService;
    QScopedPointer<BookService> m_bookService;
};

#endif // BORROW_H
