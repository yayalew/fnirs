#-------------------------------------------------
#
# Project created by QtCreator 2016-03-31T18:23:47
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = camera_pco
TEMPLATE = lib
CONFIG += plugin file_copies

PROJECTDIR=$$PWD/..


DEFINES  += _CRT_SECURE_NO_WARNINGS

INCLUDEPATH += $${PROJECTDIR}/nirs

INCLUDEPATH += "C:\Users\Frederic Lesage\AppData\Roaming\PCO Digital Camera Toolbox\pco.sdk\include"
LIBS += -L"C:\Users\Frederic Lesage\AppData\Roaming\PCO Digital Camera Toolbox\pco.sdk\lib64" -lSC2_Cam



SOURCES += \
    camera_pco.cpp \
    pco_camerafactory.cpp \
    $${PROJECTDIR}/nirs/camerafactory.cpp \
    $${PROJECTDIR}/nirs/camera.cpp

HEADERS  += \
    camera_pco.h \
    pco_camerafactory.h \
    $${PROJECTDIR}/nirs/camerafactory.h \
    $${PROJECTDIR}/nirs/camera.h

DISTFILES += \
    pcocamerafactory.json


DESTDIR = $$PWD/../../../nirs_deploy/plugins

COPIES += pcodll
pcodll.files = $$files("C:\Users\Frederic Lesage\AppData\Roaming\PCO Digital Camera Toolbox\pco.sdk\bin64\*.dll")
pcodll.path = $$DESTDIR/..
