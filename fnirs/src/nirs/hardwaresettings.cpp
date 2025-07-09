#include "hardwaresettings.h"
#include <QApplication>
#include <QFile>
#include <QMessageBox>

HardwareSettings::HardwareSettings(QObject *parent)
    : QObject{parent}, m_noFluoExp(false), m_noSpeckleExp(false)
{
    QString m_sSettingsFile = QApplication::applicationDirPath() + "/hardwaresettings.ini";
    QFile file(m_sSettingsFile);
    m_hardwareSettings = new QSettings(m_sSettingsFile, QSettings::IniFormat);
    if(file.exists()==true)
    {
        m_hardwareSettings = new QSettings(m_sSettingsFile, QSettings::IniFormat);
        // Read AI info
        for(unsigned int i=0;i<15;i++)
        {
          QStringList temp_ai = m_hardwareSettings->value(QString("ai%1").arg(i),"").toStringList();
          if(!temp_ai.isEmpty())
          {
            AIDef tmp;
            tmp.ai_num=i;
            tmp.tag=temp_ai.at(0);
            tmp.name=temp_ai.at(1);
            m_ai.append(tmp);
          }
        }

    }
    else
    {
        QString message("No Hardware settings file found.");
        QMessageBox msg;
        msg.setWindowTitle("Error");
        msg.setText(QString("%1").arg(message));
        msg.exec();
        qCritical() << message;
        exit(-1);
    }
}

HardwareSettings::~HardwareSettings()
{
    delete m_hardwareSettings;
}

QString HardwareSettings::GetCameraType()
{
    QString camera_type = m_hardwareSettings->value("CameraType", "").toString();
    return camera_type;
}
