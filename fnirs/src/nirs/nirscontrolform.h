#ifndef NIRSCONTROLFORM_H
#define NIRSCONTROLFORM_H

#include <QMainWindow>
#include <QDir>
#include <QTimer>
#include <QVector>
#include "analogviewer.h"
#include "analoginput.h"
#include "float64datasaver.h"
#include "NIDAQmx.h"
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
    void updateIllumination();
    void analogViewClosed();

private:
    Ui::NirsControlForm *ui;
    TaskHandle m_taskHandleIllumination;
    AnalogViewer* m_analogView;
    AnalogInput* m_analogInput;
    bool m_bSaveData;
    Float64DataSaver* m_dataSaverAnalogInputs;
    QDir m_saveDir;
    QString m_saveName;
    QTimer* m_viewTimer;
    int m_currentEmitterIndex;
    QVector<QVector<uInt8>> m_illumStates;
    int32 m_error;
    HardwareSettings m_hardwareSettings;
};

#endif // NIRSCONTROLFORM_H
