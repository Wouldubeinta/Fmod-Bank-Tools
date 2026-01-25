#include <qdebug.h>
#include "fileio.h"
#include <vector>
#include <cmath>

quint32 fileio::chunkAmount(quint32 unCompressedSize, quint32 chunkSize) {
    if (chunkSize == 0) {
        throw std::invalid_argument("chunkSize must be greater than 0");
    }

    // Calculate the number of chunks needed
    quint32 chunkCount = unCompressedSize / chunkSize;

    // If there is any remainder, we need an additional chunk
    if (unCompressedSize % chunkSize != 0) {
        chunkCount++;
    }

    return (unCompressedSize == 0) ? 0 : chunkCount; // Return 0 for zero size input
}

std::vector<quint64> fileio::chunkSizes(quint32 unCompressedSize, qint32 chunkAmount, quint32 chunkSize) {
    std::vector<quint64> chunkS(chunkAmount, 0); // Initialize with zeros

    // Handle case where the uncompressed size is less than the chunk size
    if (unCompressedSize < chunkSize) {
        chunkS[0] = unCompressedSize;
        return chunkS; // Early return
    }

    // Calculate the number of full chunks that can be created
    int fullChunks = unCompressedSize / chunkSize;
    int remainder = unCompressedSize % chunkSize;

    // If the number of chunks is equal to the full chunks, fill them
    if (fullChunks >= chunkAmount) {
        for (int i = 0; i < chunkAmount; ++i) {
            chunkS[i] = chunkSize;
        }
        return chunkS; // Early return
    }

    // Fill the full chunks
    for (int i = 0; i < fullChunks; ++i) {
        chunkS[i] = chunkSize;
    }

    // Handle the last chunk
    if (fullChunks < chunkAmount) {
        chunkS[fullChunks] = remainder; // Assign remainder to the last chunk
    }

    return chunkS;
}
