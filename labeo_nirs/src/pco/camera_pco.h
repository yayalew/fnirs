#ifndef CAMERA_PCO_H
#define CAMERA_PCO_H

#define _AMD64_

#include <QThread>
#include <QMutex>
#include <windows.h>
#include "camera.h"
#include "PCO_err.h"

class Camera_PCO : public Camera
{
    Q_OBJECT
public:
    Camera_PCO();
    virtual ~Camera_PCO();
    char* Discover();
    void SetCameraID(const char* camID); //For multiple cameras
    void Open();
    void Close();
    void Start();
    void Stop();
    double GetLostRatio();
    void Configure();
    void GetSize(int &nx, int &ny);
    void SetExposureTime(double exposure_time_ms);
    void SetBinning(int newBinning);
    int GetMaximumBinning() {return 8;}
    void SetFrameRate(double frameRate);
    double GetRollingDuration();
    int close_sdk_dll();
    unsigned short *getSingleFrame();
    void configOnDemand();
    void SetSkipHalfImage(int idx) {m_skipHalfImageIdx = idx;}
    QVector<int> GetAvailableFrameRates();
    bool AllowsBulbMode() {return true;}
    bool AllowsMultiCam() {return false;} //TODO
    void VerifyROI(QRect &roi);
    void GetMaxROI(QRect &roi);
    void VerifyExposureTime(double &exposure);
    double GetMaxExposureWithoutCrosstalk();
    double GetMaxExposureWhileFullyIlluminated();
protected:
    int GetNextBuffer();
private:
    //Counters to make sure everything is ok
    quint64 m_counterMissedTrigger; //Count from the camera, issue with exposure time if not 0
    quint64 m_counterMissedImage; //Count between 2 images, issue with GigE connection if not 0
    quint64 m_counterImage;
    quint64 m_counterImageWorkaround; //At high frame rate, counter image is sometimes the counter from the next image
    qint64 m_lastPixelClock = 0;
    bool m_isCameraDiscovered = false;
    quint64 m_acceptedDiffPixelClock[6];

    HANDLE m_cameraHandle = 0;

    //initialize frame variables
    unsigned short *m_imageBuffer = 0;
    int m_frameCount = 0;
    int m_previousFrameCount = 0;
    char m_camerasIds[1024];
    char m_theCameraID[1024];

    static bool m_isSDKClosed;
    int m_skipHalfImageIdx = -1;
    short m_n_buffer;
    short* m_buff_num;
    HANDLE* m_buff_event;
    WORD** m_buff_addr;
};

#endif // CAMERA_PCO_H
