#include "pco_camerafactory.h"

PCO_CameraFactory::PCO_CameraFactory()
{
    //Ennumerate available cameras
    Camera_PCO* cam = new Camera_PCO();

    QString camID("pco.edge 5.5");
    m_cameraNames.append(camID);
    cam->SetCameraID(camID.toStdString().c_str());
    m_cameras.append(cam);
}

PCO_CameraFactory::~PCO_CameraFactory()
{
    for(unsigned int i=0;i<m_cameras.size();i++) delete m_cameras.at(i);
}

unsigned int PCO_CameraFactory::GetNumberOfCameras()
{    
    return m_cameraNames.size();
}

QString PCO_CameraFactory::GetCameraName(unsigned int camera_id)
{
    // Needs protection
    return m_cameraNames.at(camera_id);
}

Camera* PCO_CameraFactory::GetCamera(unsigned int camera_id)
{
    if( camera_id < m_cameraNames.size() )
    {
        return (Camera*) m_cameras.at(camera_id);
    }
    else
    {
        return NULL;
    }
}
