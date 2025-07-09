#ifndef HARDWARESETTINGS_H
#define HARDWARESETTINGS_H

#include <QObject>
#include <QSettings>


class HardwareSettings : public QObject
{
    Q_OBJECT
public:
    struct AIDef
    {
        unsigned int ai_num;
        QString tag;
        QString name;
    };
    struct ColorDef
    {
        unsigned int color_num;
        QString name;
        QString icon;
    };

    explicit HardwareSettings(QObject *parent = nullptr);
    virtual ~HardwareSettings();
    QString GetCameraType();
    QString GetOptoGenAddOn() {return m_ogAddOn;};
    unsigned int GetNAI() {return m_ai.size();}
    AIDef GetAI(unsigned int n){return m_ai.at(n);};
    unsigned int GetNColors() {return m_colors.size();}
    ColorDef GetColor(unsigned int n){return m_colors.at(n);};
    bool GetNoFluoExp(){return m_noFluoExp;}
    bool GetNoSpeckleExp(){return m_noSpeckleExp;}

private:
    QSettings* m_hardwareSettings;
    QVector<AIDef> m_ai;
    QVector<ColorDef> m_colors;
    bool m_noFluoExp;
    bool m_noSpeckleExp;
    QString m_ogAddOn;
};

#endif // HARDWARESETTINGS_H
