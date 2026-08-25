#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settings.h"
#include "about.h"
#include "extract_worker.h"
#include "rebuild_worker.h"
#include "fileio.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // Initialize user interface components designed in Qt Creator (.ui file)
    ui->setupUi(this);
    QString version = APP_VERSION;
    this->setWindowTitle(QString("Fmod Bank Tools - v%1").arg(version));

    // ==========================================
    // CONFIGURATION LOADING & PATH VALIDATION
    // ==========================================
    // Define the path to the application's configuration file
    QString config = QString("%1/config.ini").arg(QCoreApplication::applicationDirPath());
    QSettings settings(config, QSettings::IniFormat);

    settings.beginGroup("Directorys");

    // Read and validate the Bank Directory path; fall back to a default folder if missing/invalid
    QString bankDir = fileio::resolveFolderPath(settings.value("BankDir").toString());
    if (bankDir.isEmpty() || !QFileInfo::exists(bankDir))
    {
        bankDir = QString("%1/bank").arg(QCoreApplication::applicationDirPath());
        settings.setValue("BankDir", bankDir);
    }

    // Read and validate the WAV Directory path
    QString wavDir = fileio::resolveFolderPath(settings.value("WavDir").toString());
    if (wavDir.isEmpty() || !QFileInfo::exists(wavDir))
    {
        wavDir = QString("%1/wav").arg(QCoreApplication::applicationDirPath());
        settings.setValue("WavDir", wavDir);
    }

    // Read and validate the Rebuild Output Directory path
    QString rebuildDir = fileio::resolveFolderPath(settings.value("RebuildDir").toString());
    if (rebuildDir.isEmpty() || !QFileInfo::exists(rebuildDir))
    {
        rebuildDir = QString("%1/build").arg(QCoreApplication::applicationDirPath());
        settings.setValue("RebuildDir", rebuildDir);
    }

    // Read and validate the FSB Cache Directory path
    QString cacheDir = fileio::resolveFolderPath(settings.value("CacheDir").toString());
    if (cacheDir.isEmpty() || !QFileInfo::exists(cacheDir))
    {
        cacheDir = QString("%1/fsbcache").arg(QCoreApplication::applicationDirPath());
        settings.setValue("CacheDir", cacheDir);
    }
    settings.endGroup();

    // ==========================================
    // ENCODING & AUDIO OPTIONS PARSING
    // ==========================================
    settings.beginGroup("Options");
    QString format = settings.value("Format").toString();
    QString quality = settings.value("Quality").toString();
    QString cpuThreads = settings.value("CPUThreads").toString();
    QString defaultSettings = settings.value("DefaultSettings").toString();
    QString encodeSyncPoint = settings.value("EncodeSyncPoint").toString();
    QString looping = settings.value("Looping").toString();
    QString embededFileNames = settings.value("EmbededFileNames").toString();
    QString writePeakVolume = settings.value("WritePeakVolume").toString();
    settings.endGroup();

    // Sync Audio Format dropdown menu selections
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
        ui->format_comboBox->setCurrentIndex(0); // Default fallback format

    // Initialize the audio encoding compression quality spin box
    if (!quality.isEmpty())
        ui->quality_spinBox->setValue(quality.toInt() ? quality.toInt() : 75);
    else
        ui->quality_spinBox->setValue(75);

    // Cap the maximum multi-threading slider limit to leave 2 cores free for system stability
    ui->cpuThread_horizontalSlider->setMaximum(QThread::idealThreadCount() - 2);

    // Sync the CPU Multi-threading thread limits slider and visual label metrics
    if (!cpuThreads.isEmpty())
    {
        ui->cpuThread_horizontalSlider->setValue(cpuThreads.toInt() ? cpuThreads.toInt() : 2);
        ui->cpuThreadsValue_Label->setText(cpuThreads);
    }
    else
        ui->cpuThread_horizontalSlider->setValue(2);

    // ==========================================
    // 3. SYNCHRONISING UI CHECKBOX STATES
    // ==========================================
    if (!defaultSettings.isEmpty())
    {
        if (defaultSettings == "true")
            ui->defaultSettings_checkBox->setCheckState(Qt::Checked);
        else
            ui->defaultSettings_checkBox->setCheckState(Qt::Unchecked);
    }
    else
        // This fallback clears the encodeSyncPoint checkbox instead of defaultSettings!
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
    // Disable the extract UI action immediately to prevent double-clicks or overlapping threads
    ui->actionExtract->setEnabled(false);

    // Clear old text logs from the UI console before starting a new extraction job
    ui->consoleTextBox->clear();

    // Allocate the extraction worker context object on the heap
    ExtractWorker *extractWorker = new ExtractWorker();

    // Instantiate a separate execution thread pipeline to isolate heavy unpacking tasks from the UI thread
    QThread *thread = new QThread();

    // Shift the worker object's thread affinity so its slots execute inside the new background thread context
    extractWorker->moveToThread(thread);

    // ==========================================
    // DATA AND STATUS COMPONENT CONNECTIONS
    // ==========================================
    // Intercept incremental track processing milestones to update UI progress bars
    connect(extractWorker, &ExtractWorker::progressUpdated, this, &MainWindow::handleProgressUpdate);

    // Stream status notifications, file formats, and current extraction information directly into the console window
    connect(extractWorker, &ExtractWorker::updateConsole, this, &MainWindow::handleConsoleUpdate);

    // Listen for the overall extraction finished signal to handle post-task routines
    connect(extractWorker, &ExtractWorker::taskFinished, this, &MainWindow::handleWorkFinished);

    // Re-enable the UI Extract trigger action ONLY after the background work is completely done
    connect(extractWorker, &ExtractWorker::taskFinished, this, [this]() {
        ui->actionExtract->setEnabled(true);
    });

    // ==========================================
    // LIFECYCLE MANAGEMENT CONNECTIONS
    // ==========================================
    // As soon as the background thread spins up and enters its event loop, kick off the extraction process
    connect(thread, &QThread::started, extractWorker, &ExtractWorker::extract_fsb);

    // Order the event loop of the background thread to exit cleanly once the worker emits taskFinished
    connect(extractWorker, &ExtractWorker::taskFinished, thread, &QThread::quit);

    // Automatically flag the heap-allocated worker object for safe deletion once the thread completely exits
    connect(thread, &QThread::finished, extractWorker, &QObject::deleteLater);

    // SAFE CLEANUP PATTERN: Clean up the thread container object *only after* the worker is safely destroyed
    connect(extractWorker, &QObject::destroyed, thread, &QObject::deleteLater);

    // ==========================================
    // EXECUTION STARTUP
    // ==========================================
    // Release control back to OS scheduler to execute the background thread instructions independently
    thread->start();
}

void MainWindow::on_actionRebuild_triggered()
{
    // Disable the trigger action immediately to prevent accidental double-clicks or overlapping threads
    ui->actionRebuild->setEnabled(false);

    // Clear old text logs from the UI console before starting a new compile job
    ui->consoleTextBox->clear();

    // Allocate the worker context object on the heap
    RebuildWorker *rebuildWorker = new RebuildWorker();

    // Instantiate a separate execution thread pipeline to isolate heavy processing from the UI thread
    QThread *thread = new QThread();

    // Shift the worker object's thread affinity so its slots run inside the new background thread context
    rebuildWorker->moveToThread(thread);

    // ==========================================
    // DATA AND STATUS COMPONENT CONNECTIONS
    // ==========================================
    // Intercept incremental task percentage milestones to refresh UI progress elements
    connect(rebuildWorker, &RebuildWorker::progressUpdated, this, &MainWindow::handleProgressUpdate);

    // Stream status notifications, format specs, and progress text straight into the console window
    connect(rebuildWorker, &RebuildWorker::updateConsole, this, &MainWindow::handleConsoleUpdate);

    // Listen for the overall macro compilation completion signal to trigger final UI state resets
    connect(rebuildWorker, &RebuildWorker::taskFinished, this, &MainWindow::handleWorkFinished);

    // Re-enable the UI Rebuild trigger action ONLY after the background work is completely done
    connect(rebuildWorker, &RebuildWorker::taskFinished, this, [this]() {
        ui->actionRebuild->setEnabled(true);
    });

    // ==========================================
    // LIFECYCLE MANAGEMENT CONNECTIONS
    // ==========================================
    // As soon as the background thread spins up and enters its event loop, kick off the compilation
    connect(thread, &QThread::started, rebuildWorker, &RebuildWorker::rebuild_bank);

    // Order the event loop of the background thread to exit cleanly once the worker emits taskFinished
    connect(rebuildWorker, &RebuildWorker::taskFinished, thread, &QThread::quit);

    // Automatically flag the heap-allocated worker object for safe deletion once the thread completely exits
    connect(thread, &QThread::finished, rebuildWorker, &QObject::deleteLater);

    // SAFE CLEANUP PATTERN: Clean up the thread container object *only after* the worker is safely destroyed
    connect(rebuildWorker, &QObject::destroyed, thread, &QObject::deleteLater);

    // ==========================================
    // EXECUTION STARTUP
    // ==========================================
    // Release control back to OS scheduler to execute the background thread instructions independently
    thread->start();
}

void MainWindow::on_actionSettings_triggered()
{
    Settings settings;
    settings.setModal(true);
    settings.exec();
}

void MainWindow::on_defaultSettings_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    QString config = QString("%1/config.ini").arg(QCoreApplication::applicationDirPath());
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("DefaultSettings", arg1 == Qt::Checked ? "true" : "false");
    settings.endGroup();
}

void MainWindow::on_encodeSyncPoint_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    QString config = QString("%1/config.ini").arg(QCoreApplication::applicationDirPath());
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("EncodeSyncPoint", arg1 == Qt::Checked ? "true" : "false");
    settings.endGroup();
}

void MainWindow::on_looping_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    QString config = QString("%1/config.ini").arg(QCoreApplication::applicationDirPath());
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("Looping", arg1 == Qt::Checked ? "true" : "false");
    settings.endGroup();
}

void MainWindow::on_embededFileNames_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    QString config = QString("%1/config.ini").arg(QCoreApplication::applicationDirPath());
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("EmbededFileNames", arg1 == Qt::Checked ? "true" : "false");
    settings.endGroup();
}

void MainWindow::on_writePeakVolume_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    QString config = QString("%1/config.ini").arg(QCoreApplication::applicationDirPath());
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("WritePeakVolume", arg1 == Qt::Checked ? "true" : "false");
    settings.endGroup();
}

void MainWindow::on_format_comboBox_currentIndexChanged(int index)
{
    QString config = QString("%1/config.ini").arg(QCoreApplication::applicationDirPath());
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("Format", ui->format_comboBox->itemText(index).toLower());
    settings.endGroup();
}

void MainWindow::on_quality_spinBox_valueChanged(int arg1)
{
    QString config = QString("%1/config.ini").arg(QCoreApplication::applicationDirPath());
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("Quality", arg1);
    settings.endGroup();
}

void MainWindow::on_cpuThread_horizontalSlider_valueChanged(int value)
{
    QString config = QString("%1/config.ini").arg(QCoreApplication::applicationDirPath());
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Options");
    settings.setValue("CPUThreads", QString::number(value));
    settings.endGroup();
    ui->cpuThreadsValue_Label->setText(QString::number(value));
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

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

