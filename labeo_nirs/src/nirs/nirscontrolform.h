#ifndef NIRSCONTROLFORM_H
#define NIRSCONTROLFORM_H

#include <QMainWindow>
#include <QTimer>
#include "analoginput.h"
#include "analogviewer.h"
#include "float64datasaver.h"
#include "hardwaresettings.h"
#include "NIDAQmx.h"

QT_BEGIN_NAMESPACE
namespace Ui { class NirsControlForm; }
QT_END_NAMESPACE

class NirsControlForm : public QMainWindow
{
    Q_OBJECT

public:
    NirsControlForm(QWidget *parent = nullptr);
    ~NirsControlForm();

private slots:
    void startAcquisition();
    void stopAcquisition();
    void saveData(bool);
    void setSaveDir();
    void analogViewClosed();

private:
    Ui::NirsControlForm *ui;
    AnalogInput* m_analogInput;
    AnalogViewer* m_analogView;
    QString m_saveName;
    QDir m_saveDir;
    bool m_isUserStop;
    QRect m_ROI;
    Float64DataSaver* m_dataSaverAnalogInputs;
    TaskHandle m_taskHandleIllumination;
    TaskHandle m_taskHandleClock;
    int m_error;
    HardwareSettings m_hardwareSettings;
    QTimer* m_viewTimer;


    bool m_bShowCameraROI;
    bool m_bSaveData;
};
#endif // NIRSCONTROLFORM_H
