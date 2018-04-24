#ifndef AERON_ARCHIVE_ARCHIVEEXCEPTION_H
#define AERON_ARCHIVE_ARCHIVEEXCEPTION_H

#include <util/Exceptions.h>

namespace aeron::archive {

class ArchiveException : public util::SourcedException
{
public:
    enum class ErrorCode {
        GENERIC,
        ACTIVE_LISTING,
        ACTIVE_RECORDING,
        ACTIVE_SUBSCRIPTION,
        UNKNOWN_SUBSCRIPTION,
        UNKNOWN_RECORDING,
        UNKNOWN_REPLAY,
        MAX_REPLAYS,
        MAX_RECORDINGS,
        INVALID_EXTENSION
    };

    ArchiveException(
            const std::string &what,
            const std::string& function,
            const std::string& where,
            const int line) :
        ArchiveException(what, ErrorCode::GENERIC, function, where, line)
    {
    }

    ArchiveException(
            const std::string &what,
            const ErrorCode code,
            const std::string& function,
            const std::string& where,
            const int line) :
        SourcedException(what, function, where, line),
        m_errorCode(code)
    {
    }

    /**
     * Error code providing more detail into what went wrong.
     *
     * @return code providing more detail into what went wrong.
     */
    [[nodiscard]] ErrorCode errorCode() const
    {
        return m_errorCode;
    }

private:
    const ErrorCode m_errorCode;
};

}

#endif // AERON_ARCHIVE_ARCHIVEEXCEPTION_H
