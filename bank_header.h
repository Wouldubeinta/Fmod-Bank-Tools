#ifndef BANK_HEADER_H
#define BANK_HEADER_H

#include <QMetaType>
#include <QVector>

struct BankInfo {
    QVector<quint32> sndh_fsbOffset;
    QVector<quint32> sndh_fsbSize;
    QVector<quint32> snd_location;
    QVector<quint32> snd_buffer;
    quint32 chunk_size;
    quint32 sndh_unknown;
    quint32 fsbCount;
    quint32 sndh_location;
};

// Declare the struct as a meta type
Q_DECLARE_METATYPE(BankInfo);

#endif // BANK_HEADER_H
