#include <QApplication>
#include <QLocale>
#include <QTranslator>


#include "db/DBConnection.h"
#include "view/mainwindow.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "LibraryManagement_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    // 获取数据库连接单例
    auto& db = DBConnection::getInstance();

    // 连接数据库（根据实际情况修改参数）
    if (!db.connect("localhost", "library_db", "root", "gjl123456", 3306)) {
        // 连接失败处理
        qCritical() << "无法连接到数据库，程序将退出";
        return -1;
    }

    // 创建并显示主窗口
    MainWindow mainWindow;
    mainWindow.show();

    return QApplication::exec();
}
