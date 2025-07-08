#include "config.h"
#include <iostream>
#include "analoginput.h"
#include "daqexception.h"
#include "config.h"
//#include "analogfs.h"

#define DAQmxErrChk(functionCall) if( DAQmxFailed(m_taskError=(functionCall)) ) throw DAQException(m_taskError) ; else

AnalogInput::AnalogInput(int sampling_rate) : p_data_saver_ptr(nullptr)
{
    SAMPLINGRATE = (int) sampling_rate;
    //Try-catch in main code for user warning
    DAQmxErrChk(DAQmxCreateTask("AnalogInput",&p_ai_task_handle));
    DAQmxErrChk(DAQmxCreateAIVoltageChan(p_ai_task_handle,AI_CHANNELS,"",DAQmx_Val_RSE,-10.0,10.0,DAQmx_Val_Volts,nullptr));
    DAQmxErrChk(DAQmxCfgSampClkTiming(p_ai_task_handle,nullptr,SAMPLINGRATE,DAQmx_Val_Rising,DAQmx_Val_ContSamps,(uInt64) SAMPLINGRATE));
    av_ptr = 0;
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
        dataForSaving = new double[N_AI_CHANNELS*SAMPLINGRATE];
        counterSaving = 0;
        try {
            if (hasTriggerIn) {
                DAQmxErrChk(DAQmxCfgDigEdgeStartTrig(p_ai_task_handle, TRIGGER_IN, DAQmx_Val_Rising));
            } else {
                DAQmxErrChk(DAQmxDisableStartTrig(p_ai_task_handle));
            }
            DAQmxErrChk(DAQmxStartTask(p_ai_task_handle));
        } catch(DAQException& e) {
            e.show();
        }

        start(); //Comment this line when debugging the code
    }

}

void AnalogInput::Stop()
{
    // Stop reading first as reading calls could block
    m_mutex.lock();
    if(m_isThreadStarted)
    {
        m_isThreadStarted = false;
        m_mutex.unlock();
        wait();
        av_ptr = 0;
        DAQmxStopTask(p_ai_task_handle);
        delete [] dataForSaving;
    }
    else {
        m_mutex.unlock();
    }
}

void AnalogInput::run()
{
    // Read every 1/20th of a second (for analog viewer) and send to saver every second
    int readPerSeconds = 20;
    int32 num_samp_per_chan = SAMPLINGRATE/readPerSeconds;
    int32 samples_read;
    uInt32 arraySizeInSamps = N_AI_CHANNELS*num_samp_per_chan;
    double* data = new double[arraySizeInSamps];
    bool errorReading = false; //Could be true in trigger input mode when no trigger in detected. Hangs if user press Stop.

    while(true)
    {
        errorReading = false;
        try
        {
            DAQmxErrChk(DAQmxReadAnalogF64(p_ai_task_handle,num_samp_per_chan,5,DAQmx_Val_GroupByChannel,data,arraySizeInSamps,&samples_read,NULL));
        }
        catch(DAQException&)
        {
            errorReading = true;
            //e.show();
        }

        if(p_data_saver_ptr)
        {
            for (int idxChannel = 0; idxChannel < N_AI_CHANNELS; idxChannel++) { //Put back in chunks of 10000 data per channel
                memcpy(&dataForSaving[counterSaving*num_samp_per_chan + SAMPLINGRATE*idxChannel], &data[num_samp_per_chan*idxChannel], sizeof(double)*num_samp_per_chan);
            }
            counterSaving++;
            if (counterSaving == readPerSeconds) {
                p_data_saver_ptr->put((double*) dataForSaving);
                counterSaving = 0;
            }

        }
        // Needs to be fast

        if(av_ptr) { //Analog viewer
            av_ptr->put((double*) data);
        }
		
        m_mutex.lock();

        if(!m_isThreadStarted) {
            m_mutex.unlock();
            if (counterSaving == 0 || errorReading) //Wait until a full second is acquired
                break;
        } else {
            m_mutex.unlock();
        }
    }
    delete [] data;
}

void AnalogInput::SetDataSaver(Float64DataSaver* data_saver_ptr)
{
    p_data_saver_ptr = data_saver_ptr;
}


void AnalogInput::setAnalogViewer(AnalogViewer* ptr)
{
    av_ptr = ptr;
}

// Simple function to set the sampling rate, KP
//void AnalogInput::set_AnalogSamplingRate(int sam_rate)
//{
//    SAMPLINGRATE = sam_rate;
//}
