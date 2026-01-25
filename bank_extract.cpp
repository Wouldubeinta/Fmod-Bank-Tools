#include "bank_extract.h"
#include "qdebug.h"
#include "fileio.h"
#include "qfileinfo.h"
#include <QVector>
#include <QtGlobal>

int bank_extract::extract(QString bankPath, quint32 &fsbCount)
{
    int check = 0;
    QFile file(bankPath);

    if (!file.open(QIODevice::ReadOnly)) {
        return check; // File open error
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    in.setByteOrder(QDataStream::LittleEndian);

    QString magic = readString(in, 4);
    if (magic != "RIFF") {
        return check; // Invalid magic
    }

    file.seek(0x08);
    QString fevString = readString(in, 4);
    if (fevString != "FEV ") {
        return check; // Invalid FEV
    }

    file.seek(0x14);
    quint32 version;
    in >> version;
    if (version == 0) {
        return check; // Invalid version
    }

    file.seek(0x1c);
    QString listString = readString(in, 4);
    if (listString != "LIST") {
        return check; // Invalid LIST
    }

    file.seek(file.pos() + 0x04);
    QString projString = readString(in, 4);
    QString BnkiString = readString(in, 4);
    if (projString != "PROJ" || BnkiString != "BNKI") {
        return check; // Invalid project or BNKI
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
            return check; // Invalid chunk
        }

        if (chunk_type == 0x48444E53) { // "SNDH"
            if (chunk_size == 0)
                return 2; // Doesn't have fsb's in bank
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
        return 2; // FSB offset or size is zero
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
            check = (fsbMagic != "FSB5") ? 5 : 1; // check fsb is encrypted
        }

        file.seek(sndh_fsbOffset[j]);

        quint32 chunkCount = fileio::chunkAmount(sndh_fsbSize[j]);
        std::vector<quint64> _chunkSizes = fileio::chunkSizes(sndh_fsbSize[j], chunkCount);

        QFile fsboutFile(QCoreApplication::applicationDirPath() + "/fsb/" + fsbNameTmp + "[" + QString::number(j) + "].fsb");

        if (!fsboutFile.open(QIODevice::WriteOnly)) {
            return 0; // File write error
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
    return check;
}

QString bank_extract::readString(QDataStream &in, int length) {
    std::vector<char> buffer(length);
    in.readRawData(buffer.data(), length);
    return QString::fromUtf8(buffer.data(), length);
}
