#include "rebuild_worker.h"
#include "fileio.h"
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

RebuildWorker::RebuildWorker(QObject *parent) : QObject(parent) {}

void RebuildWorker::rebuild_bank()
{
    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);

    settings.beginGroup("Directorys");
    QString fsbDir = QCoreApplication::applicationDirPath() + "/fsb/";
    QString bankDir = settings.value("BankDir").toString() + "/";
    QString wavDir = settings.value("WavDir").toString() + "/";
    QString rebuildDir = settings.value("RebuildDir").toString() + "/";
    settings.endGroup();

    settings.beginGroup("Options");
    QString format = settings.value("Format").toString();
    unsigned int quality = settings.value("Quality").toUInt();
    settings.endGroup();

    QStringList nameFilters;
    nameFilters << "*.txt";

    QDirIterator it(wavDir, nameFilters, QDir::Files, QDirIterator::Subdirectories);
    QStringList wavTxtList;

    while (it.hasNext()) {
        wavTxtList << it.next();
    }

    if (wavTxtList.isEmpty())
    {
        emit taskFinished("\nError: could not find txt wav lists !!!");
        emit progressUpdated(0);
        return;
    }

    FSBANK_RESULT result;

    int i = 0;

    for (QString &wavTxt : wavTxtList)
    {
        result = FSBank_Init(FSBANK_FSBVERSION_FSB5, FSBANK_INIT_GENERATEPROGRESSITEMS, 2, 0);
        if (result != FSBANK_OK) { emit taskFinished(FSBank_ErrorString(result)); return; }

        std::vector<FSBANK_SUBSOUND> subsounds;
        QStringList wavFiles = readTextFileToQStringList(wavTxt);
        QFileInfo wavFileInfo(wavTxt);
        quint32 removePos = wavFileInfo.completeBaseName().length() - 3;
        QString bankName = wavFileInfo.completeBaseName().remove(removePos, 3);
        QString bankFileBasePath = bankDir + bankName;
        QString bankFilePath = bankFileBasePath + ".bank";
        QString passwordTxT = bankFileBasePath + ".txt";
        QString fsbFilePath = fsbDir + wavFileInfo.completeBaseName() + ".fsb";

        QString newLineCheck = (i == 0) ? "" : "\n";
        emit updateConsole(newLineCheck + "Fmod Bank file: " + bankName + ".bank");
        QString _format = format == "vorbis" ? "Vorbis" : "PCM";
        emit updateConsole("Format: " + _format);
        emit updateConsole("Thread Count: 2\n");
        emit updateConsole("ReBuilding " + bankName + ".bank has started, Please wait.....\n");

        QVector<char*> wavFile(wavFiles.size());

        for (int j = 0; j < wavFiles.size(); ++j)
        {
            QString wavFilePath = wavDir + wavFileInfo.completeBaseName() + "/" + wavFiles[j];
            QByteArray wavFilePathArray = wavFilePath.toUtf8();
            wavFile[j] = new char[wavFilePathArray.size() + 1];
            std::memcpy(wavFile[j], wavFilePathArray.constData(), wavFilePathArray.size() + 1);

            auto &subsound = subsounds.emplace_back();
            std::memset(&subsound, 0, sizeof(FSBANK_SUBSOUND));

            subsound.numFiles = 1;
            subsound.fileNames = &wavFile[j];
        }

        char* outputFile = new char[fsbFilePath.toUtf8().size() + 1];
        std::memcpy(outputFile, fsbFilePath.toUtf8().constData(), fsbFilePath.toUtf8().size() + 1);

        char* encryption = nullptr;

        // Check if the password text file exists and encrypt bank
        if (QFileInfo::exists(passwordTxT))
        {
            QString password = readTextFileToQStringList(passwordTxT).constFirst();

            // Ensure the password list is not empty before accessing
            if (password.isEmpty()) {
                emit updateConsole("Password file is empty: " + passwordTxT + "\n");
                return;
            }

            QByteArray encryptionKeyArray = password.toUtf8();
            encryption = new char[encryptionKeyArray.size() + 1];
            std::memcpy(encryption, encryptionKeyArray.constData(), encryptionKeyArray.size() + 1);
            emit updateConsole("Encrypting bank file with password: " + encryptionKeyArray + "\n");
        }

        result = FSBank_Build(subsounds.data(), subsounds.size(), format == "vorbis" ? FSBANK_FORMAT_VORBIS : FSBANK_FORMAT_PCM, FSBANK_BUILD_DEFAULT, quality, encryption, outputFile);
        if (result != FSBANK_OK) { emit taskFinished(FSBank_ErrorString(result)); return; }

        bankProgress(wavFiles);

        result = FSBank_Release();
        if (result != FSBANK_OK) { emit taskFinished(FSBank_ErrorString(result)); return; }

        for (char* file : wavFile)
        {
            delete[] file;
        }
        delete[] outputFile;
        delete[] encryption;
        bankRebuild(bankFilePath, rebuildDir);
        i++;
    }

    emit progressUpdated(0);
    emit taskFinished("\nRebuilding Bank files has finished");
}

void RebuildWorker::bankProgress(const QStringList wavList)
{
    // Build process started successfully, now fetch progress items
    const FSBANK_PROGRESSITEM* progressItem = nullptr;

    int index = 0;

    while (FSBank_FetchNextProgressItem(&progressItem) == FSBANK_OK && progressItem != nullptr)
    {
        // Process the progress item
        switch (progressItem->state)
        {
        case FSBANK_STATE_PREPROCESSING:
            // Item is waiting to be processed
            break;
        case FSBANK_STATE_ANALYSING:
            // Item is analysing
            break;
        case FSBANK_STATE_DECODING:
            // Item is decoding
            break;
        case FSBANK_STATE_ENCODING:
        {
            // Item is currently being built
            break;
        }
        case FSBANK_STATE_WRITING:
        {
            // Item is writing
            break;
        }
        case FSBANK_STATE_FINISHED:
        {
            // Item build complete
            if (progressItem->subSoundIndex != -1)
            {
                emit updateConsole(QString::number(index) + ": (" + wavList[index] + ") [Proccessing]");
                int subSoundsPercent = 100 * (index + 1) / wavList.size();
                emit progressUpdated(subSoundsPercent); // Emit signal to update progress in UI
                index++;
            }
            break;
        }
        case FSBANK_STATE_WARNING:
            // Item warning
            emit updateConsole("\nWarning, there is a issue with one of the wav files.");
            break;
        case FSBANK_STATE_FAILED:
            // Item build failed
            emit updateConsole("\nfsb file failed to build.");
            break;
        default:
            emit updateConsole("\nUnknown error");
            break;
        }

        FSBank_ReleaseProgressItem(progressItem); // Release memory for the item
        progressItem = nullptr; // Reset for next fetch
        QThread::msleep(10);
    }
}

void RebuildWorker::bankRebuild(const QString bankFile, const QString buildPath)
{
    QFile file(bankFile);
    if (!file.open(QIODevice::ReadOnly)) {
        emit taskFinished("\nError opening file: " + bankFile);
        return;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    in.setByteOrder(QDataStream::LittleEndian);

    const QByteArray magicArray = readBytes(in, 4);
    if (magicArray != "RIFF") {
        emit taskFinished("\nError, has no RIFF in header");
        return;
    }

    file.seek(0x08);
    const QByteArray fevStringArray = readBytes(in, 4);
    if (fevStringArray != "FEV ") {
        emit taskFinished("\nError, has no FEV in header");
        return;
    }

    file.seek(0x14);
    quint32 version;
    in >> version;
    if (version == 0) {
        emit taskFinished("\nError, version not supported");
        return;
    }

    file.seek(0x1c);
    const QByteArray listStringArray = readBytes(in, 4);
    if (listStringArray != "LIST") {
        emit taskFinished("\nError, has no LIST in header");
        return;
    }

    file.seek(file.pos() + 0x04);
    const QByteArray projStringArray = readBytes(in, 4);
    const QByteArray BnkiStringArray = readBytes(in, 4);
    if (projStringArray != "PROJ" || BnkiStringArray != "BNKI") {
        emit taskFinished("\nError, has no PROJ or BNKI in header");
        return;
    }

    QVector<quint32> sndh_fsbOffset, sndh_fsbSize, snd_location, snd_buffer;
    quint32 chunk_size, sndh_unknown = 0, fsbCount = 1, sndh_location = 0;

    in >> chunk_size;
    file.seek(file.pos() + chunk_size);

    sndh_fsbOffset.resize(1);
    sndh_fsbOffset[0] = 0;

    sndh_fsbSize.resize(1);
    sndh_fsbSize[0] = 0;

    snd_location.resize(1);
    snd_location[0] = 0;

    while (snd_location[0] == 0 && file.pos() < file.size()) {
        quint32 chunk_type;
        in >> chunk_type;
        in >> chunk_size;

        if (chunk_type == 0xFFFFFFFF || chunk_size == 0xFFFFFFFF) {
            return; // Invalid chunk
        }

        switch(chunk_type)
        {
            case 0x48444E53: /* "SNDH" */
            {
                fsbCount = (chunk_size - 4) / 8;

                sndh_fsbOffset.resize(fsbCount);
                sndh_fsbSize.resize(fsbCount);

                in >> sndh_unknown;

                sndh_location = file.pos();

                for (quint32 j = 0; j < fsbCount; j++)
                {
                    in >> sndh_fsbOffset[j];
                    in >> sndh_fsbSize[j];
                }
                continue;
            }
            case 0x4C425453: /* "STBL" */
            {
                quint64 currentPos = file.pos();

                if (chunk_size != 0)
                {
                    file.seek(currentPos + chunk_size);
                    quint32 chunkTypeHash = 0;
                    in >> chunkTypeHash;

                    switch(chunkTypeHash)
                    {
                       case 0x20444E53: /* "SND " */
                       case 0x48534148: /* "HASH" */
                            break;
                       default:
                            chunk_size += 1;
                            break;
                    }
                }
                file.seek(currentPos + chunk_size);
                continue;
            }
            case 0x20444E53: /* "SND " */
            {
                snd_location.resize(fsbCount);
                snd_location[0] = file.pos() - 8;
                snd_buffer.resize(fsbCount);
                snd_buffer[0] = chunk_size - sndh_fsbSize[0];

                if (fsbCount > 1)
                {
                    for (quint32 j = 0; j < fsbCount - 1; j++)
                    {
                        snd_location[j + 1] = sndh_fsbOffset[j] + sndh_fsbSize[j];
                        file.seek(snd_location[j + 1] + 4);
                        quint32 _chunk_size = 0;
                        in >> _chunk_size;
                        snd_buffer[j + 1] = _chunk_size - sndh_fsbSize[j + 1];
                    }
                }
                break;
            }
        }

        file.seek(file.pos() + chunk_size);
    }

    QString bankName = file.fileName();

    if (sndh_fsbOffset[0] == 0 || sndh_fsbSize[0] == 0) { emit taskFinished("\nRebuilding Bank Error, sndh_offset or sndh_size should not be 0 for - " + bankName); return; }
    if (sndh_location == 0) { emit taskFinished("\nRebuilding Bank Error, sndh_location should not be 0 for - " + bankName); return; }
    if (snd_location[0] == 0) { emit taskFinished("\nRebuilding Bank Error, snd_location should not be 0 for - " + bankName); return; }

    file.seek(0);
    QByteArray bankHeader = readBytes(in, sndh_fsbOffset[0]);

    file.close();

    QFileInfo fileInfo(bankName);
    QString bankNameTmp = fileInfo.fileName();
    QFile bankoutFile(buildPath + bankNameTmp);
    if (!bankoutFile.open(QIODevice::WriteOnly)) {
        emit taskFinished("\nRebuilding Bank Error, writing to: " + buildPath + bankNameTmp);
        return;
    }

    bankoutFile.write(bankHeader, sndh_fsbOffset[0]);

    QVector<quint32> fsbSizes;

    fsbSizes.resize(fsbCount);
    fsbSizes[0] = 0;

    QString fsbFileName = QCoreApplication::applicationDirPath() + "/fsb/" + bankNameTmp.replace(".bank", "");

    for (quint32 i = 0; i < fsbCount; i++)
    {
        QString fsbFilePath = fsbFileName + "[" + QString::number(i) + "].fsb";
        QFileInfo fileInfo(fsbFilePath);

        if (fileInfo.exists()) {
            fsbSizes[i] = (quint32)fileInfo.size(); // Returns the fsb size in bytes.
        } else {
            emit taskFinished("\nRebuilding Bank Error, fsb file does not exist " + fsbFilePath);
            return;
        }
    }

    if (fsbCount > 1)
    {
        for (quint32 i = 0; i < fsbCount - 1; i++)
        {
            sndh_fsbOffset[i + 1] = sndh_fsbOffset[i] + fsbSizes[i] + snd_buffer[i + 1] + 8; // Adding new fsb offsets, if more then 1 fsb in bank.
        }
    }

    bankoutFile.seek(sndh_location);

    for (quint32 i = 0; i < fsbCount; i++)
    {
        bankoutFile.write(reinterpret_cast<const char*>(&sndh_fsbOffset[i]), 4);
        bankoutFile.write(reinterpret_cast<const char*>(&fsbSizes[i]), 4);
    }

    bankoutFile.flush();
    bankoutFile.seek(snd_location[0]);

    for (quint32 i = 0; i < fsbCount; i++)
    {
        QString fsbFilePath = fsbFileName + "[" + QString::number(i) + "].fsb";
        QFile fsbInFile(fsbFilePath);
        if (!fsbInFile.open(QIODevice::ReadOnly)) {
            emit taskFinished("\nRebuilding Bank Error, reading: " + fsbFilePath);
            return;
        }

        QDataStream fsbIn(&fsbInFile);
        fsbIn.setVersion(QDataStream::Qt_DefaultCompiledVersion);
        fsbIn.setByteOrder(QDataStream::LittleEndian);

        bankoutFile.write("SND ");
        quint32 fsbTmpSize = fsbSizes[i] + snd_buffer[i];
        bankoutFile.write(reinterpret_cast<const char*>(&fsbTmpSize), 4);
        quint32 bufferSize = snd_buffer[i];
        QByteArray buffer(bufferSize, '\0');

        if (bufferSize != 0)
            bankoutFile.write(buffer);

        quint32 chunkCount = fileio::chunkAmount(fsbSizes[i]);
        std::vector<quint64> _chunkSizes = fileio::chunkSizes(fsbSizes[i], chunkCount);

        for (unsigned int k = 0; k < chunkCount; k++) {
            QByteArray fsbBuffer = readBytes(fsbIn, _chunkSizes[k]);
            bankoutFile.write(fsbBuffer);
        }

        fsbInFile.close();
    }

    quint32 headerSize = (bankoutFile.size()) - 8;
    bankoutFile.seek(4);
    bankoutFile.write(reinterpret_cast<const char*>(&headerSize), 4);

    bankoutFile.flush();
    bankoutFile.close();
}

// Helper function to read bytes
QByteArray RebuildWorker::readBytes(QDataStream &in, int size) {
    QByteArray buffer(size, 0);
    in.readRawData(buffer.data(), size);
    return buffer;
}

QStringList RebuildWorker::readTextFileToQStringList(const QString& filePath) {
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
