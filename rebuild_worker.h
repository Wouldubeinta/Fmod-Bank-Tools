#ifndef REBUILD_WORKER_H
#define REBUILD_WORKER_H

#include <QObject>
#include <QThread>
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QApplication>
#include <qsettings.h>
#include "global_errors.h"

class RebuildWorker : public QObject
{
    Q_OBJECT
public:
    explicit RebuildWorker(QObject *parent = nullptr);

private:
    ErrorChecks bankProgress(const QStringList wavList);
    ErrorChecks bankRebuild(const QString bankFile, const QString buildPath);
    QByteArray readBytes(QDataStream &in, int size);

public slots:
    void rebuild_bank();

signals:
    void progressUpdated(int value);
    void updateConsole(QString result);
    void taskFinished(QString result);
};

#endif // REBUILD_WORKER_H
