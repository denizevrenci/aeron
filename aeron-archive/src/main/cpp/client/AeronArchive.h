#ifndef AERON_ARCHIVE_AERONARCHIVE_H
#define AERON_ARCHIVE_AERONARCHIVE_H

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>

#include <concurrent/YieldingIdleStrategy.h>
#include <util/Exceptions.h>

#include "ArchiveContext.h"
#include "ArchiveException.h"
#include "ArchiveProxy.h"
#include "ControlResponsePoller.h"
#include "ErrorHandler.h"
#include "RecordingDescriptorPoller.h"
#include "RecordingSubscriptionDescriptorPoller.h"

namespace aeron {

class ExclusivePublication;
class Publication;

namespace archive {

class TimeoutException : public util::SourcedException
{
public:
    using util::SourcedException::SourcedException;
};

class AeronArchive
{
    static constexpr std::int64_t NULL_VALUE = -1;

public:
    /**
     * Represents a timestamp that has not been set. Can be used when the time is not known.
     */
    static constexpr std::int64_t NULL_TIMESTAMP = NULL_VALUE;

    /**
     * Represents a position that has not been set. Can be used when the position is not known.
     */
    static constexpr std::int64_t NULL_POSITION = NULL_VALUE;

    /**
     * Represents a length that has not been set. If null length is provided then replay the whole recorded stream.
     */
    static constexpr std::int64_t NULL_LENGTH = NULL_VALUE;

    AeronArchive(
        const Context& context,
        std::int64_t controlSessionId,
        Aeron& aeron,
        ControlResponsePoller controlResponsePoller,
        ArchiveProxy archiveProxy,
        RecordingDescriptorPoller recordingDescriptorPoller,
        RecordingSubscriptionDescriptorPoller recordingSubscriptionDescriptorPoller);

    ~AeronArchive();

    /**
     * The control session id allocated for this connection to the archive.
     *
     * @return control session id allocated for this connection to the archive.
     */
    std::int64_t controlSessionId() const
    {
        return m_controlSessionId;
    }

    /**
     * Aeron client for communicating with the local Media Driver.
     * <p>
     * @return client for communicating with the local Media Driver.
     */
    Aeron& aeron()
    {
        return m_aeron;
    }

    /**
     * Aeron client for communicating with the local Media Driver.
     * <p>
     * @return client for communicating with the local Media Driver.
     */
    const Aeron& aeron() const
    {
        return m_aeron;
    }

    /**
     * The ArchiveProxy for send asynchronous messages to the connected archive.
     *
     * @return the ArchiveProxy for send asynchronous messages to the connected archive.
     */
    ArchiveProxy& archiveProxy()
    {
        return m_archiveProxy;
    }

    /**
     * The ArchiveProxy for send asynchronous messages to the connected archive.
     *
     * @return the ArchiveProxy for send asynchronous messages to the connected archive.
     */
    const ArchiveProxy& archiveProxy() const
    {
        return m_archiveProxy;
    }

    /**
     * Get the ControlResponsePoller for polling additional events on the control channel.
     *
     * @return the ControlResponsePoller for polling additional events on the control channel.
     */
    ControlResponsePoller& controlResponsePoller()
    {
        return m_controlResponsePoller;
    }

    /**
     * Get the ControlResponsePoller for polling additional events on the control channel.
     *
     * @return the ControlResponsePoller for polling additional events on the control channel.
     */
    const ControlResponsePoller& controlResponsePoller() const
    {
        return m_controlResponsePoller;
    }

    /**
     * Get the RecordingDescriptorPoller for polling recording descriptors on the control channel.
     *
     * @return the RecordingDescriptorPoller for polling recording descriptors on the control channel.
     */
    RecordingDescriptorPoller& recordingDescriptorPoller()
    {
        return m_recordingDescriptorPoller;
    }

    /**
     * Get the RecordingDescriptorPoller for polling recording descriptors on the control channel.
     *
     * @return the RecordingDescriptorPoller for polling recording descriptors on the control channel.
     */
    const RecordingDescriptorPoller& recordingDescriptorPoller() const
    {
        return m_recordingDescriptorPoller;
    }

    /**
     * The RecordingSubscriptionDescriptorPoller for polling subscription descriptors on the control channel.
     *
     * @return the RecordingSubscriptionDescriptorPoller for polling subscription descriptors on the control
     * channel.
     */
    RecordingSubscriptionDescriptorPoller& recordingSubscriptionDescriptorPoller()
    {
        return m_recordingSubscriptionDescriptorPoller;
    }

    /**
     * The RecordingSubscriptionDescriptorPoller for polling subscription descriptors on the control channel.
     *
     * @return the RecordingSubscriptionDescriptorPoller for polling subscription descriptors on the control
     * channel.
     */
    const RecordingSubscriptionDescriptorPoller& recordingSubscriptionDescriptorPoller() const
    {
        return m_recordingSubscriptionDescriptorPoller;
    }

    /**
     * Poll the response stream once for an error. If another message is present then it will be skipped over
     * so only call when not expecting another response.
     *
     * @return the error string otherwise null if no error is found.
     */
    std::string pollForErrorResponse();

    /**
     * Check if an error has been returned for the control session and throw a ArchiveException if Context#errorHandler() is not set.
     * <p>
     * To check for an error response without raising an exception then try pollForErrorResponse().
     *
     * @see #pollForErrorResponse()
     */
    void checkForErrorResponse();

    /**
     * Add a Publication and set it up to be recorded. If this is not the first,
     * i.e. Publication#isOriginal() is true,  then an ArchiveException
     * will be thrown and the recording not initiated.
     * <p>
     * This is a sessionId specific recording.
     *
     * @param channel  for the publication.
     * @param streamId for the publication.
     * @return the Publication ready for use.
     */
    std::shared_ptr<Publication> addRecordedPublication(const std::string& channel, std::int32_t streamId);

    /**
     * Add an ExclusivePublication and set it up to be recorded.
     * <p>
     * This is a sessionId specific recording.
     *
     * @param channel  for the publication.
     * @param streamId for the publication.
     * @return the ExclusivePublication ready for use.
     */
    std::shared_ptr<ExclusivePublication> addRecordedExclusivePublication(
        const std::string& channel,
        std::int32_t streamId);

    /**
     * Start recording a channel and stream pairing.
     * <p>
     * Channels that include sessionId parameters are considered different than channels without sessionIds. If a
     * publication matches both a sessionId specific channel recording and a non-sessionId specific recording, it will
     * be recorded twice.
     *
     * @param channel        to be recorded.
     * @param streamId       to be recorded.
     * @param sourceLocation of the publication to be recorded.
     * @return the subscriptionId, i.e. Subscription#registrationId(), of the recording.
     */
    std::int64_t startRecording(const std::string& channel,
        std::int32_t streamId,
        codecs::SourceLocation::Value sourceLocation);

    /**
     * Extend an existing, non-active recording of a channel and stream pairing.
     * <p>
     * The channel must be configured for the initial position from which it will be extended. This can be done
     * with ChannelUriStringBuilder#initialPosition(std::int64_t, std::int32_t, std::int32_t). The details required
     * to initialise can be found by calling #listRecording(std::int64_t, RecordingDescriptorConsumer).
     *
     * @param recordingId    of the existing recording.
     * @param channel        to be recorded.
     * @param streamId       to be recorded.
     * @param sourceLocation of the publication to be recorded.
     * @tparam IdleStrategy  to use for polling operations.
     * @return the subscriptionId, i.e. Subscription#registrationId, of the recording.
     */
    std::int64_t extendRecording(
        std::int64_t recordingId,
        const std::string& channel,
        std::int32_t streamId,
        codecs::SourceLocation::Value sourceLocation);

    /**
     * Stop recording for a channel and stream pairing.
     * <p>
     * Channels that include sessionId parameters are considered different than channels without sessionIds. Stopping
     * a recording on a channel without a sessionId parameter will not stop the recording of any sessionId specific
     * recordings that use the same channel and streamId.
     *
     * @param channel  to stop recording for.
     * @param streamId to stop recording for.
     */
    void stopRecording(const std::string& channel, std::int32_t streamId);

    /**
     * Stop recording a sessionId specific recording that pertains to the given Publication.
     *
     * @param publication to stop recording for.
     */
    void stopRecording(const Publication& publication);

    /**
     * Stop recording a sessionId specific recording that pertains to the given ExclusivePublication.
     *
     * @param publication to stop recording for.
     */
    void stopRecording(const ExclusivePublication& publication);

    /**
     * Stop recording for a subscriptionId that has been returned from
     * #startRecording(const std::string&, std::int32_t, codecs::SourceLocation::Value) or
     * #extendRecording(std::uint64_t, const std::string&, std::int32_t, codecs::SourceLocation::Value).
     *
     * @param subscriptionId is the Subscription#registrationId() for the recording in the archive.
     */
    void stopRecording(std::int64_t subscriptionId);

    /**
     * Start a replay for a length in bytes of a recording from a position. If the position is #NULL_POSITION
     * then the stream will be replayed from the start.
     * <p>
     * The lower 32-bits of the returned value contain the Image#sessionId() of the received replay. All
     * 64-bits are required to uniquely identify the replay when calling #stopReplay(long). The lower 32-bits
     * can be obtained by casting the std::int64_t value to an std::int32_t.
     *
     * @param recordingId    to be replayed.
     * @param position       from which the replay should begin or #NULL_POSITION if from the start.
     * @param length         of the stream to be replayed. Use std::numeric_limits<int64_t>::max() to follow a live recording or
     *                       #NULL_LENGTH to replay the whole stream of unknown length.
     * @param replayChannel  to which the replay should be sent.
     * @param replayStreamId to which the replay should be sent.
     * @return the id of the replay session which will be the same as the Image#sessionId() of the received
     * replay for correlation with the matching channel and stream id in the lower 32 bits.
     */
    std::int64_t startReplay(
        std::int64_t recordingId,
        std::int64_t position,
        std::int64_t length,
        const std::string& replayChannel,
        std::int32_t replayStreamId);

    /**
    * Start a bound replay for a length in bytes of a recording from a position. If the position is #NULL_POSITION
    * then the stream will be replayed from the start. The replay is bounded by the limit counter's position value.
    * <p>
    * The lower 32-bits of the returned value contains the Image#sessionId of the received replay. All
    * 64-bits are required to uniquely identify the replay when calling #stopReplay. The lower 32-bits
    * can be obtained by casting the std::int64_t value to an std::int32_t.
    *
    * @param recordingId    to be replayed.
    * @param position       from which the replay should begin or #NULL_POSITION if from the start.
    * @param length         of the stream to be replayed. Use std::numeric_limits<std::int64_t>::max to follow a live
    *                       recording or #NULL_LENGTH to replay the whole stream of unknown length.
    * @param limitCounterId for the counter which bounds the replay by the position it contains.
    * @param replayChannel  to which the replay should be sent.
    * @param replayStreamId to which the replay should be sent.
    * @return the id of the replay session which will be the same as the Image#sessionId of the received
    *         replay for correlation with the matching channel and stream id in the lower 32 bits.
    */
    std::int64_t startBoundedReplay(
        std::int64_t recordingId,
        std::int64_t position,
        std::int64_t length,
        std::int32_t limitCounterId,
        const std::string& replayChannel,
        std::int32_t replayStreamId);

    /**
     * Stop a replay session.
     *
     * @param replaySessionId to stop replay for.
     */
    void stopReplay(std::int64_t replaySessionId);

    /**
     * Stop all replays matching a recording id. If recording id is #NULL_VALUE then match all replays.
     *
     * @param recordingId to stop replays for.
     */
    void stopAllReplays(std::int64_t recordingId);

    /**
     * Replay a length in bytes of a recording from a position and for convenience create a Subscription
     * to receive the replay. If the position is #NULL_POSITION then the stream will be replayed from the start.
     *
     * @param recordingId    to be replayed.
     * @param position       from which the replay should begin or #NULL_POSITION if from the start.
     * @param length         of the stream to be replayed or std::int64_t#MAX_VALUE to follow a live recording.
     * @param replayChannel  to which the replay should be sent.
     * @param replayStreamId to which the replay should be sent.
     * @return registration id for the Subscription for consuming the replay.
     */
    std::int64_t replay(
        std::int64_t recordingId,
        std::int64_t position,
        std::int64_t length,
        const std::string& replayChannel,
        std::int32_t replayStreamId);

    /**
     * Replay a length in bytes of a recording from a position and for convenience create a Subscription
     * to receive the replay. If the position is #NULL_POSITION then the stream will be replayed from the start.
     *
     * @param recordingId             to be replayed.
     * @param position                from which the replay should begin or #NULL_POSITION if from the start.
     * @param length                  of the stream to be replayed or std::int64_t#MAX_VALUE to follow a live recording.
     * @param replayChannel           to which the replay should be sent.
     * @param replayStreamId          to which the replay should be sent.
     * @param availableImageHandler   to be called when the replay image becomes available.
     * @param unavailableImageHandler to be called when the replay image goes unavailable.
     * @return registration id for the Subscription for consuming the replay.
     */
    std::int64_t replay(
        std::int64_t recordingId,
        std::int64_t position,
        std::int64_t length,
        const std::string& replayChannel,
        std::int32_t replayStreamId,
        const on_available_image_t& availableImageHandler,
        const on_unavailable_image_t& unavailableImageHandler);

    /**
     * List all recording descriptors from a recording id with a limit of record count.
     * <p>
     * If the recording id is greater than the largest known id then nothing is returned.
     *
     * @param fromRecordingId at which to begin the listing.
     * @param recordCount     to limit for each query.
     * @param consumer        to which the descriptors are dispatched.
     * @return the number of descriptors found and consumed.
     */
    template <typename RecordingDescriptorConsumer>
    std::int32_t listRecordings(
        const std::int64_t fromRecordingId, const std::int32_t recordCount, RecordingDescriptorConsumer&& consumer)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        const std::int64_t correlationId = m_aeron.nextCorrelationId();
        if (!m_archiveProxy.listRecordings(
            fromRecordingId,
            recordCount,
            correlationId,
            m_controlSessionId))
        {
            throw ArchiveException("failed to send list recordings request", SOURCEINFO);
        }
        return pollForDescriptors(correlationId, recordCount, std::forward<RecordingDescriptorConsumer>(consumer));
    }

    /**
     * List recording descriptors from a recording id with a limit of record count for a given channelFragment and
     * stream id.
     * <p>
     * If the recording id is greater than the largest known id then nothing is returned.
     *
     * @param fromRecordingId at which to begin the listing.
     * @param recordCount     to limit for each query.
     * @param channelFragment for a contains match on the original channel stored with the archive descriptor.
     * @param streamId        to match.
     * @param consumer        to which the descriptors are dispatched.
     * @return the number of descriptors found and consumed.
     */
    template <typename RecordingDescriptorConsumer>
    std::int32_t listRecordingsForUri(
        const std::int64_t fromRecordingId,
        const std::int32_t recordCount,
        const std::string& channelFragment,
        const std::int32_t streamId,
        RecordingDescriptorConsumer&& consumer)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        const std::int64_t correlationId = m_aeron.nextCorrelationId();
        if (!m_archiveProxy.listRecordingsForUri(
            fromRecordingId,
            recordCount,
            channelFragment,
            streamId,
            correlationId,
            m_controlSessionId))
        {
            throw ArchiveException("failed to send list recordings request", SOURCEINFO);
        }
        return pollForDescriptors(correlationId, recordCount, std::forward<RecordingDescriptorConsumer>(consumer));
    }


    /**
     * List a recording descriptor for a single recording id.
     * <p>
     * If the recording id is greater than the largest known id then nothing is returned.
     *
     * @param recordingId at which to begin the listing.
     * @param consumer    to which the descriptors are dispatched.
     * @return the number of descriptors found and consumed.
     */
    template <typename RecordingDescriptorConsumer>
    std::int32_t listRecording(const std::int64_t recordingId, RecordingDescriptorConsumer&& consumer)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        const std::int64_t correlationId = m_aeron.nextCorrelationId();
        if (!m_archiveProxy.listRecording(
            recordingId,
            correlationId,
            m_controlSessionId))
        {
            throw ArchiveException("failed to send list recording request", SOURCEINFO);
        }
        return pollForDescriptors(correlationId, 1, std::forward<RecordingDescriptorConsumer>(consumer));
    }

    /**
     * List active recording subscriptions in the archive.
     * These are the result of requesting one of startRecording or a extendRecording.
     * The returned subscription ID can be used for passing to stopRecording.
     *
     * @param pseudoIndex       in the active list at which to begin for paging.
     * @param subscriptionCount to get in a listing.
     * @param channelFragment   to do a contains match on the stripped channel URI. Empty string is match all.
     * @param streamId          to match on the subscription.
     * @param applyStreamId     true if the stream id should be matched.
     * @param consumer          for the matched subscription descriptors.
     * @return the count of matched subscriptions.
     */
    template <typename RecordingSubscriptionDescriptorConsumer>
    std::int32_t listRecordingSubscriptions(
        const std::int32_t pseudoIndex,
        const std::int32_t subscriptionCount,
        const std::string& channelFragment,
        const std::int32_t streamId,
        const bool applyStreamId,
        const RecordingSubscriptionDescriptorConsumer& consumer)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        const std::int64_t correlationId = m_aeron.nextCorrelationId();
        if (!m_archiveProxy.listRecordingSubscriptions(
            pseudoIndex,
            subscriptionCount,
            channelFragment,
            streamId,
            applyStreamId,
            correlationId,
            m_controlSessionId))
        {
            throw ArchiveException("failed to send list recording subscriptions request", SOURCEINFO);
        }
        return pollForSubscriptionDescriptors(correlationId, subscriptionCount, std::forward<RecordingSubscriptionDescriptorConsumer>(consumer));
    }

    /**
     * Get the position recorded for an active recording. If no active recording then return #NULL_POSITION.
     *
     * @param recordingId of the active recording for which the position is required.
     * @return the recorded position for the active recording or #NULL_POSITION if recording is not active.
     * @see #getStopPosition(std::int64_t)
     */
    std::int64_t getRecordingPosition(std::int64_t recordingId);

    /**
     * Get the stop position for a recording.
     *
     * @param recordingId of the active recording for which the position is required.
     * @return the stop position, or #NULL_POSITION if still active.
     * @see #getRecordingPosition(std::int64_t)
     */
    std::int64_t getStopPosition(std::int64_t recordingId);

    /**
     * Find the last recording that matches the given criteria.
     *
     * @param minRecordingId  to search back to.
     * @param channelFragment for a contains match on the original channel stored with the archive descriptor.
     * @param streamId        of the recording to match.
     * @param sessionId       of the recording to match.
     * @return the recordingId if found otherwise #NULL_VALUE if not found.
     */
    std::int64_t findLastMatchingRecording(
        std::int64_t minRecordingId,
        const std::string& channelFragment,
        std::int32_t streamId,
        std::int32_t sessionId);

    /**
     * Truncate a stopped recording to a given position that is less than the stopped position. The provided position
     * must be on a fragment boundary. Truncating a recording to the start position effectively deletes the recording.
     *
     * @param recordingId of the stopped recording to be truncated.
     * @param position    to which the recording will be truncated.
     */
    void truncateRecording(std::int64_t recordingId, std::int64_t position);

private:
    static constexpr std::int32_t FRAGMENT_LIMIT = 10;

    const std::int64_t m_controlSessionId;
    const std::chrono::nanoseconds m_messageTimeout;
    Aeron& m_aeron;
    ArchiveProxy m_archiveProxy;
    concurrent::YieldingIdleStrategy m_idleStrategy;
    on_error_t m_errorHandler;
    ControlResponsePoller m_controlResponsePoller;
    RecordingDescriptorPoller m_recordingDescriptorPoller;
    RecordingSubscriptionDescriptorPoller m_recordingSubscriptionDescriptorPoller;
    mutable std::mutex m_mutex;

    std::int64_t pollForResponse(std::int64_t correlationId);

    template <typename Consumer>
    std::int32_t pollForDescriptors(
        const std::int64_t correlationId, const std::int32_t recordCount, Consumer&& consumer)
    {
        auto existingRemainCount = recordCount;
        auto deadlineTs = std::chrono::high_resolution_clock::now() + m_messageTimeout;
        auto& poller = m_recordingDescriptorPoller;
        poller.reset(correlationId, recordCount, std::forward<Consumer>(consumer));

        while (true)
        {
            const int fragments = poller.poll();
            const auto remainingRecordCount = poller.remainingRecordCount();

            if (poller.isDispatchComplete())
            {
                return recordCount - remainingRecordCount;
            }

            if (remainingRecordCount != existingRemainCount)
            {
                existingRemainCount = remainingRecordCount;
                deadlineTs = std::chrono::high_resolution_clock::now() + m_messageTimeout;
            }

            m_aeron.conductorAgentInvoker().invoke();

            if (fragments > 0)
            {
                continue;
            }

            if (!poller.subscription().isConnected())
            {
                throw ArchiveException("subscription to archive is not connected", SOURCEINFO);
            }

            if (std::chrono::high_resolution_clock::now() > deadlineTs)
            {
                throw TimeoutException(
                    "awaiting recording descriptors - correlationId=" + std::to_string(correlationId), SOURCEINFO);
            }
            m_idleStrategy.idle();
        }
    }

    template <typename Consumer>
    std::int32_t pollForSubscriptionDescriptors(
        const std::int64_t correlationId, const std::int32_t recordCount, Consumer&& consumer)
    {
        auto existingRemainCount = recordCount;
        auto deadlineTs = std::chrono::high_resolution_clock::now() + m_messageTimeout;
        auto& poller = m_recordingSubscriptionDescriptorPoller;
        poller.reset(correlationId, recordCount, std::forward<Consumer>(consumer));

        while (true)
        {
            const int fragments = poller.poll();
            const auto remainingSubscriptionCount = poller.remainingSubscriptionCount();

            if (poller.isDispatchComplete())
            {
                return recordCount - remainingSubscriptionCount;
            }

            if (remainingSubscriptionCount != existingRemainCount)
            {
                existingRemainCount = remainingSubscriptionCount;
                deadlineTs = std::chrono::high_resolution_clock::now() + m_messageTimeout;
            }

            m_aeron.conductorAgentInvoker().invoke();

            if (fragments > 0)
            {
                continue;
            }

            if (!poller.subscription().isConnected())
            {
                throw ArchiveException("subscription to archive is not connected", SOURCEINFO);
            }

            if (std::chrono::high_resolution_clock::now() > deadlineTs)
            {
                throw TimeoutException(
                    "awaiting subscription descriptors - correlationId=" + std::to_string(correlationId), SOURCEINFO);
            }
            m_idleStrategy.idle();
        }
    }
};

/**
 * Allows for the async establishment of a archive session.
 */
class AsyncConnect
{
public:
    AsyncConnect(const Context& ctx, Aeron& aeron);

    /**
     * Poll for a complete connection.
     *
     * @return true if successfully connected.
     */
    bool poll();

    /**
     * Construct an AeronArchive after a complete connection.
     *
     * @return a new AeronArchive if successfully connected. Undefined behavior otherwise.
     */
    AeronArchive makeArchive();

private:
    std::shared_ptr<Subscription> m_subscription;
    std::shared_ptr<ExclusivePublication> m_publication;
    std::optional<ArchiveProxy> m_archiveProxy;
    std::optional<ControlResponsePoller> m_controlResponsePoller;
    bool m_archiveProxyReady = false;
    bool m_controlResponsePollerReady = false;
    std::int64_t m_correlationId = aeron::NULL_VALUE;
    const Context& m_ctx;
    Aeron& m_aeron;
    const std::int64_t m_subscriptionId;
    const std::int64_t m_publicationId;
};

/**
 * Begin an attempt at creating a connection which can be completed by calling AsyncConnect#poll.
 * <p>
 * @param ctx for the archive connection.
 * @param aeron to pass access to driver.
 * @return the AsyncConnect that can be polled for completion.
 */
inline AsyncConnect asyncConnect(const Context& ctx, Aeron& aeron)
{
    return {ctx, aeron};
}

/**
 * Connect to an Aeron archive by providing a Context.
 * This will create a control session.
 * <p>
 * @param context for connection configuration.
 * @param aeron to pass access to driver.
 * @return the newly created Aeron Archive client.
 */
template <typename ConnectIdleStrategy = concurrent::YieldingIdleStrategy>
AeronArchive connect(const Context& context, Aeron& aeron, ConnectIdleStrategy idle = ConnectIdleStrategy())
{
    AsyncConnect asyncConnect(context, aeron);

    while (!asyncConnect.poll())
    {
        if (aeron.usesAgentInvoker())
        {
            aeron.conductorAgentInvoker().invoke();
        }

        idle.idle();
    }

    return asyncConnect.makeArchive();
}

}}

#endif // AERON_ARCHIVE_AERONARCHIVE_H
