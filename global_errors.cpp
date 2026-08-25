#include "global_errors.h"

QString GlobalErrors::errorToString(ErrorChecks error)
{
    switch (error) {
    case ErrorChecks::ExtSuccess:              return QStringLiteral("\nExtracting Bank files has finished.");
    case ErrorChecks::RebSuccess:              return QStringLiteral("\nRebuilding Bank files has finished.");
    case ErrorChecks::BankFileOpenError:       return QStringLiteral("\nFailed to open .bank file.");
    case ErrorChecks::BankFileNotFound:        return QStringLiteral("\nThe source bank file does not exist.");
    case ErrorChecks::PasswordFileNotFound:    return QStringLiteral("\nCan't find password.txt or ");
    case ErrorChecks::PasswordEmpty:           return QStringLiteral("\nPassword file is empty: ");
    case ErrorChecks::TextWavFileOpenFailure:  return QStringLiteral("\nError: could not find txt wav lists !!!");
    case ErrorChecks::WriteFailure:            return QStringLiteral("\nDisk write failure. The output operation was interrupted or file partition is locked.");
    case ErrorChecks::NoFSBFoundInBank:        return QStringLiteral("\nError, can't find any fsb audio in this bank file: ");
    case ErrorChecks::RebNoFSBFound:           return QStringLiteral("\nRebuilding Bank Error, fsb file does not exist ");
    case ErrorChecks::IsEncrypted:             return QStringLiteral("\nFmod Bank file is password protected");
    case ErrorChecks::InvalidRIFF:             return QStringLiteral("\nError, has no RIFF in header");
    case ErrorChecks::InvalidFEV:              return QStringLiteral("\nError, has no FEV in header");
    case ErrorChecks::InvalidVersion:          return QStringLiteral("\nError, version not supported");
    case ErrorChecks::InvalidLIST:             return QStringLiteral("\nError, has no LIST in header");
    case ErrorChecks::InvalidPROJ:             return QStringLiteral("\nError, has no PROJ in header");
    case ErrorChecks::InvalidBNKI:             return QStringLiteral("\nError, has no BNKI in header");
    case ErrorChecks::InvalidChunk:            return QStringLiteral("\nInvalid chunk type or chunk size");
    case ErrorChecks::ExtSndhOffsetSizeError:  return QStringLiteral("\nExtracting Bank Error, sndh_fsbOffset or sndh_fsbSize should not be 0");
    case ErrorChecks::RebSndhOffsetSizeError:  return QStringLiteral("\nRebuilding Bank Error, sndh_fsbOffset or sndh_fsbSize should not be 0 for - ");
    case ErrorChecks::RebSndhLocationError:    return QStringLiteral("\nRebuilding Bank Error, sndh_location should not be 0 for - ");
    case ErrorChecks::RebSndLocationError:     return QStringLiteral("\nRebuilding Bank Error, snd_location should not be 0 for - ");
    default:                                   return QStringLiteral("\nAn unknown error anomaly occurred.");
    }
}
