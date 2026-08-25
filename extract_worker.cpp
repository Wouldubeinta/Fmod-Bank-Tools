#include "extract_worker.h"
#include "fileio.h"
#include "bank_extract.h"

ExtractWorker::ExtractWorker(QObject *parent) : QObject(parent) {}

void ExtractWorker::extract_fsb()
{
    ErrorChecks errorChecks = ErrorChecks::Success;
    FMOD_RESULT result;
    FMOD_SYSTEM *system = nullptr;
    FMOD_SOUND *sound = nullptr;

    // Initialize FMOD extended info structure
    FMOD_CREATESOUNDEXINFO exinfo = {};
    memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.length = 0;

    // Load directory paths from config.ini
    QString config = QString("%1/config.ini").arg(QCoreApplication::applicationDirPath());
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup("Directorys");
    QString fsbDir = QString("%1/fsb/").arg(QCoreApplication::applicationDirPath());
    QString bankDir = QString("%1/").arg(fileio::resolveFolderPath(settings.value("BankDir").toString()));
    QString wavDir = QString("/").arg(fileio::resolveFolderPath(settings.value("WavDir").toString()));
    settings.endGroup();

    // Scan the bank directory for all .bank files
    QDir bank_directory(bankDir);
    QStringList bankFileList = bank_directory.entryList(QStringList() << "*.bank");

    int i = 0;

    // Iterate through every detected FMOD bank file
    for (QString &bankFile : bankFileList)
    {
        QString bankPath = bankDir + bankFile;
        QFileInfo bankFileInfo(bankPath);
        quint32 fsbCount;

        // Extract raw FSB files out of the FMOD bank
        errorChecks = bank_extract::extract(bankPath, fsbCount);

        QString newLineCheck = (i == 0) ? "" : "\n";
        emit updateConsole(QString("%1***** Initializing Fmod Bank file - %2 *****\n").arg(newLineCheck, QFileInfo(bankPath).fileName()));

        // Handle bank extraction errors (e.g., bank is encrypted or file not found errors!!!!)
        if (errorChecks != ErrorChecks::Success)
        {
            errorChecks = handleExtractionError(errorChecks, bankFile, bankPath, exinfo);

            if (errorChecks != ErrorChecks::Success)
            {
                // If handleExtractionError allocated a key before failing,
                // free it and reset to nullptr so it doesn't leak or crash the next loop iteration.
                delete[] exinfo.encryptionkey;
                exinfo.encryptionkey = nullptr;
                continue;
            }
        }

        QString fsbName = QFileInfo(bankPath).fileName().replace(".bank", "");

        // Process each FSB sub-file inside the bank
        for (quint32 j = 0; j < fsbCount; j++)
        {
            QString fsbPath = QString("%1%2[%3].fsb").arg(fsbDir, fsbName, QString::number(j));

            if (!QFileInfo::exists(fsbPath))
            {
                emit updateConsole(QString("Error, %1[%2].fsb file is missing !!!").arg(fsbName, QString::number(j)));
                continue;
            }

            // Reset loop-scoped pointers to prevent corruption from previous passes
            system = nullptr;
            sound = nullptr;

            // Initialize a temporary FMOD system context for this FSB file
            result = FMOD_System_Create(&system);
            if (result != FMOD_OK)
            {
                emit updateConsole(FMOD_ErrorString(result));
                continue;
            }

            result = FMOD_System_Init(system, 1, FMOD_INIT_NORMAL, nullptr);
            if (result != FMOD_OK)
            {
                emit updateConsole(FMOD_ErrorString(result));
                FMOD_System_Release(system); // Prevent FMOD system handle leak
                continue;
            }

            // Open the FSB file using FMOD to parse its sound structures
            result = FMOD_System_CreateSound(system, fsbPath.toUtf8().constData(), FMOD_OPENONLY, &exinfo, &sound);
            if (result != FMOD_OK)
            {
                emit updateConsole(FMOD_ErrorString(result));
                FMOD_System_Release(system); // Prevent FMOD system handle leak
                continue;
            }

            // Create target folder for the exported wav files
            QDir dir(wavDir);
            dir.mkdir(QString("%1[%2]").arg(bankFileInfo.fileName().replace(".bank", ""), QString::number(j)));

            emit updateConsole(QString("\nExtracting fsb file - %1\n").arg(QFileInfo(fsbPath).fileName()));

            // Extract the actual audio files inside the FSB container
            processSubSounds(sound, bankFileInfo, wavDir, j);

            // Ensures resources close gracefully on both success or failure
            if (sound)
                FMOD_Sound_Release(sound);
            if (system)
                FMOD_System_Release(system);
        }

        // Delete the allocation and clear the pointer address.
        delete[] exinfo.encryptionkey;
        exinfo.encryptionkey = nullptr;
        i++;
    }

    // Report final worker results to UI threads
    if (errorChecks == ErrorChecks::Success)
    {
        emit taskFinished(GlobalErrors::errorToString(ErrorChecks::ExtSuccess));
        emit progressUpdated(0);
    }
    else if (bankFileList.count() == 0)
    {
        emit taskFinished(GlobalErrors::errorToString(ErrorChecks::BankFileNotFound));
        emit progressUpdated(0);
    }
    else
        emit progressUpdated(0);
}

ErrorChecks ExtractWorker::handleExtractionError(ErrorChecks errorChecks, const QString &bankFile, QString bankPath, FMOD_CREATESOUNDEXINFO &exinfo)
{
    switch (errorChecks)
    {
        case ErrorChecks::InvalidRIFF:
            emit updateConsole(GlobalErrors::errorToString(ErrorChecks::InvalidRIFF) + bankFile);
            break;
        case ErrorChecks::InvalidFEV:
            emit updateConsole(GlobalErrors::errorToString(ErrorChecks::InvalidFEV) + bankFile);
            break;
        case ErrorChecks::InvalidVersion:
            emit updateConsole(GlobalErrors::errorToString(ErrorChecks::InvalidVersion) + bankFile);
            break;
        case ErrorChecks::InvalidLIST:
            emit updateConsole(GlobalErrors::errorToString(ErrorChecks::InvalidLIST) + bankFile);
            break;
        case ErrorChecks::InvalidPROJ:
            emit updateConsole(GlobalErrors::errorToString(ErrorChecks::InvalidPROJ) + bankFile);
            break;
        case ErrorChecks::InvalidBNKI:
            emit updateConsole(GlobalErrors::errorToString(ErrorChecks::InvalidBNKI) + bankFile);
            break;
        case ErrorChecks::InvalidChunk:
            emit updateConsole(GlobalErrors::errorToString(ErrorChecks::InvalidChunk) + bankFile);
            break;
        case ErrorChecks::ExtSndhOffsetSizeError:
            emit updateConsole(GlobalErrors::errorToString(ErrorChecks::ExtSndhOffsetSizeError) + bankFile);
            break;
        case ErrorChecks::NoFSBFoundInBank:
            emit updateConsole(GlobalErrors::errorToString(ErrorChecks::NoFSBFoundInBank) + bankFile);
            break;
        case ErrorChecks::IsEncrypted:
            return handlePasswordProtectedBank(bankPath, exinfo);
            break;
        default:
            emit updateConsole(QString("Unknown error with bank file: %1").arg(bankFile));
            break;
    }
    return ErrorChecks::Success;
}

ErrorChecks ExtractWorker::handlePasswordProtectedBank(QString bankPath, FMOD_CREATESOUNDEXINFO &exinfo)
{
    QFileInfo bankInfo(bankPath);
    QString passwordTextFile = QDir(bankInfo.path()).filePath("password.txt");
    QString passwordBankTextFile = QDir(bankInfo.path()).filePath(QString("%1.txt").arg(bankInfo.baseName()));

    // Determine the correct password file to use.
    if (QFileInfo::exists(passwordTextFile)) {
        if (QFileInfo::exists(passwordBankTextFile))
            passwordTextFile = passwordBankTextFile; // Prefer bank-specific password file for bank.
    }
    else if (QFileInfo::exists(passwordBankTextFile))
        passwordTextFile = passwordBankTextFile; // Use bank-specific password file if default is missing.
    else {
        emit updateConsole(QString("%1%2 with password for decryption.").arg(GlobalErrors::errorToString(ErrorChecks::PasswordFileNotFound), passwordBankTextFile));
        return ErrorChecks::PasswordFileNotFound; // Indicate password.txt failure
    }

    QString password = readTextFileToQStringList(passwordTextFile).constFirst(); // read first line in text file for password.

    // Handle empty password case.
    if (password.isEmpty()) {
        emit updateConsole(QString("%1%2\n").arg(GlobalErrors::errorToString(ErrorChecks::PasswordEmpty), passwordTextFile));
        return ErrorChecks::PasswordEmpty; // Indicate password empty failure
    }

    // Convert password to QByteArray and manage memory safely
    QByteArray encryptionKeyArray = password.toUtf8();
    char* encryption = new char[encryptionKeyArray.size() + 1];
    std::memcpy(encryption, encryptionKeyArray.constData(), encryptionKeyArray.size() + 1);
    exinfo.encryptionkey = encryption;

    // Emit console update based on password availability
    emit updateConsole(QString("Decrypting bank file with password: %1").arg(encryptionKeyArray));
    return ErrorChecks::Success;
}

void ExtractWorker::processSubSounds(FMOD_SOUND *sound, QFileInfo bankFileInfo, const QString &wavDir, quint32 fsbIndex)
{
    FMOD_RESULT result;
    int numsubsounds = 0;
    QString wavPath = QString("%1%2[%3]/")
                          .arg(wavDir,
                               bankFileInfo.fileName().replace(".bank", ""),
                               QString::number(fsbIndex));

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
        emit updateConsole(FMOD_ErrorString(result));
        return;
    }

    QStringList txtFileNames;

    for (int j = 0; j < numsubsounds; j++)
    {
        // Initialize pointers to nullptr to safely handle cleanups
        FMOD_SOUND *sound_to_play = nullptr;
        FMOD_SOUND_TYPE   stype;
        FMOD_SOUND_FORMAT sformat;

        // Increased nameLength from 64 to 256
        unsigned int length = 0, dataLen = 0, nameLength = 256;
        int schannels = 0, sbits = 0, priority = 0;
        // Increased subsoundsName from 64 to 256
        char             subsoundsName[256];
        char*            buffer = nullptr;
        float            ssamplerate = 0;

        // Fetch the subsound handle from FMOD container
        result = FMOD_Sound_GetSubSound(sound, j, &sound_to_play);
        if (result != FMOD_OK)
        {
            emit updateConsole(FMOD_ErrorString(result));
            return; // No handle allocated yet, safe to return early
        }

        // --- Beyond this point, sound_to_play MUST be released if an error occurs ---

        result = FMOD_Sound_SeekData(sound_to_play, 0);
        if (result != FMOD_OK)
        {
            emit updateConsole(FMOD_ErrorString(result));
            FMOD_Sound_Release(sound_to_play);
            return;
        }

        result = FMOD_Sound_GetDefaults(sound_to_play, &ssamplerate, &priority);
        if (result != FMOD_OK)
        {
            emit updateConsole(FMOD_ErrorString(result));
            FMOD_Sound_Release(sound_to_play);
            return;
        }

        result = FMOD_Sound_GetFormat(sound_to_play, &stype, &sformat, &schannels, &sbits);
        if (result != FMOD_OK)
        {
            emit updateConsole(FMOD_ErrorString(result));
            FMOD_Sound_Release(sound_to_play);
            return;
        }

        result = FMOD_Sound_GetLength(sound_to_play, &length, FMOD_TIMEUNIT_PCMBYTES);
        if (result != FMOD_OK)
        {
            emit updateConsole(FMOD_ErrorString(result));
            FMOD_Sound_Release(sound_to_play);
            return;
        }

        result = FMOD_Sound_GetName(sound_to_play, subsoundsName, nameLength);
        if (result != FMOD_OK)
        {
            emit updateConsole(FMOD_ErrorString(result));
            FMOD_Sound_Release(sound_to_play);
            return;
        }

        QString subsoundName = QString::fromUtf8(subsoundsName);

        // If no filename, generate a fallback name
        if (subsoundName.isEmpty())
            subsoundName = QString("sound_%1").arg(QString::number(j));

        QDir dir(wavPath);
        QString baseName = subsoundName;
        QString fileName = QString("%1.wav").arg(baseName);

        // Loop until a unique filename is found
        for (int suffix = j; dir.exists(fileName); ++suffix) {
            subsoundName = QString("%1_%2").arg(baseName, QString::number(suffix));
            fileName = QString("%1.wav").arg(subsoundName);
        }

        QString wavName = dir.absoluteFilePath(fileName);
        QFile file(wavName);

        // Check if the wav file opens for writing
        if (!file.open(QIODevice::WriteOnly)) {
            emit updateConsole("Wav File is not open for writing.");
            FMOD_Sound_Release(sound_to_play);
            return;
        }

        // RIFF/WAVE HEADER SETUP
        // Assemble standard RIFF specification header using calculated stream dimensions.
        WAVHeader header;
        header.headerLength = 38 + length;
        header.numChannels = schannels;
        header.sampleRate = ssamplerate;
        int16_t bytesPerSample = sbits / 8;
        header.bytesPerSecond = ssamplerate * schannels * bytesPerSample;
        header.bytesPerSample = bytesPerSample;
        header.bitsPerChannel = sbits;
        header.dataSize = length;

        // Commit the header payload block directly to the start of the WAV file.
        qint64 headerError = file.write(reinterpret_cast<const char*>(&header), sizeof(header)) != sizeof(header);

        if (headerError == -1) {
            file.close();
            emit updateConsole("Error: Failed to write wav header data!!!");
            emit progressUpdated(0);
            return;
        }

        quint32 chunkCount = fileio::chunkAmount(length);
        std::vector<quint64> _chunkSizes = fileio::chunkSizes(length, chunkCount);

        bool chunkError = false;

        // Process audio stream chunks
        for (unsigned int k = 0; k < chunkCount; k++)
        {
            buffer = (char*)malloc(_chunkSizes[k]);
            if (buffer == nullptr)
            {
                emit updateConsole("Error: Memory allocation failed for audio data chunk.");
                emit progressUpdated(0);
                chunkError = true;
                break;
            }

            result = FMOD_Sound_ReadData(sound_to_play, buffer, _chunkSizes[k], &dataLen);
            if (result != FMOD_OK)
            {
                emit updateConsole(FMOD_ErrorString(result));
                emit progressUpdated(0);
                free(buffer);
                buffer = nullptr;
                chunkError = true;
                break;
            }

            qint64 chunkWriteError = file.write(buffer, _chunkSizes[k]);

            if (chunkWriteError == -1)
            {
                file.close();
                emit updateConsole("Error: Failed to write raw wav chunk audio data!!!");
                emit progressUpdated(0);
                free(buffer);
                buffer = nullptr;
                chunkError = true;
                break;
            }

            free(buffer); // Safely release the individual chunk heap allocation
            buffer = nullptr;
        }

        // Explicitly close the file handle
        file.flush();
        file.close();

        // If an error occurred inside the chunk loop, clean up the sub sound and exit
        if (chunkError)
        {
            FMOD_Sound_Release(sound_to_play);
            return;
        }

        // Post UI statistics updates
        int subSoundsPercent = 100 * (j + 1) / numsubsounds;
        emit progressUpdated(subSoundsPercent);

        QString index = QString::number(j);
        txtFileNames << QString("%1.wav").arg(subsoundName);
        emit updateConsole(QString("%1: (%2.wav) [Extracting]").arg(index, subsoundName));

        // Always release the sub sound handle at the iteration end
        FMOD_Sound_Release(sound_to_play);
    }

    // Save fsb track list to txt file
    writeFilenamesToFile(txtFileNames,
                         QString("%1%2[%3].txt")
                             .arg(wavPath,
                                  bankFileInfo.fileName().replace(".bank", ""),
                                  QString::number(fsbIndex)));

}

void ExtractWorker::writeFilenamesToFile(const QStringList &filenames, const QString &outputFilePath) {
    QFile file(outputFilePath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &filename : filenames) {
            out << filename << "\n"; // Write each filename followed by a newline
        }
        file.close();
    } else {
        emit updateConsole(QString("Error opening file for writing: %1").arg(file.errorString()));
    }
}

QStringList ExtractWorker::readTextFileToQStringList(const QString& filePath) {
    QStringList stringList;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit updateConsole(QString("\nCould not open file: %1").arg(filePath));
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
