QT       += core gui\
            multimedia\
            multimediawidgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    adddevice.cpp \
    aircondition.cpp \
    aironoff.cpp \
    firstwidget.cpp \
    fridge.cpp \
    logwidget.cpp \
    main.cpp \
    mainwidget.cpp \
    regdialog.cpp \
    regwidget.cpp \
    setdialog.cpp \
    include/client.cpp \
    warning.cpp \
    fridgewarning.cpp \
    include/media.c

HEADERS += \
    adddevice.h \
    aircondition.h \
    aironoff.h \
    firstwidget.h \
    fridge.h \
    include/message.h \
    logwidget.h \
    mainwidget.h \
    regdialog.h \
    regwidget.h \
    setdialog.h \
    include/client.h \
    warning.h \
    fridgewarning.h \
    include/media.h

FORMS += \
    adddevice.ui \
    aircondition.ui \
    aironoff.ui \
    firstwidget.ui \
    fridge.ui \
    logwidget.ui \
    mainwidget.ui \
    regdialog.ui \
    regwidget.ui \
    setdialog.ui \
    warning.ui \
    fridgewarning.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
