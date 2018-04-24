#ifndef AERON_ARCHIVE_ERRORHANDLER_H
#define AERON_ARCHIVE_ERRORHANDLER_H

#include <functional>

#include "ArchiveException.h"

namespace aeron::archive {

using on_error_t = std::function<void(const ArchiveException&)>;

}

#endif // AERON_ARCHIVE_ERRORHANDLER_H
