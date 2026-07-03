#ifndef RETURN_H
#define RETURN_H

#include <QDialog>

namespace Ui {
class ReturnAndBorrow;
}

class ReturnAndBorrow : public QDialog
{
    Q_OBJECT

public:
    explicit ReturnAndBorrow(QWidget *parent = nullptr);
    ~ReturnAndBorrow();

private:
    Ui::ReturnAndBorrow *ui;
};

#endif // RETURN_H
