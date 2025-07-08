#-------------------------------------------------
#
# Project created by QtCreator 2016-03-31T18:23:47
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = camera_simulated
TEMPLATE = lib
CONFIG += plugin

PROJECTDIR=$$PWD/..


DEFINES  += _CRT_SECURE_NO_WARNINGS

INCLUDEPATH += $${PROJECTDIR}/nirs

INCLUDEPATH += "C:\Program Files (x86)\National Instruments\NI-DAQ\DAQmx ANSI C Dev\include"

SOURCES += \
    camera_simulated.cpp \
    $${PROJECTDIR}/nirs/camera.cpp \
    $${PROJECTDIR}/nirs/camerafactory.cpp \
    simulated_camerafactory.cpp

HEADERS  += \
    camera_simulated.h \
    $${PROJECTDIR}/nirs/camera.h \
    $${PROJECTDIR}/nirs/camerafactory.h \
    simulated_camerafactory.h

DISTFILES += \
    simulatedcamerafactory.json

DESTDIR = $$PWD/../../../nirs_deploy/plugins

