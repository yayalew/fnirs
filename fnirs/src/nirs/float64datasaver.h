#ifndef FLOAT64DATASAVER_H
#define FLOAT64DATASAVER_H

#include <stdio.h>
#include <iostream>
#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <QVector>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QtDebug>
#include <QMessageBox>

class Float64DataSaver : public QThread
{
    Q_OBJECT
public:
    Float64DataSaver(int size_x, int size_y, int extra_size, int save_block_size, const char* prefix);
    virtual ~Float64DataSaver();
    void setDatasetName(QString name);
    void setDatasetPath(QString path);
    void addInfo(QString new_info);
    void writeInfoFile();
    void startSaving();
    void stopSaving();
    void put(double* frame);
    void run();
private:
    QString p_dataset_name;
    QString p_path_name;
    QString p_info_txt;
    QString p_file_prefix;
    int p_save_block_size;
    int p_frame_size;
    int m_sizeX;
    int m_sizeY;
    int p_buffer_size;
    unsigned char* p_data_buffer;
    unsigned int p_current_pos;
    QMutex m_mutex;
    QSemaphore p_free_spots;
    QSemaphore p_used_spots;
    bool m_isThreadStarted;
    unsigned int p_total_frames; // Track total frames written
};

inline Float64DataSaver::Float64DataSaver(int size_x, int size_y, int extra_size, int save_block_size, const char* prefix) :
    p_save_block_size(save_block_size), p_free_spots(2 * p_save_block_size),
    p_used_spots(0), p_current_pos(0), p_file_prefix(prefix), p_total_frames(0)
{
    p_frame_size = size_x * size_y * sizeof(double) + extra_size;
    m_sizeX = size_x;
    m_sizeY = size_y; // Set to 1 for 1 sample per frame
    p_buffer_size = 2 * p_save_block_size;

    p_data_buffer = new unsigned char[p_frame_size * p_buffer_size];

    m_isThreadStarted = false;
    p_dataset_name = "dummy";
    p_path_name = QDir::homePath();
    p_info_txt = "Scan info\n\n";
}

inline Float64DataSaver::~Float64DataSaver()
{
    delete[] p_data_buffer;
    qDebug() << "[Float64DataSaver] Total frames written:" << p_total_frames;
}

inline void Float64DataSaver::addInfo(QString new_info)
{
    p_info_txt += new_info;
}

inline void Float64DataSaver::setDatasetName(QString name)
{
    p_dataset_name = name;
}

inline void Float64DataSaver::setDatasetPath(QString path)
{
    p_path_name = path;
}

inline void Float64DataSaver::startSaving()
{
    m_isThreadStarted = true;
    start();
}

inline void Float64DataSaver::stopSaving()
{
    m_mutex.lock();
    m_isThreadStarted = false;
    m_mutex.unlock();
    wait();
}

inline void Float64DataSaver::put(double* frame)
{
    p_free_spots.acquire();
    memcpy(&p_data_buffer[(p_current_pos % p_buffer_size) * p_frame_size], frame, p_frame_size);
    p_used_spots.release();
    p_current_pos += 1;
}

inline void Float64DataSaver::writeInfoFile()
{
    QDir parent_dir = QDir::cleanPath(p_path_name);
    parent_dir.mkdir(p_dataset_name);
    parent_dir.setPath(QDir::cleanPath(p_path_name + QDir::separator() + p_dataset_name + QDir::separator()));

    QString tmp = "info.txt";
    tmp = parent_dir.absolutePath() + QDir::separator() + tmp;
    QFile file(tmp);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox msg;
        msg.setWindowTitle("Error");
        msg.setText("Could not open file info.txt, check disk space or location. Exiting.");
        msg.exec();
        qCritical() << "Could not open info.txt file:" << tmp;
        exit(-1);
    }
    QTextStream out(&file);
    out << p_info_txt << "Version:2\n"
        << "Channels:" << m_sizeX << "\n"
        << "SamplesPerFrame:" << m_sizeY << "\n"
        << "FrameSizeBytes:" << p_frame_size << "\n"
        << "BlockSizeFrames:" << p_save_block_size << "\n"
        << "TotalFrames:" << p_total_frames << "\n";
    file.close();
}

inline void Float64DataSaver::run()
{
    QDir parent_dir = QDir::cleanPath(p_path_name);
    parent_dir.mkdir(p_dataset_name);
    parent_dir.setPath(QDir::cleanPath(p_path_name + QDir::separator() + p_dataset_name));
    QFile file;
    QString tmp;
    int header_info[5];
    int version = 2;
    header_info[0] = version;
    header_info[1] = m_sizeX;
    header_info[2] = m_sizeY;
    header_info[3] = p_frame_size;
    header_info[4] = p_save_block_size;
    QByteArray header = QByteArray::fromRawData((const char*)header_info, 5 * sizeof(int));

    unsigned int file_num = 0;
    unsigned int index = 0;
    bool isNewFileCreated = false;
    while (true)
    {
        if (index % p_save_block_size == 0 && !isNewFileCreated)
        {
            if (file.isOpen()) file.close();
            tmp = QString("%1_%2.bin").arg(QString(p_file_prefix)).arg(file_num, 5, 10, QLatin1Char('0'));
            tmp = parent_dir.absolutePath() + QDir::separator() + tmp;
            file.setFileName(tmp);
            file.open(QIODevice::WriteOnly);
            file.write(header);
            file_num++;
            isNewFileCreated = true;
        }
        if (p_used_spots.available() > 0)
        {
            p_used_spots.acquire();
            QByteArray data_tmp = QByteArray::fromRawData((const char*)&p_data_buffer[(index % p_buffer_size) * p_frame_size], p_frame_size);
            file.write(data_tmp);
            file.flush();
            p_free_spots.release();
            index++;
            p_total_frames++; // Increment total frames
            isNewFileCreated = false;
            qDebug() << "[Float64DataSaver] Written frame" << p_total_frames << "at index" << index;
        }
        else
        {
            QThread::msleep(10);
        }
        m_mutex.lock();
        if (!m_isThreadStarted && p_used_spots.available() == 0)
        {
            m_mutex.unlock();
            break;
        }
        else
        {
            m_mutex.unlock();
        }
    }
    if (file.isOpen()) file.close();
}

#endif // FLOAT64DATASAVER_H


