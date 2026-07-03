QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dao/bookdao.cpp \
    dao/borrowrecorddao.cpp \
    dao/readerdao.cpp \
    db/dbconnection.cpp \
    main.cpp \
    model/book.cpp \
    model/borrowrecord.cpp \
    model/reader.cpp \
    service/bookservice.cpp \
    service/borrowrecordservice.cpp \
    service/readerservice.cpp \
    view/bookdisplaywidget.cpp \
    view/mainwindow.cpp \
    view/return.cpp

HEADERS += \
    dao/bookdao.h \
    dao/borrowrecorddao.h \
    dao/readerdao.h \
    db/dbconnection.h \
    model/book.h \
    model/borrowrecord.h \
    model/reader.h \
    service/bookservice.h \
    service/borrowrecordservice.h \
    service/readerservice.h \
    view/bookdisplaywidget.h \
    view/mainwindow.h \
    view/return.h

FORMS += \
    view/bookdisplaywidget.ui \
    view/mainwindow.ui \
    view/return.ui

TRANSLATIONS += \
    LibraryManagement_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

QT += sql