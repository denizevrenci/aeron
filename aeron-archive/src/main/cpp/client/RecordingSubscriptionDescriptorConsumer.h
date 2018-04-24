#ifndef AERON_ARCHIVE_RECORDINGSUBCSRIPTIONDESCRIPTORCONSUMER_H
#define AERON_ARCHIVE_RECORDINGSUBCSRIPTIONDESCRIPTORCONSUMER_H

#include <functional>
#include <string>

namespace aeron::archive {

/**
 * Descriptor for an active recording subscription on the archive.
 *
 * @param controlSessionId  of the originating session requesting to list recordings.
 * @param correlationId     of the associated request to list recordings.
 * @param subscriptionId   that can be used to stop the recording subscription.
 * @param streamId         the subscription was registered with.
 * @param strippedChannel  the subscription was registered with.
 */
using on_recording_subscription_descriptor_t = std::function<void(
    std::int64_t, // controlSessionId
    std::int64_t, // correlationId
    std::int64_t, // subscriptionId
    std::int32_t, // streamId
    const std::string&)>; // strippedChannel

}

#endif // AERON_ARCHIVE_RECORDINGSUBCSRIPTIONDESCRIPTORCONSUMER_H
