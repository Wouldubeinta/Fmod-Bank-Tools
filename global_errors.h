#ifndef GLOBAL_ERRORS_H
#define GLOBAL_ERRORS_H

#include <QObject>

enum class ErrorChecks {
    Success = 0,
    ExtSuccess,
    RebSuccess,
    BankFileOpenError,
    BankFileNotFound,
    PasswordFileNotFound,
    PasswordEmpty,
    TextWavFileOpenFailure,
    WriteFailure,
    NoFSBFoundInBank,
    RebNoFSBFound,
    IsEncrypted,
    InvalidRIFF,
    InvalidFEV,
    InvalidVersion,
    InvalidLIST,
    InvalidPROJ,
    InvalidBNKI,
    InvalidChunk,
    ExtSndhOffsetSizeError,
    RebSndhOffsetSizeError,
    RebSndhLocationError,
    RebSndLocationError
};

class GlobalErrors
{

public:
    static QString errorToString(ErrorChecks error);
};

#endif // GLOBAL_ERRORS_H
