#include "config.h"
#include <iostream>
#include "analoginput.h"
#include "daqexception.h"
#include "config.h"
//#include "analogfs.h"

#define DAQmxErrChk(functionCall) if( DAQmxFailed(m_taskError=(functionCall)) ) throw DAQException(m_taskError) ; else

AnalogInput::AnalogInput(int sampling_rate) : p_data_saver_ptr(nullptr)
{
    SAMPLINGRATE = sampling_rate;
    DAQmxErrChk(DAQmxCreateTask("AnalogInput", &p_ai_task_handle));
    DAQmxErrChk(DAQmxCreateAIVoltageChan(p_ai_task_handle, AI_CHANNELS, "", DAQmx_Val_RSE, -10.0, 10.0, DAQmx_Val_Volts, nullptr));
    DAQmxErrChk(DAQmxCfgSampClkTiming(p_ai_task_handle, nullptr, SAMPLINGRATE, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 2 * SAMPLINGRATE));
    av_ptr = nullptr;
    m_isThreadStarted = false;
}

AnalogInput::~AnalogInput()
{
    Stop();
    DAQmxClearTask(p_ai_task_handle);
}

void AnalogInput::Start(bool hasTriggerIn)
{
    if (!m_isThreadStarted)
    {
        m_isThreadStarted = true;
        dataForSaving = new double[N_AI_CHANNELS]; // Allocate for 1 sample per channel
        counterSaving = 0;
        bool taskStarted = false;
        try
        {
            if (hasTriggerIn)
            {
                DAQmxErrChk(DAQmxCfgDigEdgeStartTrig(p_ai_task_handle, TRIGGER_IN, DAQmx_Val_Rising));
            }
            else
            {
                DAQmxErrChk(DAQmxDisableStartTrig(p_ai_task_handle));
            }
            DAQmxErrChk(DAQmxStartTask(p_ai_task_handle));
            taskStarted = true;
            int32 taskStatus;
        }
        catch (DAQException& e)
        {
            e.show();
            if (taskStarted) DAQmxStopTask(p_ai_task_handle);
            m_isThreadStarted = false;
            delete[] dataForSaving;
            return;
        }
        start();
    }
}

void AnalogInput::Stop()
{
    m_mutex.lock();
    if (m_isThreadStarted)
    {
        m_isThreadStarted = false;
        m_mutex.unlock();
        wait();
        av_ptr = nullptr;
        DAQmxStopTask(p_ai_task_handle);
        delete[] dataForSaving;
    }
    else
    {
        m_mutex.unlock();
    }
}

void AnalogInput::run()
{
    int readPerSeconds = SAMPLINGRATE; // Match read frequency to sampling rate
    int32 num_samp_per_chan = 1; // Read 1 sample per channel per iteration
    if (num_samp_per_chan == 0) {
        qCritical() << "Error: num_samp_per_chan is 0. Check SAMPLINGRATE (" << SAMPLINGRATE << ") and readPerSeconds (" << readPerSeconds << ")";
        return;
    }
    int32 samples_read;
    uInt32 arraySizeInSamps = N_AI_CHANNELS * num_samp_per_chan;
    double* data = new double[arraySizeInSamps];

    qDebug() << "[AnalogInput] Config: SAMPLINGRATE=" << SAMPLINGRATE << ", num_samp_per_chan=" << num_samp_per_chan << ", arraySizeInSamps=" << arraySizeInSamps;

    while (true)
    {
        try
        {
            DAQmxErrChk(DAQmxReadAnalogF64(p_ai_task_handle, num_samp_per_chan, 5.0, DAQmx_Val_GroupByChannel, data, arraySizeInSamps, &samples_read, NULL));
            qDebug() << "[AnalogInput] Read" << samples_read << "samples at" << QTime::currentTime().toString();
        }
        catch (DAQException& e)
        {
            char errBuff[2048] = {'\0'};
            DAQmxGetErrorString(e.getError(), errBuff, 2048);
            qCritical() << "DAQ Error:" << errBuff;
            break;
        }

        if (p_data_saver_ptr && samples_read > 0)
        {
            for (int idxChannel = 0; idxChannel < N_AI_CHANNELS; ++idxChannel)
            {
                dataForSaving[idxChannel] = data[idxChannel]; // Copy 1 sample per channel
                qDebug() << "[AnalogInput] Channel" << idxChannel << "data[0]:" << data[idxChannel];
            }
            qDebug() << "[AnalogInput] Sending 1 frame to DataSaver";
            p_data_saver_ptr->put(dataForSaving);
        }

        if (av_ptr && samples_read > 0)
        {
            av_ptr->put(data);
        }

        m_mutex.lock();
        if (!m_isThreadStarted)
        {
            m_mutex.unlock();
            break;
        }
        else
        {
            m_mutex.unlock();
        }
    }
    delete[] data;
}

void AnalogInput::SetDataSaver(Float64DataSaver* data_saver_ptr)
{
    p_data_saver_ptr = data_saver_ptr;
}

void AnalogInput::setAnalogViewer(AnalogViewer* ptr)
{
    av_ptr = ptr;
}


