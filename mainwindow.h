#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QApplication>
#include <QFileDialog>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionExtract_triggered();
    void on_actionRebuild_triggered();
    void on_actionExit_triggered();
    void handleProgressUpdate(int value);
    void handleConsoleUpdate(QString result);
    void handleWorkFinished(QString result);
    void on_actionInfo_triggered();

    void on_format_comboBox_currentIndexChanged(int index);

    void on_quality_spinBox_valueChanged(int arg1);

    void on_actionSettings_triggered();

    void on_encodeSyncPoint_checkBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_looping_checkBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_embededFileNames_checkBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_writePeakVolume_checkBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_defaultSettings_checkBox_checkStateChanged(const Qt::CheckState &arg1);

private:
    Ui::MainWindow *ui;
    QStringList readTextFileToQStringList(const QString& filePath);
    void bankRebuild(const QString filePath);
};

#endif // MAINWINDOW_H
