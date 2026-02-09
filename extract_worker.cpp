#include "extract_worker.h"
#include "fileio.h"
#include "bank_extract.h"

ExtractWorker::ExtractWorker(QObject *parent) : QObject(parent) {}

void ExtractWorker::extract_fsb()
{
    FMOD_RESULT result;
    FMOD_SYSTEM *system = nullptr;
    FMOD_SOUND *sound = nullptr;

    FMOD_CREATESOUNDEXINFO exinfo = {};
    memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.length = 0;

    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Directorys");
    QString fsbDir = QCoreApplication::applicationDirPath() + "/fsb/";
    QString bankDir = settings.value("BankDir").toString() + "/";
    QString wavDir = settings.value("WavDir").toString() + "/";
    settings.endGroup();

    QDir bank_directory(bankDir);
    QStringList bankFileList = bank_directory.entryList(QStringList() << "*.bank");

    int i = 0;

    for (QString &bankFile : bankFileList)
    {
        QString bankPath = bankDir + bankFile;
        QFileInfo bankFileInfo(bankPath);
        quint32 fsbCount;
        int check = bank_extract::extract(bankPath, fsbCount);

        QString newLineCheck = (i == 0) ? "" : "\n";
        emit updateConsole(newLineCheck + "Initializing Fmod Bank file - " + QFileInfo(bankPath).fileName() + "\n");

        if (check != 1) // checking for encryption or error
        {
            if (handleExtractionError(check, bankFile, bankPath, exinfo))
                continue; // Skip to the next bank file if error
        }

        QString fsbName = QFileInfo(bankPath).fileName().replace(".bank", "");

        for (quint32 j = 0; j < fsbCount; j++)
        {
            QString fsbPath = fsbDir + fsbName + "[" + QString::number(j) + "].fsb";
            if (!QFileInfo::exists(fsbPath))
            {
                emit taskFinished("Error, " + fsbName + "[" + QString::number(j) + "].fsb" + " file is missing !!!");
                continue;
            }

            result = FMOD_System_Create(&system);
            if (result != FMOD_OK || (result = FMOD_System_Init(system, 1, FMOD_INIT_NORMAL, nullptr)) != FMOD_OK)
            {
                emit taskFinished(FMOD_ErrorString(result));
                continue;
            }

            result = FMOD_System_CreateSound(system, fsbPath.toUtf8().constData(), FMOD_OPENONLY, &exinfo, &sound);
            if (result != FMOD_OK)
            {
                emit taskFinished(FMOD_ErrorString(result));
                continue;
            }

            QDir dir(wavDir);
            dir.mkdir(bankFileInfo.fileName().replace(".bank", "") + "[" + QString::number(j) + "]");

            emit updateConsole("\nExtracting fsb file - " + QFileInfo(fsbPath).fileName() + "\n");

            bool error = processSubSounds(sound, bankFileInfo, wavDir, j);
            if (error)
                continue;

            delete[] exinfo.encryptionkey;
            FMOD_Sound_Release(sound);
            FMOD_System_Release(system);
        }

        i++;
    }

    if (bankFileList.count() != 0)
    {
        emit taskFinished("\nExtracting Bank files has finished."); // Signal completion
        emit progressUpdated(0);
        qInfo() << "Extracting Bank files has finished.";
    }
    else
    {
        emit taskFinished("No bank files found."); // Signal completion
        emit progressUpdated(0);
        qInfo() << "No bank files found.";
    }
}

bool ExtractWorker::handleExtractionError(int errorCheck, const QString &bankFile, QString bankPath, FMOD_CREATESOUNDEXINFO &exinfo)
{
    switch (errorCheck)
    {
        case 0:
            emit taskFinished("Error extracting bank file: " + bankFile);
            return true;
            break;
        case 2:
            emit taskFinished("Error, can't find any fsb audio in this bank file: " + bankFile);
            return true;
            break;
        case 5:
            return handlePasswordProtectedBank(bankPath, exinfo);
            break;
        default:
            emit taskFinished("Unknown error with bank file: " + bankFile);
            return true;
            break;
    }
    return false;
}

bool ExtractWorker::handlePasswordProtectedBank(QString bankPath, FMOD_CREATESOUNDEXINFO &exinfo)
{
    QString passwordTxT = bankPath.replace(".bank", ".txt");

    // Check if the password text file exists
    if (!QFileInfo::exists(passwordTxT)) {
        emit updateConsole("Can't find " + passwordTxT + " with password for decryption.\n");
        return true;
    }

    QString password = readTextFileToQStringList(passwordTxT).constFirst();

    // Ensure the password list is not empty before accessing
    if (password.isEmpty()) {
        emit updateConsole("Password file is empty: " + passwordTxT + "\n");
        return true;
    }

    // Convert password to QByteArray and manage memory safely
    QByteArray encryptionKeyArray = password.toUtf8();
    char* encryption = new char[encryptionKeyArray.size() + 1];
    std::memcpy(encryption, encryptionKeyArray.constData(), encryptionKeyArray.size() + 1);
    exinfo.encryptionkey = encryption;

    // Emit console update based on password availability
    emit updateConsole("Decrypting bank file with password: " + encryptionKeyArray + "\n");
    return false;
}

bool ExtractWorker::processSubSounds(FMOD_SOUND *sound, QFileInfo bankFileInfo, const QString &wavDir, quint32 fsbIndex)
{
    FMOD_RESULT result;
    int numsubsounds = 0;
    QString wavPath = wavDir + bankFileInfo.fileName().replace(".bank", "") + "[" + QString::number(fsbIndex) + "]/";

    QDir dir(wavPath);
    if (dir.exists()) {
        // Remove the wav directory and all its contents
        dir.removeRecursively();
        // Recreate the empty wav directory
        dir.mkpath(wavPath);
    }

    result = FMOD_Sound_GetNumSubSounds(sound, &numsubsounds);
    if (result != FMOD_OK)
    {
        emit taskFinished(FMOD_ErrorString(result));
        return true;
    }

    QStringList txtFileNames;

    for (int j = 0; j < numsubsounds; j++)
    {
        FMOD_SOUND *sound_to_play;
        FMOD_SOUND_TYPE   stype;
        FMOD_SOUND_FORMAT sformat;

        unsigned int length = 0, dataLen = 0, nameLength = 64;
        int schannels = 0, sbits = 0, priority = 0;
        char             subsoundsName[64];
        char*            buffer = 0;
        float            ssamplerate = 0;

        result = FMOD_Sound_GetSubSound(sound, j, &sound_to_play);
        if (result != FMOD_OK)
        {
            emit taskFinished(FMOD_ErrorString(result));
            return true;
        }

        result = FMOD_Sound_SeekData(sound_to_play, 0);
        if (result != FMOD_OK)
        {
            emit taskFinished(FMOD_ErrorString(result));
            return true;
        }

        result = FMOD_Sound_GetDefaults(sound_to_play, &ssamplerate, &priority);
        if (result != FMOD_OK)
        {
            emit taskFinished(FMOD_ErrorString(result));
            return true;
        }

        result = FMOD_Sound_GetFormat(sound_to_play, &stype, &sformat, &schannels, &sbits);
        if (result != FMOD_OK)
        {
            emit taskFinished(FMOD_ErrorString(result));
            return true;
        }

        result = FMOD_Sound_GetLength(sound_to_play, &length, FMOD_TIMEUNIT_PCMBYTES);
        if (result != FMOD_OK)
        {
            emit taskFinished(FMOD_ErrorString(result));
            return true;
        }

        result = FMOD_Sound_GetName(sound_to_play, subsoundsName, nameLength);
        if (result != FMOD_OK)
        {
            emit taskFinished(FMOD_ErrorString(result));
            return true;
        }

        QString subsoundName = QString::fromUtf8(subsoundsName);

        // If no filename, it creates one
        if (subsoundName.isEmpty())
            subsoundName = "sound_" + QString::number(j);

        QDir dir(wavPath);
        QString baseName = subsoundName;
        QString fileName = baseName + ".wav";

        // Loop until a unique filename is found
        for (int suffix = j; dir.exists(fileName); ++suffix) {
            subsoundName = QString("%1_%2").arg(baseName).arg(suffix);
            fileName = subsoundName + ".wav";
        }

        QString wavName = dir.absoluteFilePath(fileName);

        QFile file(wavName);
        file.open(QIODevice::WriteOnly);

        // Check if the wav file is open for writing
        if (!file.isOpen() || !file.isWritable()) {
            emit taskFinished("Wav File is not open for writing.");
            return true;
        }

        writeWAVHeader(file, ssamplerate, sbits, schannels, length);

        quint32 chunkCount = fileio::chunkAmount(length);
        std::vector<quint64> _chunkSizes = fileio::chunkSizes(length, chunkCount);

        for (unsigned int k = 0; k < chunkCount; k++)
        {
            buffer = (char*)malloc(_chunkSizes[k]);
            result = FMOD_Sound_ReadData(sound_to_play, buffer, _chunkSizes[k], &dataLen);
            if (result != FMOD_OK) { emit taskFinished(FMOD_ErrorString(result)); emit progressUpdated(0); return true; }

            if (buffer != 0)
            {
                file.write(buffer, _chunkSizes[k]);
            }
            else
            {
                emit taskFinished("Error reading wav data chunks !!!");
                emit progressUpdated(0);
                return true;
            }

            free(buffer);
        }

        file.flush();
        file.close();

        int subSoundsPercent = 100 * (j + 1) / numsubsounds;
        emit progressUpdated(subSoundsPercent); // Emit signal to update progress in UI
        QString index = QString::number(j);
        txtFileNames << subsoundName + ".wav";
        emit updateConsole(index + ": (" + subsoundName + ".wav) [Extracting]"); // Emit signal to update console in UI
        //qInfo() << "Bit Depth: " + QString::number(sbits);

        FMOD_Sound_Release(sound_to_play);
    }
    writeFilenamesToFile(txtFileNames, wavPath + bankFileInfo.fileName().replace(".bank", "") + "[" + QString::number(fsbIndex) + "].txt");
    return false;
}

void ExtractWorker::writeFilenamesToFile(const QStringList &filenames, const QString &outputFilePath) {
    QFile file(outputFilePath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &filename : filenames) {
            out << filename << "\n"; // Write each filename followed by a newline
        }
        file.close();
        //qDebug() << "Filenames successfully written to" << outputFilePath;
    } else {
        qDebug() << "Error opening file for writing:" << file.errorString();
    }
}

void ExtractWorker::writeWAVHeader(QFile& file, unsigned int sampleRate, short bitsPerChannel, short numChannels, unsigned int dataLen)
{
    // Constants for WAV header
    const unsigned int fmtChunkLen = 18; // 18 for PCM
    const unsigned short formatType = 1;  // PCM format
    const unsigned int headerLen = dataLen + 38; // 38 = 4 (RIFF) + 4 (WAVE) + 4 (fmt) + 4 (data) + 4 (fmtChunkLen) + 2 (formatType) + 2 (numChannels) + 4 (sampleRate) + 4 (bytesPerSecond) + 4 (bytesPerSample) + 2 (bitsPerChannel)

    // Write the WAV header
    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&headerLen), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    file.write(reinterpret_cast<const char*>(&fmtChunkLen), 4);
    file.write(reinterpret_cast<const char*>(&formatType), 2);
    file.write(reinterpret_cast<const char*>(&numChannels), 2);
    file.write(reinterpret_cast<const char*>(&sampleRate), 4);

    short bytesPerSample = bitsPerChannel / 8;
    unsigned int bytesPerSecond = sampleRate * numChannels * bytesPerSample;
    file.write(reinterpret_cast<const char*>(&bytesPerSecond), 4);

    file.write(reinterpret_cast<const char*>(&bytesPerSample), 2);
    file.write(reinterpret_cast<const char*>(&bitsPerChannel), 4);
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&dataLen), 4);
}

QStringList ExtractWorker::readTextFileToQStringList(const QString& filePath) {
    QStringList stringList;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit taskFinished("\nCould not open file: " + filePath);
        return stringList; // Return empty list if file cannot be opened
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        stringList.append(line);
    }

    file.close();
    return stringList;
}
