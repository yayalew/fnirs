#include "simulated_camerafactory.h"
#include "camera_simulated.h"

Simulated_CameraFactory::Simulated_CameraFactory()
{
    cam = (Camera*) new Camera_simulated();
}

Simulated_CameraFactory::~Simulated_CameraFactory()
{
    delete cam;
}
unsigned int Simulated_CameraFactory::GetNumberOfCameras()
{    
    return 1;
}

QString Simulated_CameraFactory::GetCameraName(unsigned int camera_id)
{
    if(camera_id==0)
    {
        return QString("Simulated Camera.");
    }
    else
    {
        return QString("");
    }
}

Camera* Simulated_CameraFactory::GetCamera(unsigned int camera_id)
{
    if( camera_id == 0)
    {
        return (Camera*) cam;
    }
    else
    {
        return NULL;
    }
}

