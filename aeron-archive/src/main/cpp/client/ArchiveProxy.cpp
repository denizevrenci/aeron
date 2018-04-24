#include "ArchiveProxy.h"

#include <cstdlib>

#include <ClientConductor.h>
#include <ExclusivePublication.h>
#include <concurrent/AgentInvoker.h>

#include "ArchiveConfiguration.h"
#include "ArchiveException.h"

namespace aeron::archive {

namespace {

template <typename Encoder1, typename Encoder2>
constexpr void verifySchemaIdAndVersion()
{
    static_assert(Encoder1::sbeSchemaId() == Encoder1::sbeSchemaId());
    static_assert(Encoder1::sbeSchemaVersion() == Encoder1::sbeSchemaVersion());
}

template <typename... Encoders>
constexpr void verifySchemaIdAndVersions()
{
    (verifySchemaIdAndVersion<Encoders, codecs::CloseSessionRequest>(), ...);
}

template <typename Codec>
util::index_t messageAndHeaderLength(Codec& codec)
{
    return static_cast<util::index_t>(codecs::MessageHeader::encodedLength() + codec.encodedLength());
}

}

ArchiveProxy::ArchiveProxy(
    std::shared_ptr<ExclusivePublication> publication,
    const std::chrono::nanoseconds connectTimeout,
    const std::int32_t retryAttempts) :
    m_buffer(static_cast<uint8_t*>(std::aligned_alloc(BUFFER_ALIGNMENT, BUFFER_LENGTH)), BUFFER_LENGTH),
    m_connectTimeout(connectTimeout),
    m_retryAttempts(retryAttempts),
    m_publication(std::move(publication)),
    m_closeSessionRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_connectRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_extendRecordingRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_listRecordingRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_listRecordingSubscriptionsRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_listRecordingsForUriRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_listRecordingsRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_messageHeader(m_buffer.sbeData(), BUFFER_LENGTH, codecs::MessageHeader::sbeSchemaVersion()),
    m_recordingPositionRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_replayRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_startRecordingRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_stopRecordingRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_stopRecordingSubscriptionRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_stopReplayRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_truncateRecordingRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_stopPositionRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength()),
    m_findLastMatchingRecordingRequest(m_buffer.sbeData() + codecs::MessageHeader::encodedLength(), BUFFER_LENGTH - codecs::MessageHeader::encodedLength())
{
    verifySchemaIdAndVersions<
        codecs::ConnectRequest,
        codecs::ExtendRecordingRequest,
        codecs::ListRecordingRequest,
        codecs::ListRecordingSubscriptionsRequest,
        codecs::ListRecordingsForUriRequest,
        codecs::ListRecordingsRequest,
        codecs::MessageHeader,
        codecs::RecordingPositionRequest,
        codecs::ReplayRequest,
        codecs::StartRecordingRequest,
        codecs::StopRecordingRequest,
        codecs::StopRecordingSubscriptionRequest,
        codecs::StopReplayRequest,
        codecs::TruncateRecordingRequest,
        codecs::StopPositionRequest,
        codecs::FindLastMatchingRecordingRequest>();

    m_messageHeader
        .schemaId(codecs::CloseSessionRequest::sbeSchemaId())
        .version(codecs::CloseSessionRequest::sbeSchemaVersion());
}

ArchiveProxy::~ArchiveProxy()
{
    if (m_buffer.buffer())
        std::free(m_buffer.buffer());
}

bool ArchiveProxy::connect(
    const std::string& responseChannel, const std::int32_t responseStreamId, const std::int64_t correlationId)
{
    applyHeaderAndResetPosition(m_connectRequest)
        .correlationId(correlationId)
        .responseStreamId(responseStreamId)
        .version(configuration::CLIENT_SEMANTIC_VERSION)
        .putResponseChannel(responseChannel);

    return offerWithTimeout(messageAndHeaderLength(m_connectRequest));
}

bool ArchiveProxy::tryConnect(
    const std::string& responseChannel, const std::int32_t responseStreamId, const std::int64_t correlationId)
{
    applyHeaderAndResetPosition(m_connectRequest)
        .correlationId(correlationId)
        .responseStreamId(responseStreamId)
        .version(configuration::CLIENT_SEMANTIC_VERSION)
        .putResponseChannel(responseChannel);

    const std::int32_t length = codecs::MessageHeader::encodedLength() + m_connectRequest.encodedLength();
    return m_publication->offer(m_buffer, 0, length) > 0;
}

bool ArchiveProxy::connect(
    const std::string& responseChannel,
    const std::int32_t responseStreamId,
    const std::int64_t correlationId,
    AgentInvoker<ClientConductor>& aeronClientInvoker)
{
    applyHeaderAndResetPosition(m_connectRequest)
        .correlationId(correlationId)
        .responseStreamId(responseStreamId)
        .version(configuration::CLIENT_SEMANTIC_VERSION)
        .putResponseChannel(responseChannel);

    return offerWithTimeout(messageAndHeaderLength(m_connectRequest), aeronClientInvoker);
}

bool ArchiveProxy::closeSession(const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_closeSessionRequest)
        .controlSessionId(controlSessionId);

    return offer(messageAndHeaderLength(m_closeSessionRequest));
}

bool ArchiveProxy::startRecording(
    const std::string& channel,
    const std::int32_t streamId,
    const codecs::SourceLocation::Value sourceLocation,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_startRecordingRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .streamId(streamId)
        .sourceLocation(sourceLocation)
        .putChannel(channel);

    return offer(messageAndHeaderLength(m_startRecordingRequest));
}

bool ArchiveProxy::stopRecording(
    const std::string& channel,
    const std::int32_t streamId,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_stopRecordingRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .streamId(streamId)
        .putChannel(channel);

    return offer(messageAndHeaderLength(m_stopRecordingRequest));
}

bool ArchiveProxy::stopRecording(
    const std::int64_t subscriptionId,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_stopRecordingSubscriptionRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .subscriptionId(subscriptionId);

    return offer(messageAndHeaderLength(m_stopRecordingSubscriptionRequest));
}

bool ArchiveProxy::replay(
    const std::int64_t recordingId,
    const std::int64_t position,
    const std::int64_t length,
    const std::string& replayChannel,
    const std::int32_t replayStreamId,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_replayRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .recordingId(recordingId)
        .position(position)
        .length(length)
        .replayStreamId(replayStreamId)
        .putReplayChannel(replayChannel);

    return offer(messageAndHeaderLength(m_replayRequest));
}

bool ArchiveProxy::boundedReplay(
    const std::int64_t recordingId,
    const std::int64_t position,
    const std::int64_t length,
    const std::int32_t limitCounterId,
    const std::string& replayChannel,
    const std::int32_t replayStreamId,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_boundedReplayRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .recordingId(recordingId)
        .position(position)
        .length(length)
        .limitCounterId(limitCounterId)
        .replayStreamId(replayStreamId)
        .putReplayChannel(replayChannel);

    return offer(messageAndHeaderLength(m_boundedReplayRequest));
}

bool ArchiveProxy::stopReplay(
    const std::int64_t replaySessionId,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_stopReplayRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .replaySessionId(replaySessionId);

    return offer(messageAndHeaderLength(m_stopReplayRequest));
}

bool ArchiveProxy::stopAllReplays(
    const std::int64_t recordingId,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_stopAllReplaysRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .recordingId(recordingId);

    return offer(messageAndHeaderLength(m_stopAllReplaysRequest));
}

bool ArchiveProxy::listRecordings(
    const std::int64_t fromRecordingId,
    const std::int32_t recordCount,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_listRecordingsRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .fromRecordingId(fromRecordingId)
        .recordCount(recordCount);

    return offer(messageAndHeaderLength(m_listRecordingsRequest));
}

bool ArchiveProxy::listRecordingsForUri(
    const std::int64_t fromRecordingId,
    const std::int32_t recordCount,
    const std::string& channelFragment,
    const std::int32_t streamId,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_listRecordingsForUriRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .fromRecordingId(fromRecordingId)
        .recordCount(recordCount)
        .streamId(streamId)
        .putChannel(channelFragment);

    return offer(messageAndHeaderLength(m_listRecordingsForUriRequest));
}

bool ArchiveProxy::listRecording(
    const std::int64_t recordingId,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_listRecordingRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .recordingId(recordingId);

    return offer(messageAndHeaderLength(m_listRecordingRequest));
}

bool ArchiveProxy::listRecordingSubscriptions(
    const std::int32_t pseudoIndex,
    const std::int32_t subscriptionCount,
    const std::string& channelFragment,
    const std::int32_t streamId,
    const bool applyStreamId,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_listRecordingSubscriptionsRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .pseudoIndex(pseudoIndex)
        .subscriptionCount(subscriptionCount)
        .applyStreamId(applyStreamId
            ? codecs::BooleanType::Value::TRUE
            : codecs::BooleanType::Value::FALSE)
        .streamId(streamId)
        .putChannel(channelFragment);

    return offer(messageAndHeaderLength(m_listRecordingSubscriptionsRequest));
}

bool ArchiveProxy::extendRecording(
    const std::string& channel,
    const std::int32_t streamId,
    const codecs::SourceLocation::Value sourceLocation,
    const std::int64_t recordingId,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_extendRecordingRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .recordingId(recordingId)
        .streamId(streamId)
        .sourceLocation(sourceLocation)
        .putChannel(channel);

    return offer(messageAndHeaderLength(m_extendRecordingRequest));
}

bool ArchiveProxy::getRecordingPosition(
    const std::int64_t recordingId,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_recordingPositionRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .recordingId(recordingId);

    return offer(messageAndHeaderLength(m_recordingPositionRequest));
}

bool ArchiveProxy::getStopPosition(
    const std::int64_t recordingId,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_stopPositionRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .recordingId(recordingId);

    return offer(messageAndHeaderLength(m_stopPositionRequest));
}

bool ArchiveProxy::truncateRecording(
    const std::int64_t recordingId,
    const std::int64_t position,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_truncateRecordingRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .recordingId(recordingId)
        .position(position);

    return offer(messageAndHeaderLength(m_truncateRecordingRequest));
}

bool ArchiveProxy::findLastMatchingRecording(
    const std::int64_t minRecordingId,
    const std::string& channelFragment,
    const std::int32_t streamId,
    const std::int32_t sessionId,
    const std::int64_t correlationId,
    const std::int64_t controlSessionId)
{
    applyHeaderAndResetPosition(m_findLastMatchingRecordingRequest)
        .controlSessionId(controlSessionId)
        .correlationId(correlationId)
        .minRecordingId(minRecordingId)
        .sessionId(sessionId)
        .streamId(streamId)
        .putChannel(channelFragment);

    return offer(messageAndHeaderLength(m_findLastMatchingRecordingRequest));
}

namespace {

template <typename IdleStrategy, typename ResultHandler, typename Sentinel>
bool offerHelper(
    concurrent::AtomicBuffer& buffer,
    ExclusivePublication& publication,
    const std::int32_t length,
    IdleStrategy& idleStrategy,
    ResultHandler&& handleResult,
    Sentinel&& sentinel)
{
    while (true)
    {
        const std::int64_t result = publication.offer(buffer, 0, length);
        if (result > 0)
        {
            return true;
        }

        if (result == PUBLICATION_CLOSED)
        {
            throw ArchiveException("connection to the archive has been closed", SOURCEINFO);
        }

        if (result == MAX_POSITION_EXCEEDED)
        {
            throw ArchiveException("offer failed due to max position being reached", SOURCEINFO);
        }

        handleResult(result);

        if (!sentinel())
        {
            return false;
        }
        idleStrategy.idle();
    }
}

}

bool ArchiveProxy::offer(const std::int32_t length)
{
    auto remainingAttempts = m_retryAttempts;
    return offerHelper(m_buffer, *m_publication, length, m_idleStrategy,
        [] (const std::int64_t result)
        {
            if (result == NOT_CONNECTED)
            {
                throw ArchiveException("connection to the archive is no longer available", SOURCEINFO);
            }
        },
        [remainingAttempts] () mutable
        {
            return --remainingAttempts != 0;
        });
}

bool ArchiveProxy::offerWithTimeout(const std::int32_t length)
{
    const auto deadlineTs = std::chrono::high_resolution_clock::now() + m_connectTimeout;
    return offerHelper(m_buffer, *m_publication, length, m_idleStrategy,
        [] (const std::int64_t) {},
        [deadlineTs]
        {
            return std::chrono::high_resolution_clock::now() <= deadlineTs;
        });
}

bool ArchiveProxy::offerWithTimeout(const std::int32_t length, AgentInvoker<ClientConductor>& aeronClientInvoker)
{
    const auto deadlineTs = std::chrono::high_resolution_clock::now() + m_connectTimeout;
    return offerHelper(m_buffer, *m_publication, length, m_idleStrategy,
        [] (const std::int64_t) {},
        [deadlineTs, &aeronClientInvoker]
        {
            if (std::chrono::high_resolution_clock::now() > deadlineTs)
            {
                return false;
            }
            aeronClientInvoker.invoke();
            return true;
        });
}

template <typename Encoder>
Encoder& ArchiveProxy::applyHeaderAndResetPosition(Encoder& encoder)
{
    m_messageHeader
        .blockLength(Encoder::sbeBlockLength())
        .templateId(Encoder::sbeTemplateId());

    encoder.sbePosition(Encoder::sbeBlockLength());
    return encoder;
}

}
