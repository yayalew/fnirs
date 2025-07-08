#include "nirscontrolform.h"
#include "ui_nirscontrolform.h"
#include "daqexception.h"
#include <QRegularExpressionValidator>

#define DAQmxErrChk(functionCall) if( DAQmxFailed(m_error=(functionCall)) ) throw DAQException(m_error) ; else

NirsControlForm::NirsControlForm(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NirsControlForm), m_taskHandleIllumination(), m_analogView(0), m_analogInput(0),
      m_bSaveData(false), m_dataSaverAnalogInputs(0)
{
    ui->setupUi(this);

    // Allowed dataset names (remove leading spaces)
    QRegularExpression rx("^[a-z]*");
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(rx, this);
    ui->lineEdit_datasetName->setValidator(validator);

    m_viewTimer = new QTimer();

    connect(ui->pushButton_start,SIGNAL(clicked()),this,SLOT(startAcquisition()));
    connect(ui->pushButton_stop,SIGNAL(clicked()),this,SLOT(stopAcquisition()));
    connect(ui->checkBox_saveData,SIGNAL(clicked(bool)),this,SLOT(saveData(bool)));

    m_saveDir = QDir::home();
    ui->label_savedir->setText(m_saveDir.absolutePath());
    connect(ui->pushButton_saveDirectory,SIGNAL(clicked()),this,SLOT(setSaveDir()));
    //    //  Here I am looking for plugins assuming there will only be one camera, eventually we can correct if we mix cameras.





}

NirsControlForm::~NirsControlForm()
{
    delete ui;
}

void NirsControlForm::startAcquisition()
{

    unsigned int acq_rate = ui->lineEdit_frameRate->text().toUInt();
    m_analogView = new AnalogViewer(acq_rate,&m_hardwareSettings);
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
    //Pass sampling rate to analoginput object, KP
    //m_analogInput->set_AnalogSamplingRate((int) acq_rate);


    // Saving config
    if(m_bSaveData)
    {
        m_saveName = ui->lineEdit_datasetName->text();
        m_saveDir.setPath(ui->label_savedir->text());

        // Start Analog inputs, saving will be synchronized since camera triggers are saved.
        m_dataSaverAnalogInputs = new Float64DataSaver(16,acq_rate,0,256,"nirs");
        m_dataSaverAnalogInputs->setDatasetName(m_saveName);
        m_dataSaverAnalogInputs->setDatasetPath(m_saveDir.absolutePath());
        m_analogInput->SetDataSaver(m_dataSaverAnalogInputs);
        m_dataSaverAnalogInputs->startSaving();

    }


    // Illumination config
    int nSamplesOn=100*acq_rate;
    QVector<uInt32> IllumVect;
    IllumVect.fill(1,nSamplesOn-1);
    try {
        //Lights output, synchronized and triggered on camera clock
        DAQmxErrChk(DAQmxCreateTask("Illumination",&m_taskHandleIllumination));
        DAQmxErrChk(DAQmxCreateDOChan(m_taskHandleIllumination,"/Dev1/port0","",DAQmx_Val_ChanForAllLines));
        // Turn on light according to vector
        DAQmxErrChk(DAQmxCfgOutputBuffer(m_taskHandleIllumination, IllumVect.length()));
        // Base frequency is 100 * framerate, triggered on camera clock and re-triggered. Have to make sure we are done prior to camera stop.
        DAQmxErrChk(DAQmxCfgSampClkTiming(m_taskHandleIllumination, nullptr, 10000, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, nSamplesOn-1));
        DAQmxErrChk(DAQmxCfgDigEdgeStartTrig(m_taskHandleIllumination, "/Dev1/aiStartTrigger", DAQmx_Val_Rising));
        DAQmxErrChk(DAQmxWriteDigitalU32(m_taskHandleIllumination, IllumVect.length(), 0, 10.0, DAQmx_Val_GroupByChannel, IllumVect.data(), nullptr, nullptr));
    }
    catch (DAQException& e)
    {
        e.show();
        exit(-1);
    }

    // Camera clock
    DAQmxErrChk(DAQmxCreateTask("Clock",&m_taskHandleClock));
    DAQmxErrChk(DAQmxCreateCOPulseChanFreq(m_taskHandleClock,"/Dev1/ctr0","",DAQmx_Val_Hz,DAQmx_Val_Low,0.0,(double)acq_rate,0.5));

    m_analogInput->setAnalogViewer(m_analogView);
 //   connect(m_analogView, SIGNAL(aboutToQuit()), this, SLOT(analogViewClosed()));

    // Start everything in the right order
    try
    {
        // Camera and analog IN are trigged on PFI12 from digital out clock, start it first since it won't record until
        // it receives a trigger
        DAQmxErrChk(DAQmxStartTask(m_taskHandleIllumination));
        m_analogInput->Start(false);
        m_viewTimer->start(30);
        DAQmxErrChk(DAQmxStartTask(m_taskHandleClock));

    }
    catch (DAQException& e)
    {
        e.show();
        exit(-1);
    }
}

void NirsControlForm::stopAcquisition()
{
    m_analogInput->Stop();

    if(m_dataSaverAnalogInputs){
        m_dataSaverAnalogInputs->stopSaving();
        delete m_dataSaverAnalogInputs;
        m_dataSaverAnalogInputs=0;
    }
//    disconnect(m_analogView, SIGNAL(aboutToQuit()), this, SLOT(analogViewClosed()));
    m_analogView->close();
    delete m_analogView;
    m_analogView = nullptr;


    m_analogInput->resetDataSaver();


    m_viewTimer->stop();

    DAQmxStopTask(m_taskHandleClock);
    DAQmxClearTask(m_taskHandleClock);
    DAQmxStopTask(m_taskHandleIllumination);
    DAQmxClearTask(m_taskHandleIllumination);
    delete m_analogInput;


}


void NirsControlForm::saveData(bool flag)
{
    m_bSaveData = flag;
}

void NirsControlForm::analogViewClosed()
{
    //Closed by user using X
}

void NirsControlForm::setSaveDir()
{
    QString dataDir = QFileDialog::getExistingDirectory(this, tr("choose Directory"),
                                                m_saveDir.absolutePath(),
                                                QFileDialog::ShowDirsOnly
                                                | QFileDialog::DontResolveSymlinks);
    m_saveDir.setPath(dataDir);
    ui->label_savedir->setText(m_saveDir.absolutePath());
}
