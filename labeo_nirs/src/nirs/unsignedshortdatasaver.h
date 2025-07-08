#ifndef UnsignedShortDataSaver_H
#define UnsignedShortDataSaver_H

#include <stdio.h>
#include <iostream>
#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <QVector>
#include <QDir>
#include <QMessageBox>
#include <QtDebug>
#include <QTime>

#include "ImageConsumerInterface.h"

// Generic templated data saver with threads. Data is put into the saver in frames of size
// size_x*size_y. Each file contains save_block_size frames.
class UnsignedShortDataSaver : public QThread, public ImageConsumerInterface
{
    Q_OBJECT
public:
    UnsignedShortDataSaver(int size_x, int size_y, int frame_size, double frame_rate, const char* prefix, int bufferSizeGB);
    virtual ~UnsignedShortDataSaver();
    void setDatasetName(QString name);
    void setDatasetPath(QString path);
    void addInfo(QString new_info);
    void writeInfoFile();
    void addExpInfo(QString new_info);
    void writeExpInfoFile();
    void startSaving();
    void softStopSaving(); //No wait, for multicameras
    void stopSaving();
    void put(unsigned short* frame);
    void put(unsigned short* frame, int nFrames);
    void run();
private:
    QString p_dataset_name;
    QString p_path_name;
    QString p_info_txt;
    QString p_expInfo_txt;
    QString p_file_prefix;
    unsigned int p_save_block_size; //Number of images per file
    unsigned int nImagesToWrite; //Chunk write
    int p_frame_size;
    int m_sizeX;
    int m_sizeY;
    unsigned int p_buffer_size; //Number of images in the buffer for saving
    unsigned char* p_data_buffer;
    unsigned int p_current_pos;
    QMutex m_mutex;
    QSemaphore p_free_spots;
    QSemaphore p_used_spots;
    bool m_isThreadStarted;
};

inline UnsignedShortDataSaver::UnsignedShortDataSaver(int size_x, int size_y, int extra_size, double frame_rate, const char* prefix, int bufferSizeGB) :
    p_file_prefix(prefix), p_current_pos(0), p_used_spots(0)
{
    p_frame_size = size_x*size_y*sizeof(unsigned short) + extra_size;
    //Calculate throughput
    int tp = size_x*size_y*2*frame_rate; //Throughput in bytes/sec
    nImagesToWrite = 50;
    //We want files of about 1000 MB.
    p_save_block_size = (int)(1000000000.0/(double)tp*(double)frame_rate); //Number of images per file
    p_save_block_size = (p_save_block_size/nImagesToWrite)*nImagesToWrite+nImagesToWrite; //Make sure it is a multiple of nImagesToWrite
    //The buffer size depends on the RAM
    p_buffer_size = bufferSizeGB*p_save_block_size;

    p_free_spots.release(p_buffer_size-nImagesToWrite);
    m_sizeX = size_x;
    m_sizeY = size_y;
    quint64 bSize = (quint64)p_frame_size*(quint64)p_buffer_size;
    p_data_buffer = new unsigned char[bSize];

    m_isThreadStarted = false;
    p_dataset_name = "dummy";
    p_path_name = QDir::homePath();
    p_info_txt="Scan info\n";
    p_expInfo_txt="";
}

inline UnsignedShortDataSaver::~UnsignedShortDataSaver()
{
    delete [] p_data_buffer;
}

inline void UnsignedShortDataSaver::addInfo(QString new_info)
{
    p_info_txt += new_info;
}

inline void UnsignedShortDataSaver::addExpInfo(QString new_info)
{
    p_expInfo_txt += new_info;
}

inline void UnsignedShortDataSaver::setDatasetName(QString name)
{
    p_dataset_name = name;
}
inline void UnsignedShortDataSaver::setDatasetPath(QString path)
{
    p_path_name = path;
}

inline void UnsignedShortDataSaver::startSaving()
{
    m_isThreadStarted = true;
    start();
}

inline void UnsignedShortDataSaver::softStopSaving()
{
    m_mutex.lock();
    m_isThreadStarted = false;
    m_mutex.unlock();
    //wait();
}

inline void UnsignedShortDataSaver::stopSaving()
{
    m_mutex.lock();
    m_isThreadStarted = false;
    m_mutex.unlock();
    wait();
}

inline void UnsignedShortDataSaver::put(unsigned short* frame)
{
    if (m_isThreadStarted) {
        p_free_spots.acquire();
        memcpy(&p_data_buffer[(p_current_pos % (unsigned int)p_buffer_size) * (unsigned int)p_frame_size],frame,p_frame_size*sizeof(unsigned char));
        p_used_spots.release();
        p_current_pos+=1;
    }
}

inline void UnsignedShortDataSaver::put(unsigned short* frames, int nFrames)
{
    if (m_isThreadStarted) {
        p_free_spots.acquire(nFrames);
        if ((p_current_pos % p_buffer_size)+nFrames <= p_buffer_size)
            memcpy(&p_data_buffer[(p_current_pos % (unsigned int)p_buffer_size) * (unsigned int)p_frame_size],frames,p_frame_size*nFrames*sizeof(unsigned char));
        else { //We have to rollback in the buffer
            int spaceLeft = p_buffer_size - (p_current_pos % p_buffer_size);
            memcpy(&p_data_buffer[(p_current_pos % (unsigned int)p_buffer_size) * (unsigned int)p_frame_size],frames,p_frame_size*spaceLeft*sizeof(unsigned char));
            memcpy(&p_data_buffer[0],&frames[(unsigned int)spaceLeft * (unsigned int)p_frame_size],p_frame_size*(nFrames-spaceLeft)*sizeof(unsigned char));
        }
        p_current_pos+=nFrames;
        p_used_spots.release(nFrames);
    }
}

inline void UnsignedShortDataSaver::writeInfoFile()
{
    QDir parent_dir = QDir::cleanPath(p_path_name);
    parent_dir.mkdir(p_dataset_name);
    parent_dir.setPath(QDir::cleanPath(p_path_name + QDir::separator() + p_dataset_name +QDir::separator()));

    QString tmp = "info.txt";
    tmp=parent_dir.absolutePath()+QDir::separator()+tmp;
    QFile file(tmp);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox msg;
        msg.setWindowTitle("Error");
        msg.setText("Could not open file info.txt, check disk space or location. Exiting.");
        msg.exec();
        qCritical() << "Could not open info.txt file: " << tmp;
        exit(-1);
    }
    QTextStream out(&file);
    out << p_info_txt;
    file.close();
}

inline void UnsignedShortDataSaver::writeExpInfoFile()
{
    QDir parent_dir = QDir::cleanPath(p_path_name);
    parent_dir.mkdir(p_dataset_name);
    parent_dir.setPath(QDir::cleanPath(p_path_name + QDir::separator() + p_dataset_name +QDir::separator()));

    QString tmp = "ExperimentInfo.txt";
    tmp=parent_dir.absolutePath()+QDir::separator()+tmp;
    QFile file(tmp);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox msg;
        msg.setWindowTitle("Error");
        msg.setText("Could not open file ExperimentInfo.txt, check disk space or location. Exiting.");
        msg.exec();
        qCritical() << "Could not open ExperimentInfo.txt file: " << tmp;
        exit(-1);
    }
    QTextStream out(&file);
    out << p_expInfo_txt;
    file.close();
}

inline void UnsignedShortDataSaver::run()
{
    QDir parent_dir = QDir::cleanPath(p_path_name);
    parent_dir.mkdir(p_dataset_name);
    parent_dir.setPath(QDir::cleanPath(p_path_name + QDir::separator() + p_dataset_name));
    QFile file;
    QString tmp;
    int header_info[5];
    int version = 3;
    header_info[0]=version;
    header_info[1]=m_sizeX;
    header_info[2]=m_sizeY;
    header_info[3]=p_frame_size;
    header_info[4]=p_save_block_size;
    QByteArray header = QByteArray::fromRawData((const char*) header_info,5*sizeof(int));

    int file_num = 0;
    unsigned int index = 0;
    bool isNewFileCreated = false; //Prevents multiple new files
    while (true)
    {
        if(index % p_save_block_size<nImagesToWrite && !isNewFileCreated)
        {
            if(file.isOpen()) file.close();
            // Change file when we have a chunk
            tmp=QString("%1_%2.bin").arg(p_file_prefix.toUtf8()).arg(file_num,5,10,QLatin1Char('0'));
            tmp=parent_dir.absolutePath()+ QDir::separator()+tmp;
            file.setFileName(tmp);
            file.open(QIODevice::WriteOnly);
            // Write header
            file.write(header);
            file_num++;
            isNewFileCreated = true;
        }

        m_mutex.lock();
        if(!m_isThreadStarted) { //Finish saving
            nImagesToWrite = 1; //Don't use chunks to finish saving
            m_mutex.unlock();
            if (p_used_spots.available() == 0)
                break;
        } else {
            m_mutex.unlock();
        }

        // Acquire a block of data
        unsigned int nAvail = p_used_spots.available();
        if (nAvail >= nImagesToWrite) {
            //qDebug() << "Saving buffer" << p_used_spots.available();
            p_used_spots.acquire(nImagesToWrite);
            QByteArray data_tmp = QByteArray::fromRawData((const char*) &p_data_buffer[(index % (unsigned int)p_buffer_size) * (unsigned int)p_frame_size],sizeof(unsigned char)*p_frame_size*nImagesToWrite);
            file.write(data_tmp);
            //file.flush();
            p_free_spots.release(nImagesToWrite);
            index+=nImagesToWrite;
            isNewFileCreated = false;
            if (index%200==0) {
                if (nAvail > 0.95*(float)p_buffer_size) {
                    qDebug() << "Warning: Save buffer 95% full, corruption imminent.";
                }
                else if (nAvail > 0.5*(float)p_buffer_size) {
                    qDebug() << "Warning: Save buffer 50% full, corruption imminent.";
                }
                else if (nAvail > 0.25*(float)p_buffer_size) {
                    qDebug() << "Warning: Save buffer 25% full, corruption imminent.";
                }
            }
        }
        else {
            QThread::msleep(1);
        }
    }
    if (file.isOpen()) file.close();
}

#endif // UnsignedShortDataSaver_H
