#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settings.h"
#include "about.h"
#include "extract_worker.h"
#include "rebuild_worker.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Add to status bar
    // addPermanentWidget keeps it on the right side
    ui->statusbar->addPermanentWidget(ui->progressBar);

    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");

    QString format = settings.value("Format").toString();
    QString quality = settings.value("Quality").toString();
    QString defaultSettings = settings.value("DefaultSettings").toString();
    QString encodeSyncPoint = settings.value("EncodeSyncPoint").toString();
    QString looping = settings.value("Looping").toString();
    QString embededFileNames = settings.value("EmbededFileNames").toString();
    QString writePeakVolume = settings.value("WritePeakVolume").toString();
    settings.endGroup();

    if (!format.isEmpty())
    {
        if (format == "vorbis")
            ui->format_comboBox->setCurrentIndex(0);
        else if (format == "pcm")
            ui->format_comboBox->setCurrentIndex(1);
        else if (format == "fadpcm")
            ui->format_comboBox->setCurrentIndex(2);
    }
    else
        ui->format_comboBox->setCurrentIndex(0);

    if (!quality.isEmpty())
        ui->quality_spinBox->setValue(quality.toInt() ? quality.toInt() : 92);
    else
        ui->quality_spinBox->setValue(92);

    if (!defaultSettings.isEmpty())
    {
        if (defaultSettings == "true")
            ui->defaultSettings_checkBox->setCheckState(Qt::Checked);
        else
            ui->defaultSettings_checkBox->setCheckState(Qt::Unchecked);
    }
    else
        ui->encodeSyncPoint_checkBox->setCheckState(Qt::Unchecked);

    if (!encodeSyncPoint.isEmpty())
    {
        if (encodeSyncPoint == "true")
            ui->encodeSyncPoint_checkBox->setCheckState(Qt::Checked);
        else
            ui->encodeSyncPoint_checkBox->setCheckState(Qt::Unchecked);
    }
    else
        ui->encodeSyncPoint_checkBox->setCheckState(Qt::Unchecked);

    if (!looping.isEmpty())
    {
        if (looping == "true")
            ui->looping_checkBox->setCheckState(Qt::Checked);
        else
            ui->looping_checkBox->setCheckState(Qt::Unchecked);
    }
    else
        ui->looping_checkBox->setCheckState(Qt::Unchecked);

    if (!embededFileNames.isEmpty())
    {
        if (embededFileNames == "true")
            ui->embededFileNames_checkBox->setCheckState(Qt::Checked);
        else
            ui->embededFileNames_checkBox->setCheckState(Qt::Unchecked);
    }
    else
        ui->embededFileNames_checkBox->setCheckState(Qt::Unchecked);

    if (!writePeakVolume.isEmpty())
    {
        if (writePeakVolume == "true")
            ui->writePeakVolume_checkBox->setCheckState(Qt::Checked);
        else
            ui->writePeakVolume_checkBox->setCheckState(Qt::Unchecked);
    }
    else
        ui->writePeakVolume_checkBox->setCheckState(Qt::Unchecked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionExtract_triggered()
{
    ui->consoleTextBox->clear();

    ExtractWorker *extractWorker = new ExtractWorker();
    QThread *thread = new QThread();
    extractWorker->moveToThread(thread); // Move extract_worker to the new thread

    // Connect signals from extract_worker to slots in MainWindow
    connect(extractWorker, &ExtractWorker::progressUpdated, this, &MainWindow::handleProgressUpdate);
    connect(extractWorker, &ExtractWorker::updateConsole, this, &MainWindow::handleConsoleUpdate);
    connect(extractWorker, &ExtractWorker::taskFinished, this, &MainWindow::handleWorkFinished);

    // Connect thread signals for cleanup
    connect(thread, &QThread::started, extractWorker, &ExtractWorker::extract_fsb);
    connect(extractWorker, &ExtractWorker::taskFinished, thread, &QThread::quit);
    connect(thread, &QThread::finished, extractWorker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start(); // Start the thread
}

void MainWindow::on_actionRebuild_triggered()
{
    ui->consoleTextBox->clear();

    RebuildWorker *rebuildWorker = new RebuildWorker();
    QThread *thread = new QThread();
    rebuildWorker->moveToThread(thread); // Move extract_worker to the new thread

    // Connect signals from extract_worker to slots in MainWindow
    connect(rebuildWorker, &RebuildWorker::progressUpdated, this, &MainWindow::handleProgressUpdate);
    connect(rebuildWorker, &RebuildWorker::updateConsole, this, &MainWindow::handleConsoleUpdate);
    connect(rebuildWorker, &RebuildWorker::taskFinished, this, &MainWindow::handleWorkFinished);

    // Connect thread signals for cleanup
    connect(thread, &QThread::started, rebuildWorker, &RebuildWorker::rebuild_bank);
    connect(rebuildWorker, &RebuildWorker::taskFinished, thread, &QThread::quit);
    connect(thread, &QThread::finished, rebuildWorker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start(); // Start the thread
}

void MainWindow::on_actionSettings_triggered()
{
    Settings settings;
    settings.setModal(true);
    settings.exec();
}

void MainWindow::on_defaultSettings_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("DefaultSettings", arg1 == Qt::Checked ? "true" : "false");
    settings.endGroup();
}

void MainWindow::on_encodeSyncPoint_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("EncodeSyncPoint", arg1 == Qt::Checked ? "true" : "false");
    settings.endGroup();
}

void MainWindow::on_looping_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("Looping", arg1 == Qt::Checked ? "true" : "false");
    settings.endGroup();
}

void MainWindow::on_embededFileNames_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("EmbededFileNames", arg1 == Qt::Checked ? "true" : "false");
    settings.endGroup();
}

void MainWindow::on_writePeakVolume_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("WritePeakVolume", arg1 == Qt::Checked ? "true" : "false");
    settings.endGroup();
}

void MainWindow::on_actionInfo_triggered()
{
    About about;
    about.setModal(true);
    about.exec();
}

void MainWindow::handleProgressUpdate(int value)
{
    // Update a progress bar or text label on the UI
    ui->progressBar->setValue(value);
}

void MainWindow::handleConsoleUpdate(QString result)
{
    ui->consoleTextBox->append(result);
    // Auto-scroll
    ui->consoleTextBox->ensureCursorVisible();
}

void MainWindow::handleWorkFinished(QString result)
{
    // Display the final result on the UI
    ui->consoleTextBox->append(result);
}

void MainWindow::on_format_comboBox_currentIndexChanged(int index)
{
    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("Format", ui->format_comboBox->itemText(index).toLower());
    settings.endGroup();
}

void MainWindow::on_quality_spinBox_valueChanged(int arg1)
{
    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("Quality", arg1);
    settings.endGroup();
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

