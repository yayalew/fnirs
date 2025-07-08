#include "camera_pco.h"
#include <QElapsedTimer>
#include <QtDebug>
#include <synchapi.h>

#include "PCO_err.h"
#include "sc2_SDKStructures.h"
#include "SC2_SDKAddendum.h"
#include "SC2_CamExport.h"

volatile int is_first_frame_finished = 0;
bool Camera_PCO::m_isSDKClosed = true;

Camera_PCO::Camera_PCO() : m_counterMissedTrigger(0), m_counterMissedImage(0), m_counterImage(0), m_counterImageWorkaround(0), m_lastPixelClock(0), m_isCameraDiscovered(false),
    m_acceptedDiffPixelClock(), m_cameraHandle(0), m_imageBuffer(nullptr), m_frameCount(0), m_previousFrameCount(0), m_camerasIds(), m_theCameraID(), m_skipHalfImageIdx(-1)
{
    m_isCameraOpened = false;
    m_sensorWidth = 2560;
    m_sensorHeight = 2160;
    m_n_buffer = 4;
    m_pixelMaxValue = 65535;
    m_pixelSaturationValue = 65535;
    m_pixelTooLowValue = 100;
    m_histoBinSize = 256;
    m_histoBinOffset = 0;
    m_ROI=QRect(1,1,2560,2160);
}

Camera_PCO::~Camera_PCO()
{
    if(m_currentCopiedBuffer!=0) {
        delete [] m_currentCopiedBuffer;
        m_currentCopiedBuffer = 0;
    }
}

QVector<int> Camera_PCO::GetAvailableFrameRates()
{
    QVector<int> fr = {1, 2, 5, 10, 20, 25, 30, 60, 120, 200, 240, 480, 600, 1000};
    return fr;
}

char* Camera_PCO::Discover()
{
    return NULL; //Not implemented
}

void Camera_PCO::SetCameraID(const char* camID)
{
}

void Camera_PCO::Open()
{
    int iRet;
    WORD RecordingState;
    if (m_isCameraOpened) {
        Stop();
        Close();
    }
    iRet = PCO_OpenCamera(&m_cameraHandle, 0);
    if (iRet != PCO_NOERROR)
    {
       std::cerr << "Did not find any cameras!" << std::endl;
       exit(-1);
    }
    iRet = PCO_GetRecordingState(m_cameraHandle, &RecordingState);
    if(RecordingState)
    {
       iRet = PCO_SetRecordingState(m_cameraHandle, 0);
    }
    iRet = PCO_ResetSettingsToDefault(m_cameraHandle);

    // Camera is opened but not armed yet
    m_isCameraOpened = true;

    return;

}

void Camera_PCO::Close()
{
    int iRet;
    // Free buffers
    for(unsigned int ib=0;ib<m_n_buffer;ib++)
    {
        iRet = PCO_FreeBuffer(m_cameraHandle, m_buff_num[ib]);
    }
    delete [] m_buff_num;
    delete [] m_buff_event;
    delete [] m_buff_addr;

    iRet = PCO_CloseCamera(m_cameraHandle);
    m_isCameraOpened=false;
    m_isCameraDiscovered = false;

    delete [] m_currentCopiedBuffer;
    m_currentCopiedBuffer = 0;
}

void Camera_PCO::Start()
{
    Camera::Start();
}

void Camera_PCO::Stop()
{
    int iRet;
    try {
        if (!m_isCameraOpened) {
            return;
        }
        Camera::Stop();
        QThread::msleep(10);

        iRet = PCO_CancelImages(m_cameraHandle);
        iRet = PCO_SetRecordingState(m_cameraHandle, 0);


        // Stop the camera.
        PCO_CloseCamera(m_cameraHandle);

        m_cameraHandle = 0;
        QThread::msleep(10);

        m_isCameraOpened = false;

        if (m_counterImage > 0) {
            qInfo() << QString("Block ID: %1 Missed trigger: %2 Missed image: %3").arg(m_counterImage).arg(m_counterMissedTrigger).arg(m_counterMissedImage);
        }
    } catch (...) {
        qDebug() << "Error closing the camera...";
    }
}

double Camera_PCO::GetLostRatio()
{
    return (double)(m_counterMissedTrigger + m_counterMissedImage) / (double)(m_counterImage + m_counterMissedTrigger + m_counterMissedImage);
}

void Camera_PCO::Configure()
{
    int iRet;
    if (!m_isCameraOpened) {
        Open();
    }
    // Set ROI
    iRet = PCO_SetROI(m_cameraHandle,m_ROI.x(), m_ROI.y(), m_ROI.x()+m_ROI.width(), m_ROI.y()+m_ROI.height());
    WORD frameStatus=0;
    WORD frameRateMode=0x0000;
    DWORD frameRate=1000;
    DWORD frameRateExposure=10;
    iRet = PCO_SetFrameRate(m_cameraHandle,&frameStatus,frameRateMode,&frameRate,&frameRateExposure);
    std::cerr << iRet << std::endl;
    std::cerr << frameStatus << std::endl;
    std::cerr << frameRateExposure << std::endl;
    iRet = PCO_ArmCamera(m_cameraHandle);
    // Allocate a few buffers for continuous acquisitions
    WORD XResAct, YResAct, XResMax, YResMax;
    DWORD bufsize;
    iRet = PCO_GetSizes(m_cameraHandle, &XResAct, &YResAct, &XResMax, &YResMax);
    bufsize=XResAct*YResAct*sizeof(WORD);
    m_buff_event = new HANDLE[m_n_buffer];
    m_buff_num = new short[m_n_buffer];
    m_buff_addr= new WORD*[m_n_buffer];
    for(int b=0;b<m_n_buffer;b++)
    {
        m_buff_event[b] = NULL;
        m_buff_num[b] = -1;
        m_buff_addr[b]=NULL;
        iRet = PCO_AllocateBuffer(m_cameraHandle, &m_buff_num[b], bufsize, &m_buff_addr[b], &m_buff_event[b]);
    }

    iRet = PCO_SetImageParameters(m_cameraHandle, XResAct, YResAct,IMAGEPARAMETERS_READ_WHILE_RECORDING,NULL,0);
    iRet = PCO_SetRecordingState(m_cameraHandle, 1);

    for(int b=0;b<m_n_buffer;b++)
    {
        iRet = PCO_AddBufferEx(m_cameraHandle,0,0, m_buff_num[b], XResAct, YResAct, 16);
    }

    //initialize frame variables
    m_frameCount = 0;


    // Set hardware trigger
    // iRet = PCO_SetTriggerMode(m_cameraHandle,0x0004);

    m_counterImage = 0;
    m_counterMissedTrigger = 0;
    m_counterMissedImage = 0;
    m_counterImageWorkaround = 0;
    m_lastPixelClock = 0;
    m_currentBufferIndex = 0;
    m_previousFrameCount = 0;

    GetSize(m_sizeX, m_sizeY);
    if(m_currentCopiedBuffer != 0) {
        delete [] m_currentCopiedBuffer;
    }
    m_currentCopiedBuffer = new unsigned short[(m_sizeX * m_sizeY) + 12];

}

void Camera_PCO::GetSize(int &nx, int &ny)
{
    int iRet;
    unsigned short s_nx,s_ny,s_max_x,s_max_y;
    iRet=PCO_GetSizes(m_cameraHandle,&s_nx,&s_ny,&s_max_x,&s_max_y);
    if(iRet != 0)
    {
        std::cerr << "Problem with PCO_GetSizes" << std::endl;
    }
    nx=s_nx;
    ny=s_ny;
    return;
}

void Camera_PCO::SetExposureTime(double exposure_time_ms)
{
    // Set the exposure
    m_exposureTime = exposure_time_ms;
    DWORD exposure = (exposure_time_ms*1000.0);
    int iRet = PCO_SetDelayExposureTime(m_cameraHandle,10,0,2,2);
    char msg[256];
    PCO_GetErrorTextSDK(iRet,msg,256);
    std::cerr << msg << std::endl;
}

void Camera_PCO::SetBinning(int newBinning)
{
    if(newBinning == 1 || newBinning == 2 || newBinning == 4 || newBinning == 8 )
         m_binning = newBinning;
}

void Camera_PCO::SetFrameRate(double newFrameRate)
{
    m_frameRate = newFrameRate;
}

double Camera_PCO::GetRollingDuration()
{
   return 18.4 * (double)m_ROI.height() / 1024.0;
}

void Camera_PCO::GetMaxROI(QRect &roi)
{
    int m_max_roi_size_x;
    int m_max_roi_size_y;
    if (m_frameRate > 600) {
         m_max_roi_size_x = 32;
         m_max_roi_size_y = 32;
     } else if (m_frameRate > 480) {
         m_max_roi_size_x = 64;
         m_max_roi_size_y = 64;
     } else if (m_frameRate > 375) {
         m_max_roi_size_x = 96;
         m_max_roi_size_y = 96;
     } else if (m_frameRate > 300) {
         m_max_roi_size_x = 128;
         m_max_roi_size_y = 128;
     } else if (m_frameRate > 250) {
         m_max_roi_size_x = 160;
         m_max_roi_size_y = 160;
     } else if (m_frameRate > 200) {
         m_max_roi_size_x = 192;
         m_max_roi_size_y = 192;
     } else if (m_frameRate > 130) {
         m_max_roi_size_x = 1024;
         m_max_roi_size_y = 256;
     } else if (m_frameRate > 100) {
         m_max_roi_size_x = 384;
         m_max_roi_size_y = 384;
     } else if (m_frameRate > 82) {
         m_max_roi_size_x = 512;
         m_max_roi_size_y = 512;
     } else if (m_frameRate > 70) {
         m_max_roi_size_x = 640;
         m_max_roi_size_y = 640;
     } else if (m_frameRate > 60) {
         m_max_roi_size_x = 768;
         m_max_roi_size_y = 768;
     } else if (m_frameRate > 50) {
         m_max_roi_size_x = 896;
         m_max_roi_size_y = 896;
     } else {
         m_max_roi_size_x = 1024;
         m_max_roi_size_y = 1024;
     }

    // Maximum ROI accessible given frame rate.
    roi = QRect(512 - m_max_roi_size_x / 2, 512 - m_max_roi_size_y / 2, m_max_roi_size_x, m_max_roi_size_y);
    // Center ROI on the sensor
    roi.moveLeft((m_sensorWidth - m_max_roi_size_x) / 2);
    roi.moveTop((m_sensorHeight - m_max_roi_size_y) / 2);
}

void Camera_PCO::VerifyROI(QRect &roi)
{
    QRect maxROI;
    GetMaxROI(maxROI);
    int maxWidth = maxROI.width();
    int maxHeight = maxROI.height();

    if (roi.x() % 32 != 0) { //Offset x must be modulo 32
        roi.translate(-roi.x() % 32, 0);
    }
    if (roi.width() % 32 != 0) { //Width must be modulo 32
        roi.setWidth(roi.width() - roi.width() % 32 + 32);
    }

    //Width has no importance on the sCMOS
    if (roi.width() > maxWidth) {
        roi.setWidth(maxWidth);
    }

    if (roi.height() % 8 != 0) { //Height must be modulo 8
        roi.setHeight(roi.height() - roi.height() % 8 + 8);
    }
    if (roi.height() > maxHeight) { //Height must be smaller than p_max_size_y
        roi.setHeight(maxHeight);
    }

    if (roi.height() < 8) {
        roi.setHeight(8);
    }
    if (roi.width() < 32) {
        roi.setWidth(32);
    }
}

void Camera_PCO::VerifyExposureTime(double &exposure)
{
    double transfer_buffer_ms;

    //Transfer time depends only on height
    double extraTime = 0.2 * (1024.0 - (double)m_ROI.height()) / 1024.0; // Under 512, add up to 0.2 ms.
    transfer_buffer_ms = 18.6 * (double)m_ROI.height() / 1024.0 + 0.4 + extraTime;
    if (!m_isBulbMode) { //Transfer occurs during the next image acquisition
        transfer_buffer_ms = 0.3;
    }
    double max_exposure = 1.0 / m_frameRate * 1000.0 - transfer_buffer_ms;
    max_exposure = qRound(max_exposure * 10000.0) / 10000.0; //Round value to 4 decimal places

    double minExposure = 0.01;

    if(exposure > max_exposure){
        exposure = max_exposure;
    }
    if(exposure < minExposure && minExposure <= max_exposure) {
        exposure = minExposure;
    }
}

double Camera_PCO::GetMaxExposureWithoutCrosstalk()
{
    double tReset = 18.4 * m_ROI.height() / 1024.0;
    double maxExp = 1000.0 / m_frameRate - tReset;
    return maxExp;
}

double Camera_PCO::GetMaxExposureWhileFullyIlluminated()
{
    double tReset = 18.4 * m_ROI.height() / 1024.0;
    double maxExp = 1000.0 / m_frameRate - tReset;
    maxExp -= 0.2;
    return maxExp;
}

int Camera_PCO::GetNextBuffer()
{
    int iRet;
    DWORD waitstat;
    DWORD StatusDll,StatusDrv;
    static int current_pos = 0;
    int n_images=0;
    waitstat=WaitForMultipleObjects(m_n_buffer,m_buff_event,FALSE,5000);
    if(waitstat==WAIT_TIMEOUT)
    {
        std::cerr << "failed" << std::endl;
        exit(-1);
    }

    // WaitForMultipleObjects might return with 2 or more events set, so all buffers must be checked
    for(int b=0;b<m_n_buffer;b++)
    {
        waitstat=WaitForSingleObject(m_buff_event[current_pos],0);
        if(waitstat==WAIT_OBJECT_0)
        {
            ResetEvent(m_buff_event[current_pos]);
            iRet = PCO_GetBufferStatus(m_cameraHandle,m_buff_num[current_pos],&StatusDll,&StatusDrv);

            //!!! IMPORTANT StatusDrv must always be checked for errors
            if(StatusDrv==PCO_NOERROR)
            {
                memcpy(&m_currentCopiedBuffer[12], m_buff_addr[current_pos], (size_t) m_ROI.width() * m_ROI.height() * sizeof(unsigned short));
                iRet = PCO_AddBufferEx(m_cameraHandle,0,0, m_buff_num[current_pos], m_sizeX, m_sizeY, 16);
                m_currentBufferIndex++;
                n_images++;
            }
            else
            {
                break;
            }
        }
        else
            break;
        current_pos = (current_pos++)%m_n_buffer;
    }
    return n_images;
}

int Camera_PCO::close_sdk_dll(void) {
    int ret = 0;
    return ret;
}

void Camera_PCO::configOnDemand()
{
}

unsigned short* Camera_PCO::getSingleFrame()
{
    return &m_currentCopiedBuffer[12];
}
