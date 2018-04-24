#ifndef AERON_ARCHIVE_RECORDINGDESCRIPTORCONSUMER_H
#define AERON_ARCHIVE_RECORDINGDESCRIPTORCONSUMER_H

#include <functional>
#include <string>

namespace aeron::archive {

/**
 * A recording descriptor returned as a result of requesting a listing of recordings.
 *
 * @param controlSessionId  of the originating session requesting to list recordings.
 * @param correlationId     of the associated request to list recordings.
 * @param recordingId       of this recording descriptor.
 * @param startTimestamp    for the recording.
 * @param stopTimestamp     for the recording.
 * @param startPosition     for the recording against the recorded publication.
 * @param stopPosition      reached for the recording.
 * @param initialTermId     for the recorded publication.
 * @param segmentFileLength for the recording which is a multiple of termBufferLength.
 * @param termBufferLength  for the recorded publication.
 * @param mtuLength         for the recorded publication.
 * @param sessionId         for the recorded publication.
 * @param streamId          for the recorded publication.
 * @param strippedChannel   for the recorded publication.
 * @param originalChannel   for the recorded publication.
 * @param sourceIdentity    for the recorded publication.
 */
using on_recording_descriptor_t = std::function<void(
    std::int64_t, // controlSessionId
    std::int64_t, // correlationId
    std::int64_t, // recordingId
    std::int64_t, // startTimestamp
    std::int64_t, // stopTimestamp
    std::int64_t, // startPosition
    std::int64_t, // stopPosition
    std::int32_t, // initialTermId
    std::int32_t, // segmentFileLength
    std::int32_t, // termBufferLength
    std::int32_t, // mtuLength
    std::int32_t, // sessionId
    std::int32_t, // streamId
    const std::string&, // strippedChannel
    const std::string&, // originalChannel
    const std::string&)>; // sourceIdentity

}

#endif // AERON_ARCHIVE_RECORDINGDESCRIPTORCONSUMER_H
