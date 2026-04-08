#include "settings.h"
#include "ui_settings.h"

Settings::Settings(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Settings)
{
    ui->setupUi(this);

    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Directorys");

    QString bankDir = settings.value("BankDir").toString();

    if (bankDir.isEmpty())
    {
        bankDir = QCoreApplication::applicationDirPath() + "/bank";
        settings.setValue("BankDir", bankDir);
    }

    ui->bankInputDirectory_lineEdit->setText(bankDir);

    QString wavDir = settings.value("WavDir").toString();

    if (wavDir.isEmpty())
    {
        wavDir = QCoreApplication::applicationDirPath() + "/wav";
        settings.setValue("WavDir", wavDir);
    }

    ui->wavOutputDirectory_lineEdit->setText(wavDir);

    QString rebuildDir = settings.value("RebuildDir").toString();

    if (rebuildDir.isEmpty())
    {
        rebuildDir = QCoreApplication::applicationDirPath() + "/build";
        settings.setValue("RebuildDir", rebuildDir);
    }

    ui->buildOutputDirectory_lineEdit->setText(rebuildDir);

    QString cacheDir = settings.value("CacheDir").toString();

    if (cacheDir.isEmpty())
    {
        cacheDir = QCoreApplication::applicationDirPath() + "/fsbcache";
        settings.setValue("CacheDir", cacheDir);
    }

    ui->cacheDirectory_lineEdit->setText(cacheDir);
    settings.endGroup();
}

Settings::~Settings()
{
    delete ui;
}

void Settings::on_bankInputDirectory_Button_clicked()
{
    QString dir = QCoreApplication::applicationDirPath() + "/bank";
    QFileDialog::Options options = QFileDialog::ShowDirsOnly;
    QString folder = QFileDialog::getExistingDirectory(nullptr, "Select Bank Folder", dir, options);

    if(folder.isEmpty())
        ui->bankInputDirectory_lineEdit->setText(dir);
    else
        ui->bankInputDirectory_lineEdit->setText(folder);

    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Directorys");
    settings.setValue("BankDir", ui->bankInputDirectory_lineEdit->text());
    settings.endGroup();
}


void Settings::on_wavOutputDirectory_Button_clicked()
{
    QString dir = QCoreApplication::applicationDirPath() + "/wav";
    QFileDialog::Options options = QFileDialog::ShowDirsOnly;
    QString folder = QFileDialog::getExistingDirectory(nullptr, "Select Wav Folder", dir, options);

    if(folder.isEmpty())
        ui->wavOutputDirectory_lineEdit->setText(dir);
    else
        ui->wavOutputDirectory_lineEdit->setText(folder);

    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Directorys");
    settings.setValue("WavDir", ui->wavOutputDirectory_lineEdit->text());
    settings.endGroup();
}


void Settings::on_buildOutputDirectory_Button_clicked()
{
    QString dir = QCoreApplication::applicationDirPath() + "/build";
    QFileDialog::Options options = QFileDialog::ShowDirsOnly;
    QString folder = QFileDialog::getExistingDirectory(nullptr, "Select Build Folder", dir, options);

    if(folder.isEmpty())
        ui->buildOutputDirectory_lineEdit->setText(dir);
    else
        ui->buildOutputDirectory_lineEdit->setText(folder);

    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Directorys");
    settings.setValue("RebuildDir", ui->buildOutputDirectory_lineEdit->text());
    settings.endGroup();
}


void Settings::on_cacheDirectory_Button_clicked()
{
    QString dir = QCoreApplication::applicationDirPath() + "/fsbcache";
    QFileDialog::Options options = QFileDialog::ShowDirsOnly;
    QString folder = QFileDialog::getExistingDirectory(nullptr, "Select FSB Cache Folder", dir, options);

    if(folder.isEmpty())
        ui->cacheDirectory_lineEdit->setText(dir);
    else
        ui->cacheDirectory_lineEdit->setText(folder);

    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Directorys");
    settings.setValue("CacheDir", ui->cacheDirectory_lineEdit->text());
    settings.endGroup();
}

