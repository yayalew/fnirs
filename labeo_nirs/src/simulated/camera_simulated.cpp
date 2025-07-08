#include "camera_simulated.h"

Camera_simulated::Camera_simulated()
{
    m_sensorWidth = 1024;
    m_sensorHeight = 1024;
    m_sizeX=m_ROI.width()/m_binning;
    m_sizeY=m_ROI.height()/m_binning;

    m_pixelMaxValue = 65535;
    m_pixelSaturationValue = 54000;
    m_pixelTooLowValue = 1500;
    m_histoBinSize = 252;
    m_histoBinOffset = -15;
}

Camera_simulated::~Camera_simulated()
{

}

QVector<int> Camera_simulated::GetAvailableFrameRates()
{
    QVector<int> fr = {1, 2, 5, 10, 20, 25, 30, 60, 120, 240, 480, 600};
    return fr;
}

void Camera_simulated::Open()
{

}

void Camera_simulated::Close()
{

}

void Camera_simulated::GetMaxROI(QRect &roi)
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
         m_max_roi_size_x = 256;
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
    roi = QRect(512-m_max_roi_size_x/2,512-m_max_roi_size_y/2,m_max_roi_size_x,m_max_roi_size_y);
    // Center ROI on the sensor
    roi.moveLeft((m_sensorWidth-m_max_roi_size_x)/2);
    roi.moveTop((m_sensorHeight-m_max_roi_size_y)/2);
}

void Camera_simulated::VerifyROI(QRect &roi)
{
    QRect maxROI;
    GetMaxROI(maxROI);
    //int maxWidth = maxROI.width();
    int maxHeight = maxROI.height();

    if (roi.x()%32 != 0) { //Offset x must be modulo 32
        roi.translate(-roi.x()%32, 0);
    }
    if (roi.width()%32 != 0) { //Width must be modulo 32
        roi.setWidth(roi.width() - roi.width()%32 + 32);
    }

    //Width has no importance on the sCMOS
    if (roi.width() > 1024) {
        roi.setWidth(1024);
    }

    if (roi.height()%8 != 0) { //Height must be modulo 8
        roi.setHeight(roi.height() - roi.height()%8 + 8);
    }
    if (roi.height() > maxHeight) { //Height must be smaller than p_max_size_y
        roi.setHeight(maxHeight);
    }
}

void Camera_simulated::VerifyExposureTime(double &exposure)
{
    double transfer_buffer_ms;

    //Transfer time depends only on height
    double extraTime = 0.2 * (1024.0-(double)m_ROI.height()) / 1024.0; // Under 512, add up to 0.2 ms.
    transfer_buffer_ms = 18.6 * (double)m_ROI.height() / 1024.0 + 0.4 + extraTime;
    if (!m_isBulbMode) { //Transfer occurs during the next image acquisition
        transfer_buffer_ms = 0.0;
    }
    double max_exposure = 1.0/m_frameRate*1000.0-transfer_buffer_ms;
    max_exposure = qRound(max_exposure*10000.0)/10000.0; //Round value to 4 decimal places

    double minExposure = 0.001;

    if(exposure > max_exposure) {
        exposure = max_exposure;
    }
    if(exposure < minExposure && minExposure <= max_exposure) {
        exposure = minExposure;
    }
}

double Camera_simulated::GetMaxExposureWithoutCrosstalk()
{
    double tReset = 18.4 * m_ROI.height() / 1024.0;
    double maxExp = 1000.0/m_frameRate - tReset;
    return maxExp;
}

double Camera_simulated::GetMaxExposureWhileFullyIlluminated()
{
    double tReset = 18.4 * m_ROI.height() / 1024.0;
    double maxExp = 1000.0/m_frameRate - tReset;
    return maxExp;
}

void Camera_simulated::Configure()
{
    int desiredFPS = 30;
    if (m_frameRate > 100) {
        mNChunkSize = m_frameRate/desiredFPS;
    } else {
        mNChunkSize = 1;
    }
    if(m_currentCopiedBuffer!=0) free(m_currentCopiedBuffer);
    GetSize(m_sizeX,m_sizeY);
    m_currentCopiedBuffer = (unsigned short*) malloc(mNChunkSize*((m_sizeX*m_sizeY)*sizeof(unsigned short)+3*sizeof(__int64)));
}

void Camera_simulated::GetSize(int &nx, int &ny)
{
    nx=m_ROI.width()/m_binning;
    ny=m_ROI.height()/m_binning;
}

void Camera_simulated::SetExposureTime(double exposure_time_ms)
{
    m_exposureTime = exposure_time_ms;
}

void Camera_simulated::SetFrameRate(double newFrameRate)
{
    m_frameRate = newFrameRate;
}

double Camera_simulated::GetRollingDuration()
{
   return 18.4 * (double)m_ROI.height() / 1024.0;
}

void Camera_simulated::SetBinning(int binning)
{
    if(binning == 1 || binning == 2 || binning == 4 || binning == 8 )
    {
        m_binning = binning;
    }
}

int Camera_simulated::GetNextBuffer()
{
    int frameSize = m_sizeX*m_sizeY + 12;
    for (int iC = 0; iC < mNChunkSize; iC++) {
        int a = 0;

        for(int j=0;j<m_sizeY;j++)
        {
            for(int i=0;i<m_sizeX;i++)
            {
                a=rand()%128;
                //if (m_currentBufferIndex%2 == 1) a-=1000; //Debug
                m_currentCopiedBuffer[iC*frameSize + j*m_sizeX+i+12]=((unsigned short) ((512.0*sin((i)/128.0)+3500+a)));
                if(m_currentCopiedBuffer[iC*frameSize + j*m_sizeX+i+12]>4095) m_currentCopiedBuffer[iC*frameSize + j*m_sizeX+i+12]=4095;
            }
        }
        m_currentBufferIndex++;

        unsigned long long tempCounterImage = m_currentBufferIndex;
        unsigned long long tempCounterMissedTrigger = 0;
        memcpy(&m_currentCopiedBuffer[iC*frameSize],&tempCounterImage,sizeof(unsigned long long));
        memcpy(&m_currentCopiedBuffer[iC*frameSize + 4],&tempCounterMissedTrigger,sizeof(unsigned long long));

         msleep(100);
    }
    return mNChunkSize;
}

 unsigned short* Camera_simulated::getSingleFrame()
 {
     int a = 0;
     m_currentCopiedBuffer[12]=1;
     for(int j=0;j<m_sizeY;j++)
     {
         for(int i=0;i<m_sizeX;i++)
         {
             a=rand()%128;
             m_currentCopiedBuffer[j*m_sizeX+i+12]=((unsigned short) ((512.0*sin((i)/128.0)+3500+a)));
             if(m_currentCopiedBuffer[j*m_sizeX+i+12]>4095) m_currentCopiedBuffer[j*m_sizeX+i+12]=4095;
         }
     }
     return &m_currentCopiedBuffer[12];
 }
