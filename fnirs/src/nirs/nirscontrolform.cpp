#include "nirscontrolform.h"
#include "ui_nirscontrolform.h"
#include "daqexception.h"
#include <QRegularExpressionValidator>

#define DAQmxErrChk(functionCall) if( DAQmxFailed(m_error=(functionCall)) ) throw DAQException(m_error) ; else

NirsControlForm::NirsControlForm(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NirsControlForm), m_taskHandleIllumination(0), m_analogView(0), m_analogInput(0),
      m_bSaveData(false), m_dataSaverAnalogInputs(0)
{
    ui->setupUi(this);

    // Allowed dataset names (remove leading spaces)
    QRegularExpression rx("^[a-z]*");
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(rx, this);
    ui->lineEdit_datasetName->setValidator(validator);

    m_viewTimer = new QTimer();
    m_viewTimer->setTimerType(Qt::PreciseTimer);

    connect(ui->pushButton_start, SIGNAL(clicked()), this, SLOT(startAcquisition()));
    connect(ui->pushButton_stop, SIGNAL(clicked()), this, SLOT(stopAcquisition()));
    connect(ui->checkBox_saveData, SIGNAL(clicked(bool)), this, SLOT(saveData(bool)));
    connect(ui->pushButton_saveDirectory, SIGNAL(clicked()), this, SLOT(setSaveDir()));
    connect(m_viewTimer, SIGNAL(timeout()), this, SLOT(updateIllumination()));

    m_saveDir = QDir::home();
    ui->label_savedir->setText(m_saveDir.absolutePath());
}

NirsControlForm::~NirsControlForm()
{
    delete ui;
}

void NirsControlForm::startAcquisition()
{
    unsigned int acq_rate = ui->lineEdit_frameRate->text().toUInt();
    m_analogView = new AnalogViewer(acq_rate, &m_hardwareSettings);
    m_analogView->show();

    try {
        m_analogInput = new AnalogInput(acq_rate);
    } catch (DAQException& e) {
        char errBuff[2048] = {'\0'};
        DAQmxGetErrorString(e.getError(), errBuff, 2048);
        QMessageBox msg;
        msg.setWindowTitle("DAQmx Error");
        msg.setText(QString("Could not initialize analog input: %1").arg(errBuff));
        msg.exec();
        exit(-1);
    }

    if (m_bSaveData)
    {
        m_saveName = ui->lineEdit_datasetName->text();
        m_saveDir.setPath(ui->label_savedir->text());

        m_dataSaverAnalogInputs = new Float64DataSaver(16, acq_rate, 0, 256, "nirs");
        m_dataSaverAnalogInputs->setDatasetName(m_saveName);
        m_dataSaverAnalogInputs->setDatasetPath(m_saveDir.absolutePath());
        m_analogInput->SetDataSaver(m_dataSaverAnalogInputs);
        m_dataSaverAnalogInputs->startSaving();
    }

    try {
        if (m_taskHandleIllumination) {
            DAQmxStopTask(m_taskHandleIllumination);
            DAQmxClearTask(m_taskHandleIllumination);
        }

        const int numEmitters = 4;
        double emitterRate = acq_rate * numEmitters;  // 2 emitters per acquisition frame

        QVector<uInt32> pattern = {
            0b00000001, // line 0 on
            0b00000010  // line 1 on
        };

        QVector<uInt32> fullBuffer;
        int frameRepeats = 50;  // 50 full frames (2 samples per frame)
        for (int i = 0; i < frameRepeats; ++i)
            fullBuffer += pattern;

        DAQmxErrChk(DAQmxCreateTask("Illumination", &m_taskHandleIllumination));
        DAQmxErrChk(DAQmxCreateDOChan(
            m_taskHandleIllumination,
            "/Dev2/port0/line0:1",
            "",
            DAQmx_Val_ChanForAllLines
        ));

        DAQmxErrChk(DAQmxCfgSampClkTiming(
            m_taskHandleIllumination,
            "", // use internal DO clock
            emitterRate,
            DAQmx_Val_Rising,
            DAQmx_Val_ContSamps,
            fullBuffer.size()
        ));

        DAQmxErrChk(DAQmxWriteDigitalU32(
            m_taskHandleIllumination,
            fullBuffer.size(),
            false,  // do not autostart
            10.0,
            DAQmx_Val_GroupByScanNumber,
            fullBuffer.data(),
            nullptr,
            nullptr
        ));

    } catch (DAQException& e) {
        char errBuff[2048] = {'\0'};
        DAQmxGetErrorString(e.getError(), errBuff, 2048);
        QMessageBox msg;
        msg.setWindowTitle("DAQmx Error");
        msg.setText(QString("Error in illumination setup: %1").arg(errBuff));
        msg.exec();
        exit(-1);
    }

    m_analogInput->setAnalogViewer(m_analogView);

    try {
        DAQmxErrChk(DAQmxStartTask(m_taskHandleIllumination));
        m_analogInput->Start(false);
    } catch (DAQException& e) {
        char errBuff[2048] = {'\0'};
        DAQmxGetErrorString(e.getError(), errBuff, 2048);
        QMessageBox msg;
        msg.setWindowTitle("DAQmx Error");
        msg.setText(QString("Error starting tasks: %1").arg(errBuff));
        msg.exec();
        exit(-1);
    }
}









void NirsControlForm::stopAcquisition()
{
    m_analogInput->Stop();

    if (m_dataSaverAnalogInputs) {
        m_dataSaverAnalogInputs->stopSaving();
        delete m_dataSaverAnalogInputs;
        m_dataSaverAnalogInputs = 0;
    }
    m_analogView->close();
    delete m_analogView;
    m_analogView = nullptr;

    m_analogInput->resetDataSaver();

    m_viewTimer->stop();

    if (m_taskHandleIllumination) {
        DAQmxStopTask(m_taskHandleIllumination);
        DAQmxClearTask(m_taskHandleIllumination);
    }
    delete m_analogInput;
}

void NirsControlForm::saveData(bool flag)
{
    m_bSaveData = flag;
}

void NirsControlForm::setSaveDir()
{
    QString dataDir = QFileDialog::getExistingDirectory(this, tr("Choose Directory"),
                                                m_saveDir.absolutePath(),
                                                QFileDialog::ShowDirsOnly
                                                | QFileDialog::DontResolveSymlinks);
    m_saveDir.setPath(dataDir);
    ui->label_savedir->setText(m_saveDir.absolutePath());
}

void NirsControlForm::updateIllumination()
{
    const int numEmitters = 24;
    m_currentEmitterIndex = (m_currentEmitterIndex + 1) % numEmitters;
    try {
        DAQmxErrChk(DAQmxWriteDigitalLines(m_taskHandleIllumination, 1, 1, 10.0, DAQmx_Val_GroupByChannel, m_illumStates[m_currentEmitterIndex].data(), nullptr, nullptr));
    } catch (DAQException& e) {
        char errBuff[2048] = {'\0'};
        DAQmxGetErrorString(e.getError(), errBuff, 2048);
        QMessageBox msg;
        msg.setWindowTitle("DAQmx Error");
        msg.setText(QString("Error updating illumination: %1").arg(errBuff));
        msg.exec();
        stopAcquisition();
    }
}

void NirsControlForm::analogViewClosed()
{
    //Closed by user using X
}



