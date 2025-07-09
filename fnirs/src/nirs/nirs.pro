QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++17
CONFIG += file_copies

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += "C:\Program Files (x86)\National Instruments\NI-DAQ\DAQmx ANSI C Dev\include"
LIBS += -L"C:\Program Files (x86)\National Instruments\Shared\ExternalCompilerSupport\C\lib64\msvc" -lNIDAQmx

SOURCES += \
    analoginput.cpp \
    analogviewer.cpp \
    hardwaresettings.cpp \
    main.cpp \
    nirscontrolform.cpp \
    qcustomplot.cpp

HEADERS += \
    analogfs.h \
    analoginput.h \
    analogviewer.h \
    config.h \
    daqexception.h \
    float64datasaver.h \
    hardwaresettings.h \
    nirscontrolform.h \
    qcustomplot.h \
    unsignedshortdatasaver.h

FORMS += \
    nirscontrolform.ui

DESTDIR = $$PWD/../../../nirs_deploy

COPIES += hardware
hardware.files = $$files(../settings/*.ini)
hardware.path = $$DESTDIR

QTDIR = $$[QT_INSTALL_PREFIX]
DEPLOY_TARGET=$$shell_quote($$shell_path($${DESTDIR}/$${TARGET}.exe))
QMAKE_POST_LINK = $$QTDIR/bin/windeployqt $$DEPLOY_TARGET

