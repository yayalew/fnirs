#ifndef SCMOS_CAMERAFACTORY_H
#define SCMOS_CAMERAFACTORY_H

#include <QObject>
#include <QtPlugin>
#include "camera.h"
#include "camerafactory.h"

class Simulated_CameraFactory : public CameraFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.labeotech.ioi.SimulatedCameraFactory" FILE "simulatedcamerafactory.json")
    Q_INTERFACES(CameraFactory)
public:
    explicit Simulated_CameraFactory();
    virtual ~Simulated_CameraFactory();
    unsigned int GetNumberOfCameras();
    Camera* GetCamera(unsigned int camera_id = 0);
    QString GetCameraName(unsigned int camera_id = 0);
private:
    Camera* cam;
};

#endif // SCMOS_CAMERAFACTORY_H
