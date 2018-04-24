#ifndef AERON_ARCHIVE_ARCHIVEPROXY_H
#define AERON_ARCHIVE_ARCHIVEPROXY_H

#include <chrono>
#include <string>
#include <vector>

#include <concurrent/AtomicBuffer.h>
#include <concurrent/YieldingIdleStrategy.h>
#include <ExclusivePublication.h>

#include <aeron_archive_client/BoundedReplayRequest.h>
#include <aeron_archive_client/CloseSessionRequest.h>
#include <aeron_archive_client/ConnectRequest.h>
#include <aeron_archive_client/ExtendRecordingRequest.h>
#include <aeron_archive_client/FindLastMatchingRecordingRequest.h>
#include <aeron_archive_client/ListRecordingRequest.h>
#include <aeron_archive_client/ListRecordingSubscriptionsRequest.h>
#include <aeron_archive_client/ListRecordingsForUriRequest.h>
#include <aeron_archive_client/ListRecordingsRequest.h>
#include <aeron_archive_client/MessageHeader.h>
#include <aeron_archive_client/RecordingPositionRequest.h>
#include <aeron_archive_client/ReplayRequest.h>
#include <aeron_archive_client/SourceLocation.h>
#include <aeron_archive_client/StartRecordingRequest.h>
#include <aeron_archive_client/StopAllReplaysRequest.h>
#include <aeron_archive_client/StopPositionRequest.h>
#include <aeron_archive_client/StopRecordingRequest.h>
#include <aeron_archive_client/StopRecordingSubscriptionRequest.h>
#include <aeron_archive_client/StopReplayRequest.h>
#include <aeron_archive_client/TruncateRecordingRequest.h>

namespace aeron {

class ClientConductor;
class ExclusivePublication;

namespace concurrent {

template <typename>
class AgentInvoker;

}

namespace archive {

namespace codecs = client;

class ArchiveProxy
{
public:
    /**
     * Create a proxy with a ExclusivePublication for sending control message requests.
     * <p>
     * This provides defaults of AeronArchive.Configuration#MESSAGE_TIMEOUT_DEFAULT and
     * #DEFAULT_RETRY_ATTEMPTS.
     *
     * @param publication publication for sending control messages to an archive.
     */
    ArchiveProxy(
        std::shared_ptr<ExclusivePublication> publication,
        std::chrono::nanoseconds connectTimeout = MESSAGE_TIMEOUT_DEFAULT,
        std::int32_t retryAttempts = DEFAULT_RETRY_ATTEMPTS);

    ArchiveProxy(const ArchiveProxy&) = delete;
    ArchiveProxy& operator =(const ArchiveProxy&) = delete;

    ArchiveProxy(ArchiveProxy&&) = default;
    ArchiveProxy& operator =(ArchiveProxy&&) = default;

    ~ArchiveProxy();

    ExclusivePublication& publication()
    {
        return *m_publication;
    }

    [[nodiscard]] const ExclusivePublication& publication() const
    {
        return *m_publication;
    }

    /**
     * Connect to an archive on its control interface providing the response stream details.
     *
     * @param responseChannel  for the control message responses.
     * @param responseStreamId for the control message responses.
     * @param correlationId    for this request.
     * @return true if successfully offered otherwise false.
     */
    bool connect(
        const std::string& responseChannel, std::int32_t responseStreamId, std::int64_t correlationId);

    /**
     * Try Connect to an archive on its control interface providing the response stream details. Only one attempt will
     * be made to offer the request.
     *
     * @param responseChannel  for the control message responses.
     * @param responseStreamId for the control message responses.
     * @param correlationId    for this request.
     * @return true if successfully offered otherwise false.
     */
    bool tryConnect(
        const std::string& responseChannel, std::int32_t responseStreamId, std::int64_t correlationId);

    /**
     * Connect to an archive on its control interface providing the response stream details.
     *
     * @param responseChannel    for the control message responses.
     * @param responseStreamId   for the control message responses.
     * @param correlationId      for this request.
     * @param aeronClientInvoker for aeron client conductor thread.
     * @return true if successfully offered otherwise false.
     */
    bool connect(
        const std::string& responseChannel,
        std::int32_t responseStreamId,
        std::int64_t correlationId,
        AgentInvoker<ClientConductor>&);

    /**
     * Close this control session with the archive.
     *
     * @param controlSessionId with the archive.
     * @return true if successfully offered otherwise false.
     */
    bool closeSession(std::int64_t controlSessionId);

    /**
     * Start recording streams for a given channel and stream id pairing.
     *
     * @param channel          to be recorded.
     * @param streamId         to be recorded.
     * @param sourceLocation   of the publication to be recorded.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool startRecording(
        const std::string& channel,
        std::int32_t streamId,
        codecs::SourceLocation::Value sourceLocation,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * Stop an active recording.
     *
     * @param channel          to be stopped.
     * @param streamId         to be stopped.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool stopRecording(
        const std::string& channel,
        std::int32_t streamId,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * Stop an active recording by the Subscription#registrationId() it was registered with.
     *
     * @param subscriptionId   that identifies the subscription in the archive doing the recording.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool stopRecording(
        std::int64_t subscriptionId,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * Replay a recording from a given position.
     *
     * @param recordingId      to be replayed.
     * @param position         from which the replay should be started.
     * @param length           of the stream to be replayed. Use std::int64_t#MAX_VALUE to follow a live stream.
     * @param replayChannel    to which the replay should be sent.
     * @param replayStreamId   to which the replay should be sent.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool replay(
        std::int64_t recordingId,
        std::int64_t position,
        std::int64_t length,
        const std::string& replayChannel,
        std::int32_t replayStreamId,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * Replay a recording from a given position and bounded by a counter containing position to limit replay.
     *
     * @param recordingId      to be replayed.
     * @param position         from which the replay should be started.
     * @param length           of the stream to be replayed. Use std::numeric_limits<std::int64_t>::max to follow a live stream.
     * @param limitCounterId   used to bound the replay.
     * @param replayChannel    to which the replay should be sent.
     * @param replayStreamId   to which the replay should be sent.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool boundedReplay(
        std::int64_t recordingId,
        std::int64_t position,
        std::int64_t length,
        std::int32_t limitCounterId,
        const std::string& replayChannel,
        std::int32_t replayStreamId,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * Stop an existing replay session.
     *
     * @param replaySessionId  that should be stopped.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool stopReplay(
        std::int64_t replaySessionId,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * Stop existing replays matching a recording id. If recording id is #NULL_VALUE then match all replays.
     *
     * @param recordingId      that should match replays to be stopped.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool stopAllReplays(
        std::int64_t recordingId,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * List a range of recording descriptors.
     *
     * @param fromRecordingId  at which to begin listing.
     * @param recordCount      for the number of descriptors to be listed.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool listRecordings(
        std::int64_t fromRecordingId,
        std::int32_t recordCount,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * List a range of recording descriptors which match a channel URI fragment and stream id.
     *
     * @param fromRecordingId  at which to begin listing.
     * @param recordCount      for the number of descriptors to be listed.
     * @param channelFragment  to match recordings on from the original channel URI in the archive descriptor.
     * @param streamId         to match recordings on.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool listRecordingsForUri(
        std::int64_t fromRecordingId,
        std::int32_t recordCount,
        const std::string& channelFragment,
        std::int32_t streamId,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * List a recording descriptor for a given recording id.
     *
     * @param recordingId      at which to begin listing.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool listRecording(
        std::int64_t recordingId,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * List registered subscriptions in the archive which have been used to record streams.
     *
     * @param pseudoIndex       in the list of active recording subscriptions.
     * @param subscriptionCount for the number of descriptors to be listed.
     * @param channelFragment   for a contains match on the stripped channel used with the registered subscription.
     * @param streamId          for the subscription.
     * @param applyStreamId     when matching.
     * @param correlationId     for this request.
     * @param controlSessionId  for this request.
     * @return true if successfully offered otherwise false.
     */
    bool listRecordingSubscriptions(
        std::int32_t pseudoIndex,
        std::int32_t subscriptionCount,
        const std::string& channelFragment,
        std::int32_t streamId,
        bool applyStreamId,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * Extend an existing, non-active, recorded stream for a the same channel and stream id.
     *
     * The channel must be configured for the initial position from which it will be extended. This can be done
     * with ChannelUriStringBuilder#initialPosition(std::int64_t, std::int32_t, std::int32_t). The details required to initialise can
     * be found by calling #listRecording(std::int64_t, std::int64_t, std::int64_t).
     *
     * @param channel          to be recorded.
     * @param streamId         to be recorded.
     * @param sourceLocation   of the publication to be recorded.
     * @param recordingId      to be extended.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool extendRecording(
        const std::string& channel,
        std::int32_t streamId,
        codecs::SourceLocation::Value sourceLocation,
        std::int64_t recordingId,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * Get the recorded position of an active recording.
     *
     * @param recordingId      of the active recording that the position is being requested for.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool getRecordingPosition(
        std::int64_t recordingId,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * Get the stop position of a recording.
     *
     * @param recordingId      of the recording that the stop position is being requested for.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool getStopPosition(
        std::int64_t recordingId,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * Truncate a stopped recording to a given position that is less than the stopped position. The provided position
     * must be on a fragment boundary. Truncating a recording to the start position effectively deletes the recording.
     *
     * @param recordingId      of the stopped recording to be truncated.
     * @param position         to which the recording will be truncated.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool truncateRecording(
        std::int64_t recordingId,
        std::int64_t position,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

    /**
     * Truncate a stopped recording to a given position that is less than the stopped position. The provided position
     * must be on a fragment boundary. Truncating a recording to the start position effectively deletes the recording.
     * <p>
     * Find the last recording that matches the given criteria.
     *
     * @param minRecordingId   to search back to.
     * @param channelFragment  for a contains match on the original channel stored with the archive descriptor.
     * @param streamId         of the recording to match.
     * @param sessionId        of the recording to match.
     * @param correlationId    for this request.
     * @param controlSessionId for this request.
     * @return true if successfully offered otherwise false.
     */
    bool findLastMatchingRecording(
        std::int64_t minRecordingId,
        const std::string& channelFragment,
        std::int32_t streamId,
        std::int32_t sessionId,
        std::int64_t correlationId,
        std::int64_t controlSessionId);

private:
    static constexpr std::int32_t DEFAULT_RETRY_ATTEMPTS = 3;
    static constexpr std::chrono::nanoseconds MESSAGE_TIMEOUT_DEFAULT{5};
    static constexpr std::size_t BUFFER_ALIGNMENT = 16;
    static constexpr std::size_t BUFFER_LENGTH = 256;

    class UniqueAtomicBuffer : public concurrent::AtomicBuffer {
    public:
        using AtomicBuffer::AtomicBuffer;

        UniqueAtomicBuffer(UniqueAtomicBuffer&& other) noexcept :
            AtomicBuffer(other)
        {
            other.wrap(nullptr, 0);
        }

        UniqueAtomicBuffer& operator=(UniqueAtomicBuffer&& other) noexcept
        {
            wrap(other);
            other.wrap(nullptr, 0);
            return *this;
        }
    };

    UniqueAtomicBuffer m_buffer;

    std::chrono::nanoseconds m_connectTimeout;
    std::int32_t m_retryAttempts;
    concurrent::YieldingIdleStrategy m_idleStrategy;

    std::shared_ptr<ExclusivePublication> m_publication;

    codecs::CloseSessionRequest m_closeSessionRequest;
    codecs::ConnectRequest m_connectRequest;
    codecs::ExtendRecordingRequest m_extendRecordingRequest;
    codecs::ListRecordingRequest m_listRecordingRequest;
    codecs::ListRecordingSubscriptionsRequest m_listRecordingSubscriptionsRequest;
    codecs::ListRecordingsForUriRequest m_listRecordingsForUriRequest;
    codecs::ListRecordingsRequest m_listRecordingsRequest;
    codecs::MessageHeader m_messageHeader;
    codecs::RecordingPositionRequest m_recordingPositionRequest;
    codecs::ReplayRequest m_replayRequest;
    codecs::BoundedReplayRequest m_boundedReplayRequest;
    codecs::StartRecordingRequest m_startRecordingRequest;
    codecs::StopRecordingRequest m_stopRecordingRequest;
    codecs::StopRecordingSubscriptionRequest m_stopRecordingSubscriptionRequest;
    codecs::StopReplayRequest m_stopReplayRequest;
    codecs::StopAllReplaysRequest m_stopAllReplaysRequest;
    codecs::TruncateRecordingRequest m_truncateRecordingRequest;
    codecs::StopPositionRequest m_stopPositionRequest;
    codecs::FindLastMatchingRecordingRequest m_findLastMatchingRecordingRequest;

    bool offer(std::int32_t length);
    bool offerWithTimeout(std::int32_t length);
    bool offerWithTimeout(std::int32_t length, AgentInvoker<ClientConductor>& aeronClientInvoker);

    template <typename Encoder>
    Encoder& applyHeaderAndResetPosition(Encoder& encoder);
};

}}

#endif // AERON_ARCHIVE_ARCHIVEPROXY_H
