#ifndef ANALOGVIEWER_H
#define ANALOGVIEWER_H

#include <QWidget>
#include <QImage>
#include <QLabel>
#include <QKeyEvent>
#include <QStatusBar>
#include <QRubberBand>
#include "qcustomplot.h" // the header file of QCustomPlot. Don't forget to add it to your project, if you use an IDE, so it gets compiled.
//#include "NIDAQmx.h"
#include "hardwaresettings.h"

class QScrollBar;

class AnalogViewer : public QWidget
{
    Q_OBJECT
public:
    explicit AnalogViewer(double _samplingRate, HardwareSettings* hardware_settings_ptr);
    virtual ~AnalogViewer();
    void put(double* data);
    void getDefault(void** theDefaults);
    void setDefault(void** theDefaults);
    void closeEvent(QCloseEvent *bar);
    int getChunkSize() {return chunkSize;}

public slots:
    void addData();

    void modifyPlot(int idxPlot);
    void CheckTriggerSettings();
private slots:
    virtual void mousePressed(QMouseEvent *event,int idxPlot);
    virtual void mouseWheel(QWheelEvent *event, int idxPlot);
signals:
    void aboutToQuit();

private:
    QImage p_image;
    QVBoxLayout *plotlayout;
    QVBoxLayout *optionlayout;
    QHBoxLayout *mainLayout;
    QCustomPlot *customPlot;
    bool *isCustomPlotEnabled;
    QLabel *p_status_bar_left;
    QLabel *p_status_bar_middle;
    QLabel *p_status_bar_right;
    QStatusBar* p_status_bar;
    QLabel *m_labelInfo;
    QCheckBox *checkBoxes;

    QList<QString> m_theAIName;

    QTimer dataTimer;

    //Buffer between AI class and chart
    quint32 chunkSize; //Size of the data sent from the analog input class
    unsigned int nChunksPerSecond; //Number of chunks that can be fitted in the buffer
    int p_buffer_size;
    double* p_data_buffer;
    unsigned int p_current_pos;
    unsigned int p_current_read_pos;
    QMutex m_mutex;

    double theTime; //The time calculated by the display function
    double theTimeRange; //X range of the axis
    const double maxTimeRange = 600.0;

    bool hasOptogen = false;

    QTime profTest;
    QStringList channelsName;
    double samplingRate; //May be different from AISAMPRATE with Arduino system
    bool m_isPaused;

    //Trigger mode
    QComboBox* cbAcqMode;
    QComboBox* cbSource;
    QLineEdit* leLevel;
    QComboBox* cbEdge;
    struct Trigger {
        //Inputs from user
        bool isTriggerMode;
        bool lastTriggerMode;
        int sourceChannel;
        double level; //Voltage level threshold
        int edgeType; //0 for rising, 1 for falling, 2 for both

        //Hardcoded input
        double deadTime; //Time to wait before receiving a new rising edge, must be greater than the display after trig (800 ms)
        double displayTimeBeforeTrig; //Should be a multiple of the chunk size for now (50ms)
        double displayTimeAfterTrig; //Should be a multiple of the chunk size for now (50ms)

        //The term rising edge is used for reference, but it could also be a falling edge, or both
        bool rxTriggerBelow;
        bool rxRisingEdge;
        bool bothEdgeIsOver; //When trigger on both edges, remember if we are over or below
        int risingEdgePos; //Position of the rising edge in the buffer
        int risingEdgeChunkPos; //Position of the rising edge in the chunk
        int readPosTrigger; //Position of the rising edge FINDER in the buffer
        int firstDisplayPos; //Position when we go back 200 ms in the buffer
    } mTrigger;
    HardwareSettings* m_hardwareSettingsPtr;

public:
    bool isFirstRun = true;
};

//extern double ai_fs;

#endif // ANALOGVIEWER_H
