#ifndef NIRSCONTROLFORM_H
#define NIRSCONTROLFORM_H

#include <QMainWindow>
#include <QDir>
#include <QTimer>
#include "analogviewer.h"
#include "analoginput.h"
#include "float64datasaver.h"

namespace Ui {
class NirsControlForm;
}

class NirsControlForm : public QMainWindow
{
    Q_OBJECT
public:
    explicit NirsControlForm(QWidget *parent = nullptr);
    ~NirsControlForm();

private slots:
    void startAcquisition();
    void stopAcquisition();
    void saveData(bool flag);
    void setSaveDir();
    void updateDigitalOutput();

private:
    Ui::NirsControlForm *ui;
    TaskHandle m_taskHandleDigitalOut;
    TaskHandle m_taskHandleClock;
    AnalogViewer* m_analogView;
    AnalogInput* m_analogInput;
    bool m_bSaveData;
    Float64DataSaver* m_dataSaverAnalogInputs;
    QDir m_saveDir;
    QString m_saveName;
    QTimer* m_viewTimer;
    int m_currentEmitterIndex;
    int32 m_error;
    // Assume m_hardwareSettings is defined elsewhere
};

#endif // NIRSCONTROLFORM_H