#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <QSettings>
#include <QApplication>
#include <QFileDialog>

namespace Ui {
class Settings;
}

class Settings : public QDialog
{
    Q_OBJECT

public:
    explicit Settings(QWidget *parent = nullptr);
    ~Settings();

private slots:
    void on_bankInputDirectory_Button_clicked();
    void on_wavOutputDirectory_Button_clicked();
    void on_buildOutputDirectory_Button_clicked();
    void on_cacheDirectory_Button_clicked();

private:
    Ui::Settings *ui;
};

#endif // SETTINGS_H
