#ifndef SCMOS_CAMERAFACTORY_H
#define SCMOS_CAMERAFACTORY_H

#include <QObject>
#include <QtPlugin>
#include "camerafactory.h"
#include "camera.h"
#include "camera_pco.h"


class PCO_CameraFactory : public CameraFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.labeotech.nis.PCOCameraFactory" FILE "pcocamerafactory.json")
    Q_INTERFACES(CameraFactory)
public:
    explicit PCO_CameraFactory();
    virtual ~PCO_CameraFactory();
    unsigned int GetNumberOfCameras();
    QString GetCameraName(unsigned int camera_id = 0);
    Camera* GetCamera(unsigned int camera_id = 0);
private:
    QVector<QString> m_cameraNames;
    QVector<Camera_PCO*> m_cameras;
};

#endif // SCMOS_CAMERAFACTORY_H
