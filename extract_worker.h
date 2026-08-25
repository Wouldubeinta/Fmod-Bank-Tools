#ifndef EXTRACT_WORKER_H
#define EXTRACT_WORKER_H

#include <QObject>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <qsettings.h>
#include <fmod_errors.h>
#include "global_errors.h"

class ExtractWorker : public QObject
{
    Q_OBJECT
public:
    // Explicit constructor to prevent implicit conversion expressions
    explicit ExtractWorker(QObject *parent = nullptr);

private:
    // Internal pipeline exception handlers and validation modules
    ErrorChecks handleExtractionError(ErrorChecks errorChecks, const QString &bankFile, QString bankPath, FMOD_CREATESOUNDEXINFO &exinfo);
    ErrorChecks handlePasswordProtectedBank(QString bankPath, FMOD_CREATESOUNDEXINFO &exinfo);

    // Core file parsing routines and asset management modules
    ErrorChecks processSubSounds(FMOD_SOUND *sound, QFileInfo bankFileInfo, const QString &wavDir, quint32 fsbIndex);
    void writeFilenamesToFile(const QStringList &filenames, const QString &outputFilePath);

public slots:
    // Background worker task slot managed by QThread event loop calls
    void extract_fsb();

signals:
    // Interface communication pipes back to the main thread UI components
    void progressUpdated(int value);
    void updateConsole(QString result);
    void taskFinished(QString result);
};

// ================================================
// PACKED WAVHeader STRUCT & LOW-LEVEL PACKING FIX
// ================================================
// Enforce tight 1-byte boundary spacing layouts to ensure exact file-to-binary streaming mapping
#pragma pack(push, 1)
struct WAVHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    quint32 headerLength = 0;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    quint32 fmtChunkLen = 18; // 18 for PCM
    quint16 formatType = 1; // 1 = Uncompressed Integer PCM
    quint16 numChannels = 0;
    quint32 sampleRate = 0;
    quint32 bytesPerSecond = 0;
    quint16 bytesPerSample = 0;
    quint32 bitsPerChannel = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    quint32 dataSize = 0;
};
#pragma pack(pop) // Safely restores the compiler's default alignment constraints

#endif // EXTRACT_WORKER_H
