#ifndef FILEIO_H
#define FILEIO_H
#include <QFile>
#include <QDebug>
#include <cmath>
#include <vector>

class fileio
{
public:
    static quint32 chunkAmount(quint32 unCompressedSize, quint32 chunkSize = 262144);
    static std::vector<quint64> chunkSizes(quint32 unCompressedSize, qint32 chunkAmount, quint32 chunkSize = 262144);

private:
    fileio() = delete;
    fileio(const fileio&) = delete;
    fileio& operator=(const fileio&) = delete;
};

#endif // FILEIO_H
