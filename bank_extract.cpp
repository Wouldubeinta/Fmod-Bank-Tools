#include "bank_extract.h"
#include "qdebug.h"
#include "fileio.h"
#include "qfileinfo.h"
#include <QVector>
#include <QtGlobal>

ErrorChecks bank_extract::extract(QString bankPath, quint32 &fsbCount)
{
    ErrorChecks results = ErrorChecks::Success;
    QFile file(bankPath);

    if (!file.open(QIODevice::ReadOnly)) {
        return ErrorChecks::BankFileNotFound; // File open error
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    in.setByteOrder(QDataStream::LittleEndian);

    QString magic = readString(in, 4);
    if (magic != "RIFF") {
        return ErrorChecks::InvalidRIFF; // Invalid RIFF magic
    }

    file.seek(0x08);
    QString fevString = readString(in, 4);
    if (fevString != "FEV ") {
        return ErrorChecks::InvalidFEV; // Invalid FEV magic
    }

    file.seek(0x14);
    quint32 version;
    in >> version;
    if (version == 0) {
        return ErrorChecks::InvalidVersion; // Invalid Bank version
    }

    file.seek(0x1c);
    QString listString = readString(in, 4);
    if (listString != "LIST") {
        return ErrorChecks::InvalidLIST; // Invalid LIST
    }

    file.seek(file.pos() + 0x04);
    QString projString = readString(in, 4);
    if (projString != "PROJ") {
        return ErrorChecks::InvalidPROJ; // Invalid PROJ
    }

    QString BnkiString = readString(in, 4);
    if (BnkiString != "BNKI") {
        return ErrorChecks::InvalidBNKI; // Invalid BNKI
    }

    QVector<quint32> sndh_fsbOffset;
    QVector<quint32> sndh_fsbSize;
    quint32 sndh_unknown = 0;
    quint32 chunk_size;
    quint32 _fsbCount = 1;

    in >> chunk_size;
    file.seek(file.pos() + chunk_size);

    sndh_fsbOffset.resize(1);
    sndh_fsbOffset[0] = 0;

    sndh_fsbSize.resize(1);
    sndh_fsbSize[0] = 0;

    while (sndh_fsbOffset[0] == 0 && file.pos() < file.size()) {
        quint32 chunk_type;
        in >> chunk_type;
        in >> chunk_size;

        if (chunk_type == 0xFFFFFFFF || chunk_size == 0xFFFFFFFF) {
            return ErrorChecks::InvalidChunk; // Invalid chunk
        }

        if (chunk_type == 0x48444E53) { // "SNDH"
            if (chunk_size == 0)
                return ErrorChecks::NoFSBFoundInBank; // Doesn't have fsb's in bank
            _fsbCount = (chunk_size - 4) / 8;
            fsbCount = _fsbCount;
            sndh_fsbOffset.resize(_fsbCount);
            sndh_fsbSize.resize(_fsbCount);
            in >> sndh_unknown;

            for (quint32 j = 0; j < _fsbCount; j++)
            {
                in >> sndh_fsbOffset[j];
                in >> sndh_fsbSize[j];
            }
        }

        file.seek(file.pos() + chunk_size);
    }

    if (sndh_fsbOffset[0] == 0 || sndh_fsbSize[0] == 0) {
        return ErrorChecks::ExtSndhOffsetSizeError; // FSB offset or size is zero
    }

    QString bankName = file.fileName();
    QFileInfo fileInfo(bankName);
    QString fsbNameTmp = fileInfo.fileName().replace(".bank", "");

    for (quint32 j = 0; j < _fsbCount; j++)
    {
        if (j == 0)
        {
            file.seek(sndh_fsbOffset[j]);
            QString fsbMagic = readString(in, 4);
            if (fsbMagic != "FSB5")
                results = ErrorChecks::IsEncrypted;  // fsb is encrypted
        }

        file.seek(sndh_fsbOffset[j]);

        quint32 chunkCount = fileio::chunkAmount(sndh_fsbSize[j]);
        std::vector<quint64> _chunkSizes = fileio::chunkSizes(sndh_fsbSize[j], chunkCount);

        QFile fsboutFile(QCoreApplication::applicationDirPath() + "/fsb/" + fsbNameTmp + "[" + QString::number(j) + "].fsb");

        if (!fsboutFile.open(QIODevice::WriteOnly)) {
            return ErrorChecks::WriteFailure; // File write error
        }

        for (unsigned int k = 0; k < chunkCount; k++)
        {
            QByteArray fsbChunkData = file.read(_chunkSizes[k]);

            fsboutFile.write(fsbChunkData);
            fsboutFile.flush();
        }

        fsboutFile.close();
    }

    file.close();
    return results;
}

QString bank_extract::readString(QDataStream &in, int length) {
    std::vector<char> buffer(length);
    in.readRawData(buffer.data(), length);
    return QString::fromUtf8(buffer.data(), length);
}
