#include "nirscontrolform.h"
#include "ui_nirscontrolform.h"
#include "daqexception.h"
#include <QRegularExpressionValidator>

#define DAQmxErrChk(functionCall) if( DAQmxFailed(m_error=(functionCall)) ) throw DAQException(m_error) ; else

NirsControlForm::NirsControlForm(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NirsControlForm), m_taskHandleIllumination(0), m_taskHandlePWM(0), m_analogView(0), m_analogInput(0),
      m_bSaveData(false), m_dataSaverAnalogInputs(0), m_intensity(100)
{
    ui->setupUi(this);

    // Allowed dataset names
    QRegularExpression rx("^[a-z]*");
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(rx, this);
    ui->lineEdit_datasetName->setValidator(validator);

    // Validator for intensity (0–100%)
    QRegularExpression rxIntensity("^(100|[0-9]?[0-9])$");
    QRegularExpressionValidator* intensityValidator = new QRegularExpressionValidator(rxIntensity, this);
    ui->lineEdit_intensity->setValidator(intensityValidator);

    // Validator for number of emitters (1–8 for hardware-timed)
    QRegularExpression rxEmitters("^[1-8]$");
    QRegularExpressionValidator* emittersValidator = new QRegularExpressionValidator(rxEmitters, this);
    ui->lineEdit_numEmitters->setValidator(emittersValidator);

    connect(ui->pushButton_start, SIGNAL(clicked()), this, SLOT(startAcquisition()));
    connect(ui->pushButton_stop, SIGNAL(clicked()), this, SLOT(stopAcquisition()));
    connect(ui->checkBox_saveData, SIGNAL(clicked(bool)), this, SLOT(saveData(bool)));
    connect(ui->pushButton_saveDirectory, SIGNAL(clicked()), this, SLOT(setSaveDir()));

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
    unsigned int numEmitters = ui->lineEdit_numEmitters->text().toUInt();
    m_intensity = ui->lineEdit_intensity->text().toUInt();
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

    // Illumination config
    double emitterRate = acq_rate * numEmitters;
    double pwmRate = 100 * emitterRate;
    int frameRepeats = 50; // buffer for 50 frames
    QVector<uInt32> pattern(numEmitters);
    for (int i = 0; i < numEmitters; ++i) {
        pattern[i] = (1U << i); // L1, 0b00000010, ...
    }
    QVector<uInt32> fullBuffer;
    for (int i = 0; i < frameRepeats; ++i) {
        for (int j = 0; j < numEmitters; ++j) {
            if (m_intensity == 100) {
                //  On for 100% intensity
                for (int k = 0; k < 100; ++k) {
                    fullBuffer.push_back(pattern[j]);
                }
            } else {
                // PWM with clamped duty cycle
                int pwmOnSamples = (m_intensity * 100) / 100; // 0–99
                for (int k = 0; k < 100; ++k) {
                    fullBuffer.push_back(k < pwmOnSamples ? pattern[j] : 0);
                }
            }
        }
    }

    try {
        // Digital output task
        if (m_taskHandleIllumination) {
            DAQmxStopTask(m_taskHandleIllumination);
            DAQmxClearTask(m_taskHandleIllumination);
        }
        DAQmxErrChk(DAQmxCreateTask("Illumination", &m_taskHandleIllumination));
        DAQmxErrChk(DAQmxCreateDOChan(m_taskHandleIllumination, "/Dev2/port0/line0:7", "", DAQmx_Val_ChanForAllLines));
        DAQmxErrChk(DAQmxCfgSampClkTiming(m_taskHandleIllumination, "", pwmRate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, fullBuffer.size()));
        DAQmxErrChk(DAQmxWriteDigitalU32(m_taskHandleIllumination, fullBuffer.size(), false, 10.0, DAQmx_Val_GroupByScanNumber, fullBuffer.data(), nullptr, nullptr));

        // PWM counter task (only for intensity < 100%)
        if (m_intensity < 100) {
            if (m_taskHandlePWM) {
                DAQmxStopTask(m_taskHandlePWM);
                DAQmxClearTask(m_taskHandlePWM);
            }
            DAQmxErrChk(DAQmxCreateTask("PWM", &m_taskHandlePWM));
            double dutyCycle = std::max(0.01, std::min(0.99, m_intensity / 100.0)); // Clamp to 0.01–0.99
            DAQmxErrChk(DAQmxCreateCOPulseChanFreq(m_taskHandlePWM, "/Dev2/ctr1", "", DAQmx_Val_Hz, DAQmx_Val_Low, 0.0, pwmRate, dutyCycle));
        }
    } catch (DAQException& e) {
        char errBuff[2048] = {'\0'};
        DAQmxGetErrorString(e.getError(), errBuff, 2048);
        QMessageBox msg;
        msg.setWindowTitle("DAQmx Error");
        msg.setText(QString("Error in illumination/PWM setup: %1").arg(errBuff));
        msg.exec();
        exit(-1);
    }

    m_analogInput->setAnalogViewer(m_analogView);

    // Start tasks
    try {
        if (m_taskHandlePWM) {
            DAQmxErrChk(DAQmxStartTask(m_taskHandlePWM));
        }
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

    if (m_taskHandleIllumination) {
        DAQmxStopTask(m_taskHandleIllumination);
        DAQmxClearTask(m_taskHandleIllumination);
        m_taskHandleIllumination = 0;
    }
    if (m_taskHandlePWM) {
        DAQmxStopTask(m_taskHandlePWM);
        DAQmxClearTask(m_taskHandlePWM);
        m_taskHandlePWM = 0;
    }
    delete m_analogInput;
    m_analogInput = nullptr;
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

void NirsControlForm::analogViewClosed()
{
    // Closed by user using X
}


