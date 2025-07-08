#ifndef CAMERA_SIMULATED_H
#define CAMERA_SIMULATED_H

#include "camera.h"
#include <QTime>

class Camera_simulated : public Camera
{
    Q_OBJECT
public:
    Camera_simulated();
    virtual ~Camera_simulated();
    char* Discover() {return NULL;}
    void SetCameraID(const char*) {;} //For multiple cameras
    void Open();
    void Close();
    double GetLostRatio() {return -1.0;}
    void Configure();
    void GetSize(int &nx, int &ny);
    void SetExposureTime(double exposure_time_ms);
    void SetBinning(int binning);
    void SetFrameRate(double newFrameRate);
    double GetRollingDuration();
    void configOnDemand(){;}
    unsigned short *getSingleFrame();
    void SetSkipHalfImage(int) {;} //For multiple cameras
    QVector<int> GetAvailableFrameRates();
    int GetMaximumBinning() {return 8;}
    bool AllowsBulbMode() {return true;}
    bool AllowsMultiCam() {return false;}
    void VerifyROI(QRect &roi);
    void GetMaxROI(QRect &roi);
    void VerifyExposureTime(double &exposure);
    double GetMaxExposureWithoutCrosstalk();
    double GetMaxExposureWhileFullyIlluminated();

protected:
    int GetNextBuffer();
private:
    int mNChunkSize; //Number of images to acquire before sending to the save and display threads
};

#endif // CAMERA_SIMULATED_H
