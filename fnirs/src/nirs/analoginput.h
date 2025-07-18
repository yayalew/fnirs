#ifndef ANALOGINPUT_H
#define ANALOGINPUT_H

#include <QThread>
#include <QMutex>
#include "NIDAQmx.h"
#include "float64datasaver.h"
#include "analogviewer.h"

class AnalogInput : public QThread
{
    Q_OBJECT
public:
    AnalogInput(int sampling_rate);
    ~AnalogInput();
    void Start(bool hasTriggerIn);
    void Stop();
    void run();
    void SetDataSaver(Float64DataSaver* data_saver_ptr);
    void setAnalogViewer(AnalogViewer* ptr);

private:
    TaskHandle p_ai_task_handle;
    int32 m_taskError;
    int SAMPLINGRATE;
    double* dataForSaving;
    int counterSaving;
    Float64DataSaver* p_data_saver_ptr;
    AnalogViewer* av_ptr;
    bool m_isThreadStarted;
    QMutex m_mutex;
};

#endif // ANALOGINPUT_H


