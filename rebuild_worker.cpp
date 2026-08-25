#include "rebuild_worker.h"
#include "fileio.h"
#include <fsbank_errors.h>
#include "global_errors.h"

RebuildWorker::RebuildWorker(QObject *parent) : QObject(parent) {}

void RebuildWorker::rebuild_bank()
{
    // ==========================================
    // INITIALISATION & CONFIGURATION LOADING
    // ==========================================
    QString config = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(config, QSettings::IniFormat);

    settings.beginGroup("Directorys");
    QString fsbDir = QCoreApplication::applicationDirPath() + "/fsb/";
    QString bankDir = fileio::resolveFolderPath(settings.value("BankDir").toString()) + "/";
    QString wavDir = fileio::resolveFolderPath(settings.value("WavDir").toString()) + "/";
    QString rebuildDir = fileio::resolveFolderPath(settings.value("RebuildDir").toString()) + "/";
    QString cacheDir = fileio::resolveFolderPath(settings.value("CacheDir").toString()) + "/";
    settings.endGroup();

    settings.beginGroup("Options");
    QString format = settings.value("Format").toString();
    unsigned int quality = settings.value("Quality").toUInt();
    unsigned int cpuThreads = settings.value("CPUThreads").toUInt();
    QString defaultSettings = settings.value("DefaultSettings").toString();
    QString encodeSyncPoint = settings.value("EncodeSyncPoint").toString();
    QString looping = settings.value("Looping").toString();
    QString embededFileNames = settings.value("EmbededFileNames").toString();
    QString writePeakVolume = settings.value("WritePeakVolume").toString();
    settings.endGroup();

    // ==========================================
    // DISCOVERING TARGET TEXT FILES (.TXT)
    // ==========================================
    QStringList nameFilters;
    nameFilters << "*.txt";

    QDirIterator it(wavDir, nameFilters, QDir::Files, QDirIterator::Subdirectories);
    QStringList wavTxtList;

    while (it.hasNext()) {
        wavTxtList << it.next();
    }

    if (wavTxtList.isEmpty())
    {
        emit taskFinished(GlobalErrors::errorToString(ErrorChecks::TextWavFileOpenFailure));
        emit progressUpdated(0);
        return;
    }

    FSBANK_RESULT result;
    int i = 0;

    // ==========================================
    // MAIN REBUILD LOOP
    // ==========================================
    for (QString &wavTxt : wavTxtList)
    {
        // Safe, automatic memory management for the cache directory string
        QByteArray cacheDirArray = cacheDir.toUtf8();
        char* cachedir = cacheDirArray.data();

        result = FSBank_Init(FSBANK_FSBVERSION_FSB5, FSBANK_INIT_GENERATEPROGRESSITEMS, cpuThreads, cachedir);
        if (result != FSBANK_OK) {
            emit updateConsole(FSBank_ErrorString(result));
            continue;
        }

        std::vector<FSBANK_SUBSOUND> subsounds;
        QStringList wavFiles = readTextFileToQStringList(wavTxt);
        QFileInfo wavFileInfo(wavTxt);

        quint32 removePos = wavFileInfo.completeBaseName().length() - 3;
        QString bankName = wavFileInfo.completeBaseName().remove(removePos, 3);
        QString bankFileBasePath = bankDir + bankName;
        QString bankFilePath = bankFileBasePath + ".bank";
        QString passwordTextFile = QDir(bankDir).filePath("password.txt");
        QString passwordBankTextFile = bankFileBasePath + ".txt";
        QString fsbFilePath = fsbDir + wavFileInfo.completeBaseName() + ".fsb";

        QString newLineCheck = (i == 0) ? "" : "\n";
        emit updateConsole(QString("%1Fmod Bank file: %2.bank").arg(newLineCheck, bankName));
        QString _format = "Vorbis";

        if (format == "pcm")
            _format = "PCM";
        else if (format == "fadpcm")
            _format = "FADPCM";

        emit updateConsole(QString("Format: %1").arg(_format));
        emit updateConsole(QString("Thread Count: %1\n").arg(cpuThreads));
        emit updateConsole(QString("ReBuilding %1.bank has started, Please wait.....\n").arg(bankName));

        if (!QFileInfo::exists(bankFileBasePath + ".bank"))
        {
            emit updateConsole(QString("Aborting bank rebuilding, can't find - %1.bank").arg(bankFileBasePath));
            result = FSBank_Release();
            if (result != FSBANK_OK) { emit updateConsole(FSBank_ErrorString(result)); }
            continue;
        }

        // ==========================================
        // PREPARING SUBSOUND STRUCTURES (FIXED COMPILER ERROR)
        // ==========================================
        // Keeps the raw underlying QByteArrays alive in memory during the execution phase
        QVector<QByteArray> wavFilePathsBytes;
        wavFilePathsBytes.reserve(wavFiles.size());

        // Stores standard modifiable lvalue pointer types to satisfy C-linkage parameter rules
        std::vector<char*> wavFilePointers(wavFiles.size());

        for (int j = 0; j < wavFiles.size(); ++j)
        {
            QString wavFilePath = QString("%1%2/%3")
            .arg(wavDir,
                 wavFileInfo.completeBaseName(),
                 wavFiles[j]);

            wavFilePathsBytes.append(wavFilePath.toUtf8());

            // Extract the persistent data array address safely without triggering rvalue compilation errors
            wavFilePointers[j] = const_cast<char*>(wavFilePathsBytes.last().constData());

            auto &subsound = subsounds.emplace_back();
            std::memset(&subsound, 0, sizeof(FSBANK_SUBSOUND));

            subsound.numFiles = 1;
            subsound.fileNames = &wavFilePointers[j];
        }

        QByteArray fsbFilePathArray = fsbFilePath.toUtf8();
        char* outputFile = fsbFilePathArray.data();

        // ==========================================
        // ENCRYPTION & PASSWORD HANDLING
        // ==========================================
        QByteArray encryptionKeyArray;
        char* encryption = nullptr;

        if (QFileInfo::exists(passwordTextFile) || QFileInfo::exists(passwordBankTextFile))
        {
            if (QFileInfo::exists(passwordBankTextFile))
                passwordTextFile = passwordBankTextFile;

            QString password = readTextFileToQStringList(passwordTextFile).constFirst();

            if (password.isEmpty()) {
                emit updateConsole(QString("Password file is empty: %1\n").arg(passwordTextFile));
                result = FSBank_Release();
                if (result != FSBANK_OK) { emit updateConsole(FSBank_ErrorString(result)); }
                continue;
            }

            encryptionKeyArray = password.toUtf8();
            encryption = encryptionKeyArray.data();
            emit updateConsole(QString("Encrypting bank file with password: %1\n").arg(encryptionKeyArray));
        }

        // ==========================================
        // BUILD FLAG SETTINGS & BUG FIXES
        // ==========================================
        FSBANK_FORMAT fsbankFormat = FSBANK_FORMAT_VORBIS;

        if (format == "pcm")
            fsbankFormat = FSBANK_FORMAT_PCM;
        else if (format == "fadpcm")
            fsbankFormat = FSBANK_FORMAT_FADPCM;

        FSBANK_BUILDFLAGS fsbankBuildFlags = FSBANK_BUILD_DEFAULT;

        if (defaultSettings == "false")
        {
            if (encodeSyncPoint == "false")
                fsbankBuildFlags |= FSBANK_BUILD_DISABLESYNCPOINTS;

            if (looping == "false")
                fsbankBuildFlags |= FSBANK_BUILD_DONTLOOP;

            if (embededFileNames == "false")
                fsbankBuildFlags |= FSBANK_BUILD_FSB5_DONTWRITENAMES;

            if (writePeakVolume == "true")
                fsbankBuildFlags |= FSBANK_BUILD_WRITEPEAKVOLUME;
        }

        result = FSBank_Build(subsounds.data(), subsounds.size(), fsbankFormat, fsbankBuildFlags, quality, encryption, outputFile);

        if (result != FSBANK_OK)
        {
            emit updateConsole(FSBank_ErrorString(result));
            emit progressUpdated(0);

            result = FSBank_Release();
            if (result != FSBANK_OK) { emit updateConsole(FSBank_ErrorString(result)); }
            continue;
        }

        // Run the progress tracker loop
        bankProgress(wavFiles);

        result = FSBank_Release();
        if (result != FSBANK_OK) {
            emit taskFinished(FSBank_ErrorString(result));
            continue;
        }

        // ==========================================
        // REBUILD AND AUTOMATIC CLEANUP
        // ==========================================
        bankRebuild(bankFilePath, rebuildDir);
        i++;
    }

    emit taskFinished(GlobalErrors::errorToString(ErrorChecks::RebSuccess));
    emit progressUpdated(0);
}

void RebuildWorker::bankProgress(const QStringList wavList) {
    const FSBANK_PROGRESSITEM* progressItem = nullptr;

    int index = 0;

    while (FSBank_FetchNextProgressItem(&progressItem) == FSBANK_OK && progressItem != nullptr) {
        // Safely extract the sound name if the index is valid
        QString soundName = "Global";
        if (progressItem->subSoundIndex >= 0 && progressItem->subSoundIndex < wavList.size()) {
            soundName = wavList[progressItem->subSoundIndex];
        }

        switch (progressItem->state) {
        case FSBANK_STATE_PREPROCESSING:
            emit updateConsole(QString("[%1] Preprocessing...").arg(soundName));
            break;
        case FSBANK_STATE_ANALYSING:
            emit updateConsole(QString("[%1] Analysing...").arg(soundName));
            break;
        case FSBANK_STATE_DECODING:
            emit updateConsole(QString("[%1] Decoding...").arg(soundName));
            break;
        case FSBANK_STATE_ENCODING:
            emit updateConsole(QString("[%1] Encoding...").arg(soundName));
            break;
        case FSBANK_STATE_WRITING:
            emit updateConsole(QString("[%1] Writing to disk...").arg(soundName));
            break;

        case FSBANK_STATE_FINISHED: {
            emit updateConsole(QString("[%1] Finished processing successfully.").arg(soundName));

            // Safely calculate total completion percentage against the wav list
            if (wavList.size() > 0 && progressItem->subSoundIndex >= 0) {
                // Adding 1 ensures progress hits 100% when the last index finishes
                int totalPercent = 100 * (index + 1) / wavList.size();
                emit progressUpdated(totalPercent);
                index++;
            }
            break;
        }

        case FSBANK_STATE_WARNING: {
            // FMOD provides a specific struct cast for warnings via stateData
            if (progressItem->stateData) {
                auto* warnData = static_cast<const FSBANK_STATEDATA_WARNING*>(progressItem->stateData);
                // FIXED: Multi-arg syntax used here to resolve clazy warning
                emit updateConsole(QString("\nWarning on [%1]: %2 (Code: %3)")
                                       .arg(soundName, warnData->warningString, QString::number(warnData->warnCode)));
            } else {
                emit updateConsole(QString("\nWarning, there is an issue with %1").arg(soundName));
            }
            break;
        }

        case FSBANK_STATE_FAILED: {
            // FMOD provides a specific struct cast for failures via stateData
            if (progressItem->stateData) {
                auto* failData = static_cast<const FSBANK_STATEDATA_FAILED*>(progressItem->stateData);
                // FIXED: Multi-arg syntax used here to resolve clazy warning
                emit updateConsole(QString("\nError on [%1]: %2 (Code: %3)")
                                       .arg(soundName, failData->errorString, QString::number(failData->errorCode)));
            } else {
                emit updateConsole(QString("\nError: %1 failed to build.").arg(soundName));
            }
            break;
        }

        default:
            emit updateConsole("\nUnknown progress state encountered.");
            break;
        }

        FSBank_ReleaseProgressItem(progressItem);
        progressItem = nullptr;

        // Prevent thread starvation / high CPU lock
        QThread::msleep(10);
    }
}

void RebuildWorker::bankRebuild(const QString bankFile, const QString buildPath)
{
    // ==========================================
    // INPUT FILE VALIDATION & HEADER PARSING
    // ==========================================
    QFile file(bankFile);
    if (!file.open(QIODevice::ReadOnly)) {
        emit taskFinished("\nError opening file: " + bankFile);
        return;
    }

    // Set up binary stream parameters for FMOD .bank file specification
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    in.setByteOrder(QDataStream::LittleEndian);

    // Validate 4-byte RIFF container signature
    const QByteArray magicArray = readBytes(in, 4);
    if (magicArray != "RIFF") {
        emit taskFinished(GlobalErrors::errorToString(ErrorChecks::InvalidRIFF));
        return;
    }

    // Jump to offset 0x08 and validate 4-byte FEV (FMOD Event File) identifier string
    file.seek(0x08);
    const QByteArray fevStringArray = readBytes(in, 4);
    if (fevStringArray != "FEV ") {
        emit taskFinished(GlobalErrors::errorToString(ErrorChecks::InvalidFEV));
        return;
    }

    // Jump to offset 0x14 and extract structural architecture layout version version integer
    file.seek(0x14);
    quint32 version;
    in >> version;
    if (version == 0) {
        emit taskFinished(GlobalErrors::errorToString(ErrorChecks::InvalidVersion));
        return;
    }

    // Jump to offset 0x1C and validate structural sub-container LIST chunk layout marker
    file.seek(0x1c);
    const QByteArray listStringArray = readBytes(in, 4);
    if (listStringArray != "LIST") {
        emit taskFinished(GlobalErrors::errorToString(ErrorChecks::InvalidLIST));
        return;
    }

    // Skip ahead past size field to target specific nested structural metadata definitions
    file.seek(file.pos() + 0x04);
    const QByteArray projStringArray = readBytes(in, 4);
    if (projStringArray != "PROJ") {
        emit taskFinished(GlobalErrors::errorToString(ErrorChecks::InvalidPROJ));
        return;
    }

    // Validate target audio file index block marker token element string identifier
    const QByteArray BnkiStringArray = readBytes(in, 4);
    if (BnkiStringArray != "BNKI") {
        emit taskFinished(GlobalErrors::errorToString(ErrorChecks::InvalidBNKI));
        return;
    }

    // ==========================================
    // CHUNK LAYOUT PARSING & OFFSET SCANNING
    // ==========================================
    QVector<quint32> sndh_fsbOffset, sndh_fsbSize, snd_location, snd_buffer;
    quint32 chunk_size, sndh_unknown = 0, fsbCount = 1, sndh_location = 0;

    // Read the primary project header sizing envelope property information
    in >> chunk_size;
    file.seek(file.pos() + chunk_size);

    // Initialise array memory vectors before stepping into loop structure operations
    sndh_fsbOffset.resize(1);
    sndh_fsbOffset[0] = 0;

    sndh_fsbSize.resize(1);
    sndh_fsbSize[0] = 0;

    snd_location.resize(1);
    snd_location[0] = 0;

    // Walk sequentially through chunk descriptors until the primary audio target location matches
    while (snd_location[0] == 0 && file.pos() < file.size()) {
        quint32 chunk_type;
        in >> chunk_type;
        in >> chunk_size;

        // Guard against corrupted binary structures or infinite file stream pointer failures
        if (chunk_type == 0xFFFFFFFF || chunk_size == 0xFFFFFFFF) {
            emit taskFinished(GlobalErrors::errorToString(ErrorChecks::InvalidChunk));
            return;
        }

        switch(chunk_type)
        {
        case 0x48444E53: /* "SNDH" - Sound Header containing sound block arrays */
        {
            // Each sub-element index record occupies 8 bytes of structural boundary layout space
            fsbCount = (chunk_size - 4) / 8;

            sndh_fsbOffset.resize(fsbCount);
            sndh_fsbSize.resize(fsbCount);

            in >> sndh_unknown;
            sndh_location = file.pos(); // Store address position to patch updated headers later

            // Extract positional indices and sizing limits for all nested audio objects
            for (quint32 j = 0; j < fsbCount; j++)
            {
                in >> sndh_fsbOffset[j];
                in >> sndh_fsbSize[j];
            }
            continue;
        }
        case 0x4C425453: /* "STBL" - String Table block sequence wrapper */
        {
            quint64 currentPos = file.pos();

            if (chunk_size != 0)
            {
                file.seek(currentPos + chunk_size);
                quint32 chunkTypeHash = 0;
                in >> chunkTypeHash;

                // Verify whether adjacent data items override expected padding structural layouts
                switch(chunkTypeHash)
                {
                case 0x20444E53: /* "SND " */
                case 0x48534148: /* "HASH" */
                    break;
                default:
                    chunk_size += 1; // Readjust stream alignment boundary offset metrics
                    break;
                }
            }
            file.seek(currentPos + chunk_size);
            continue;
        }
        case 0x20444E53: /* "SND " - Sound data chunk data definition area */
        {
            snd_location.resize(fsbCount);
            snd_location[0] = file.pos() - 8;
            snd_buffer.resize(fsbCount);
            snd_buffer[0] = chunk_size - sndh_fsbSize[0];

            // If processing multi-file compound banks, map out secondary offset tracks
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

    // Verify structural data locations parsed correctly before generating binary outputs
    QString bankName = file.fileName();
    if (sndh_fsbOffset[0] == 0 || sndh_fsbSize[0] == 0) { emit taskFinished(GlobalErrors::errorToString(ErrorChecks::RebSndhOffsetSizeError) + bankName); return; }
    if (sndh_location == 0) { emit taskFinished(GlobalErrors::errorToString(ErrorChecks::RebSndhLocationError) + bankName); return; }
    if (snd_location[0] == 0) { emit taskFinished(GlobalErrors::errorToString(ErrorChecks::RebSndLocationError) + bankName); return; }

    // Read the original un-modified initial bank header content up to the first FSB container block
    file.seek(0);
    QByteArray bankHeader = readBytes(in, sndh_fsbOffset[0]);
    file.close();

    // ==========================================
    // TARGET FILE PREPARATION & SIZE MAPPING
    // ==========================================
    QFileInfo fileInfo(bankName);
    QString bankNameTmp = fileInfo.fileName();
    QFile bankoutFile(buildPath + bankNameTmp);
    if (!bankoutFile.open(QIODevice::WriteOnly)) {
        emit taskFinished(QString("\nRebuilding Bank Error, writing to: %1%2").arg(buildPath, bankNameTmp));
        return;
    }

    // Write original unaltered layout preamble headers straight into the output bank package structure
    bankoutFile.write(bankHeader, sndh_fsbOffset[0]);

    QVector<quint32> fsbSizes;
    fsbSizes.resize(fsbCount);
    fsbSizes[0] = 0;

    QString fsbFileName = QString("%1/fsb/%2").arg(QCoreApplication::applicationDirPath(), bankNameTmp.replace(".bank", ""));

    // Verify the newly compiled FSB files exist on disk and record their new sizes
    for (quint32 i = 0; i < fsbCount; i++)
    {
        QString fsbFilePath = QString("%1[%2].fsb").arg(fsbFileName).arg(i);
        QFileInfo fileInfo(fsbFilePath);

        if (fileInfo.exists()) {
            fsbSizes[i] = (quint32)fileInfo.size();
        } else {
            emit taskFinished(GlobalErrors::errorToString(ErrorChecks::RebNoFSBFound) + fsbFilePath);
            return;
        }
    }

    // Calculate shifting structural offset metrics for compounds with multiple child FSB data blocks
    if (fsbCount > 1)
    {
        for (quint32 i = 0; i < fsbCount - 1; i++)
        {
            sndh_fsbOffset[i + 1] = sndh_fsbOffset[i] + fsbSizes[i] + snd_buffer[i + 1] + 8;
        }
    }

    // ==========================================
    // HEADER INJECTION & AUDIO RE-ASSEMBLY
    // ==========================================
    // Jump to the metadata table position and inject the new pointer maps and sizes
    bankoutFile.seek(sndh_location);
    for (quint32 i = 0; i < fsbCount; i++)
    {
        bankoutFile.write(reinterpret_cast<const char*>(&sndh_fsbOffset[i]), 4);
        bankoutFile.write(reinterpret_cast<const char*>(&fsbSizes[i]), 4);
    }

    bankoutFile.flush();
    bankoutFile.seek(snd_location[0]); // Relocate file pointer to the start of the audio segment data space

    // Stitch the freshly generated FSB container files directly into the target banking payload
    for (quint32 i = 0; i < fsbCount; i++)
    {
        QString fsbFilePath = QString("%1[%2].fsb").arg(fsbFileName).arg(i);
        QFile fsbInFile(fsbFilePath);
        if (!fsbInFile.open(QIODevice::ReadOnly)) {
            emit taskFinished(QString("\nRebuilding Bank Error, reading: %1").arg(fsbFilePath));
            return;
        }

        QDataStream fsbIn(&fsbInFile);
        fsbIn.setVersion(QDataStream::Qt_DefaultCompiledVersion);
        fsbIn.setByteOrder(QDataStream::LittleEndian);

        // Standardise target signature fields and map tracking parameters down to byte alignment rules
        bankoutFile.write("SND ");
        quint32 fsbTmpSize = fsbSizes[i] + snd_buffer[i];
        bankoutFile.write(reinterpret_cast<const char*>(&fsbTmpSize), 4);

        // Zero-fill padding space blocks depending on technical header requirements
        quint32 bufferSize = snd_buffer[i];
        QByteArray buffer(bufferSize, '\0');

        if (bufferSize != 0)
            bankoutFile.write(buffer);

        // Stream raw binary blocks out of the independent file instances into the main container asset
        quint32 chunkCount = fileio::chunkAmount(fsbSizes[i]);
        std::vector _chunkSizes = fileio::chunkSizes(fsbSizes[i], chunkCount);

        for (unsigned int k = 0; k < chunkCount; k++) {
            QByteArray fsbBuffer = readBytes(fsbIn, _chunkSizes[k]);
            bankoutFile.write(fsbBuffer);
        }

        fsbInFile.close();
    }

    // Patch the global master file layout container size metadata element at offset index position 4
    quint32 headerSize = (bankoutFile.size()) - 8;
    bankoutFile.seek(4);
    bankoutFile.write(reinterpret_cast<const char*>(&headerSize), 4);

    // Commit all cached system operations safely down to disk storage
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
        emit taskFinished(QString("\nCould not open file: %1").arg(filePath));
        return stringList; // Return empty list if file cannot be opened
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        stringList.append(line);
    }

    if (stringList.isEmpty()) // prevent's application crash if password is empty.
        stringList.append("");

    file.close();
    return stringList;
}
