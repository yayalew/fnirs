#include "nirscontrolform.h"
#include "ui_nirscontrolform.h"
#include "daqexception.h"
#include <QRegularExpressionValidator>

#define DAQmxErrChk(functionCall) if( DAQmxFailed(m_error=(functionCall)) ) throw DAQException(m_error) ; else

NirsControlForm::NirsControlForm(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NirsControlForm), m_taskHandleIllumination(), m_taskHandleDigitalOut(), m_analogView(0), m_analogInput(0),
      m_bSaveData(false), m_dataSaverAnalogInputs(0)
{
    ui->setupUi(this);

    // Allowed dataset names (remove leading spaces)
    QRegularExpression rx("^[a-z]*");
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(rx, this);
    ui->lineEdit_datasetName->setValidator(validator);

    m_viewTimer = new QTimer();

    connect(ui->pushButton_start, SIGNAL(clicked()), this, SLOT(startAcquisition()));
    connect(ui->pushButton_stop, SIGNAL(clicked()), this, SLOT(stopAcquisition()));
    connect(ui->checkBox_saveData, SIGNAL(clicked(bool)), this, SLOT(saveData(bool)));
    connect(m_viewTimer, SIGNAL(timeout()), this, SLOT(updateDigitalOutput()));

    m_saveDir = QDir::home();
    ui->label_savedir->setText(m_saveDir.absolutePath());
    connect(ui->pushButton_saveDirectory, SIGNAL(clicked()), this, SLOT(setSaveDir()));
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
    } catch (DAQException&) {
        QMessageBox msg;
        msg.setWindowTitle("Error");
        msg.setText(QString("Could not find the acquisition card, is the power on on the card, is it plugged?"));
        msg.exec();
        exit(-1);
    }

    // Saving config
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

    // Digital output config for 20 emitters
    const int numEmitters = 20;
    QVector<uInt32> digitalPatterns(numEmitters);
    for (int i = 0; i < numEmitters; ++i) {
        digitalPatterns[i] = (1U << i); // Bitmask: 0b00000001, 0b00000010, ..., 0b10000000...
    }
    m_currentEmitterIndex = 0;

    try {
        // Create digital output task for 20 lines (e.g., Dev1/port0/line0:19)
        DAQmxErrChk(DAQmxCreateTask("DigitalOutput", &m_taskHandleDigitalOut));
        DAQmxErrChk(DAQmxCreateDOChan(m_taskHandleDigitalOut, "/Dev1/port0/line0:19", "", DAQmx_Val_ChanForAllLines));
        DAQmxErrChk(DAQmxCfgSampClkTiming(m_taskHandleDigitalOut, "/Dev1/ai/SampleClock", acq_rate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1));
        // Write initial digital pattern
        DAQmxErrChk(DAQmxWriteDigitalU32(m_taskHandleDigitalOut, 1, 0, 10.0, DAQmx_Val_GroupByChannel, &digitalPatterns[m_currentEmitterIndex], nullptr, nullptr));
    } catch (DAQException& e) {
        e.show();
        exit(-1);
    }

    // Illumination config
    int nSamplesOn = 100 * acq_rate;
    QVector<uInt32> IllumVect;
    IllumVect.fill(1, nSamplesOn - 1);
    try {
        DAQmxErrChk(DAQmxCreateTask("Illumination", &m_taskHandleIllumination));
        DAQmxErrChk(DAQmxCreateDOChan(m_taskHandleIllumination, "/Dev1/port0", "", DAQmx_Val_ChanForAllLines));
        DAQmxErrChk(DAQmxCfgOutputBuffer(m_taskHandleIllumination, IllumVect.length()));
        DAQmxErrChk(DAQmxCfgSampClkTiming(m_taskHandleIllumination, nullptr, 10000, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, nSamplesOn - 1));
        DAQmxErrChk(DAQmxCfgDigEdgeStartTrig(m_taskHandleIllumination, "/Dev1/aiStartTrigger", DAQmx_Val_Rising));
        DAQmxErrChk(DAQmxWriteDigitalU32(m_taskHandleIllumination, IllumVect.length(), 0, 10.0, DAQmx_Val_GroupByChannel, IllumVect.data(), nullptr, nullptr));
    } catch (DAQException& e) {
        e.show();
        exit(-1);
    }

    // Camera clock
    DAQmxErrChk(DAQmxCreateTask("Clock", &m_taskHandleClock));
    DAQmxErrChk(DAQmxCreateCOPulseChanFreq(m_taskHandleClock, "/Dev1/ctr0", "", DAQmx_Val_Hz, DAQmx_Val_Low, 0.0, (double)acq_rate, 0.5));

    m_analogInput->setAnalogViewer(m_analogView);

    // Start everything in the right order
    try {
        DAQmxErrChk(DAQmxStartTask(m_taskHandleDigitalOut));
        DAQmxErrChk(DAQmxStartTask(m_taskHandleIllumination));
        m_analogInput->Start(false);
        m_viewTimer->start(1000 / acq_rate); // Update at acquisition rate
        DAQmxErrChk(DAQmxStartTask(m_taskHandleClock));
    } catch (DAQException& e) {
        e.show();
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

    DAQmxStopTask(m_taskHandleClock);
    DAQmxClearTask(m_taskHandleClock);
    DAQmxStopTask(m_taskHandleIllumination);
    DAQmxClearTask(m_taskHandleIllumination);
    DAQmxStopTask(m_taskHandleDigitalOut);
    DAQmxClearTask(m_taskHandleDigitalOut);
    delete m_analogInput;
}

void NirsControlForm::updateDigitalOutput()
{
    const int numEmitters = 20;
    m_currentEmitterIndex = (m_currentEmitterIndex + 1) % numEmitters;
    uInt32 digitalPattern = (1U << m_currentEmitterIndex);
    try {
        DAQmxErrChk(DAQmxWriteDigitalU32(m_taskHandleDigitalOut, 1, 0, 10.0, DAQmx_Val_GroupByChannel, &digitalPattern, nullptr, nullptr));
    } catch (DAQException& e) {
        e.show();
        stopAcquisition();
    }
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