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
    void resetDataSaver(){p_data_saver_ptr = 0;}
    void setAnalogViewer(AnalogViewer* ptr);

    // add a set_samplingrate function to pass the sampling rate, KP
    //void set_AnalogSamplingRate(int sam_rate);

private:
    TaskHandle p_ai_task_handle;
    Float64DataSaver* p_data_saver_ptr;
    AnalogViewer* av_ptr;

    bool m_isThreadStarted;
    QMutex m_mutex;
    int32 m_taskError;
    double* dataForSaving;
    int counterSaving;

    int SAMPLINGRATE = 10000;

    // Change variable type to allow samplingRate being controlled by the user, KP
    //int SAMPLINGRATE;
};

#endif // ANALOGINPUT_H
