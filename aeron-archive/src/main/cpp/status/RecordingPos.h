#ifndef AERON_ARCHIVE_STATUS_RECORDINGPOS_H
#define AERON_ARCHIVE_STATUS_RECORDINGPOS_H

#include <concurrent/CountersReader.h>

namespace aeron::archive::recordingPos {

/**
 * The position a recording has reached when being archived.
 * <p>
 * Key has the following layout:
 * <pre>
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Recording ID                           |
 *  |                                                               |
 *  +---------------------------------------------------------------+
 *  |                         Session ID                            |
 *  +---------------------------------------------------------------+
 *  |                Source Identity for the Image                  |
 *  +---------------------------------------------------------------+
 * </pre>
 */
/**
 * Type id of a recording position counter.
 */
constexpr std::int32_t RECORDING_POSITION_TYPE_ID = 100;

constexpr int NULL_VALUE = -1;

/**
 * Represents a null recording id when not found.
 */
constexpr std::int64_t NULL_RECORDING_ID = NULL_VALUE;

/**
 * Human readable name for the counter.
 */
constexpr char NAME[] = "rec-pos";

// Exits in the Java implementation of CountersReader but not in the C++ one.
constexpr std::int32_t NULL_COUNTER_ID = NULL_VALUE;
constexpr std::int32_t TYPE_ID_OFFSET = sizeof(std::int32_t);

constexpr std::int32_t RECORDING_ID_OFFSET = 0;
constexpr std::int32_t SESSION_ID_OFFSET = RECORDING_ID_OFFSET + sizeof(std::int64_t);
constexpr std::int32_t SOURCE_IDENTITY_LENGTH_OFFSET = SESSION_ID_OFFSET + sizeof(std::int64_t);
constexpr std::int32_t SOURCE_IDENTITY_OFFSET = SOURCE_IDENTITY_LENGTH_OFFSET + sizeof(std::int64_t);

/**
 * Find the active counter id for a stream based on the recording id.
 *
 * @param countersReader to search within.
 * @param recordingId    for the active recording.
 * @return the counter id if found otherwise #NULL_COUNTER_ID.
 */
inline std::int32_t findCounterIdByRecording(const concurrent::CountersReader& countersReader, const std::int64_t recordingId)
{
    const concurrent::AtomicBuffer buffer = countersReader.metaDataBuffer();

    for (std::int32_t i = 0, size = countersReader.maxCounterId(); i < size; ++i)
    {
        if (countersReader.getCounterState(i) == concurrent::CountersReader::RECORD_ALLOCATED)
        {
            const std::int32_t recordOffset = concurrent::CountersReader::metadataOffset(i);

            if (buffer.getInt32(recordOffset + TYPE_ID_OFFSET) == RECORDING_POSITION_TYPE_ID &&
                buffer.getInt64(recordOffset + concurrent::CountersReader::KEY_OFFSET + RECORDING_ID_OFFSET) == recordingId)
            {
                return i;
            }
        }
    }

    return NULL_COUNTER_ID;
}

/**
 * Find the active counter id for a stream based on the session id.
 *
 * @param countersReader to search within.
 * @param sessionId      for the active recording.
 * @return the counter id if found otherwise {@link #NULL_COUNTER_ID}.
 */
inline std::int32_t findCounterIdBySession(const concurrent::CountersReader& countersReader, const std::int32_t sessionId)
{
    const concurrent::AtomicBuffer buffer = countersReader.metaDataBuffer();

    for (std::int32_t i = 0, size = countersReader.maxCounterId(); i < size; ++i)
    {
        if (countersReader.getCounterState(i) == concurrent::CountersReader::RECORD_ALLOCATED)
        {
            const std::int32_t recordOffset = concurrent::CountersReader::metadataOffset(i);

            if (buffer.getInt32(recordOffset + TYPE_ID_OFFSET) == RECORDING_POSITION_TYPE_ID &&
                buffer.getInt32(recordOffset + concurrent::CountersReader::KEY_OFFSET + SESSION_ID_OFFSET) == sessionId)
            {
                return i;
            }
        }
    }

    return NULL_COUNTER_ID;
}

/**
 * Get the recording id for a given counter id.
 *
 * @param countersReader to search within.
 * @param counterId      for the active recording.
 * @return the counter id if found otherwise {@link #NULL_RECORDING_ID}.
 */
inline std::int64_t getRecordingId(const concurrent::CountersReader& countersReader, const std::int32_t counterId)
{
    const concurrent::AtomicBuffer buffer = countersReader.metaDataBuffer();

    if (countersReader.getCounterState(counterId) == concurrent::CountersReader::RECORD_ALLOCATED)
    {
        const std::int32_t recordOffset = concurrent::CountersReader::metadataOffset(counterId);

        if (buffer.getInt32(recordOffset + TYPE_ID_OFFSET) == RECORDING_POSITION_TYPE_ID)
        {
            return buffer.getInt64(recordOffset + concurrent::CountersReader::KEY_OFFSET + RECORDING_ID_OFFSET);
        }
    }

    return NULL_RECORDING_ID;
}

/**
 * Get the {@link Image#sourceIdentity()} for the recording.
 *
 * @param countersReader to search within.
 * @param counterId      for the active recording.
 * @return {@link Image#sourceIdentity()} for the recording or empty string if not found.
 */
inline std::string getSourceIdentity(const concurrent::CountersReader& countersReader, const std::int32_t counterId)
{
    const concurrent::AtomicBuffer buffer = countersReader.metaDataBuffer();

    if (countersReader.getCounterState(counterId) == concurrent::CountersReader::RECORD_ALLOCATED)
    {
        const std::int32_t recordOffset = concurrent::CountersReader::metadataOffset(counterId);

        if (buffer.getInt32(recordOffset + TYPE_ID_OFFSET) == RECORDING_POSITION_TYPE_ID)
        {
            return buffer.getString(recordOffset + concurrent::CountersReader::KEY_OFFSET + SOURCE_IDENTITY_LENGTH_OFFSET);
        }
    }

    return std::string{};
}

/**
 * Is the recording counter still active.
 *
 * @param countersReader to search within.
 * @param counterId      to search for.
 * @param recordingId    to confirm it is still the same value.
 * @return true if the counter is still active otherwise false.
 */
inline bool isActive(const concurrent::CountersReader& countersReader, const std::int32_t counterId, const std::int64_t recordingId)
{
    const concurrent::AtomicBuffer buffer = countersReader.metaDataBuffer();

    if (countersReader.getCounterState(counterId) == concurrent::CountersReader::RECORD_ALLOCATED)
    {
        const std::int32_t recordOffset = concurrent::CountersReader::metadataOffset(counterId);

        return
            buffer.getInt32(recordOffset + TYPE_ID_OFFSET) == RECORDING_POSITION_TYPE_ID &&
            buffer.getInt64(recordOffset + concurrent::CountersReader::KEY_OFFSET + RECORDING_ID_OFFSET) == recordingId;
    }

    return false;
}

}

#endif // AERON_ARCHIVE_STATUS_RECORDINGPOS_H
