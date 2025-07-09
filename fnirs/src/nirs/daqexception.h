#ifndef IS_ARDUINO
#ifndef DAQEXCEPTION_H
#define DAQEXCEPTION_H

#include <exception>
#include <QtDebug>
#include <NIDAQmx.h>

class DAQException: public std::exception
{
public:
    DAQException(int32 error);
    virtual ~DAQException();
    virtual const char* what() const throw()
    {

        if( DAQmxFailed(m_taskError) )
        {
            DAQmxGetExtendedErrorInfo(errBuff,2048);
        }

        return errBuff;
    }
    void show();
    int32 getError(){return m_taskError;}

private:
    char*    errBuff;
    int32 m_taskError;

};

inline DAQException::DAQException(int32 error)
{
    m_taskError = error;
    errBuff=new char[2048];
    errBuff[0]='\0';
}

inline DAQException::~DAQException()
{
    delete [] errBuff;
}
inline void DAQException::show()
{

    if( DAQmxFailed(m_taskError) )
    {
        DAQmxGetExtendedErrorInfo(errBuff,2048);
        qCritical() << errBuff;
    }
    return;
}

#endif // DAQEXCEPTION_H
#endif
