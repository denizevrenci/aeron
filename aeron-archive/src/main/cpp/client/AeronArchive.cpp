#include "AeronArchive.h"

#include <cassert>

#include <ExclusivePublication.h>
#include <Publication.h>

#include <aeron_archive_client/ControlResponseCode.h>

namespace aeron::archive {

AeronArchive::AeronArchive(
    const Context& context,
    const std::int64_t controlSessionId,
    Aeron& aeron,
    ControlResponsePoller controlResponsePoller,
    ArchiveProxy archiveProxy,
    RecordingDescriptorPoller recordingDescriptorPoller,
    RecordingSubscriptionDescriptorPoller recordingSubscriptionDescriptorPoller) :
    m_controlSessionId(controlSessionId),
    m_messageTimeout(context.messageTimeout()),
    m_aeron(aeron),
    m_archiveProxy(std::move(archiveProxy)),
    m_errorHandler(context.errorHandler()),
    m_controlResponsePoller(std::move(controlResponsePoller)),
    m_recordingDescriptorPoller(std::move(recordingDescriptorPoller)),
    m_recordingSubscriptionDescriptorPoller(std::move(recordingSubscriptionDescriptorPoller))
{
}

AeronArchive::~AeronArchive()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_archiveProxy.closeSession(m_controlSessionId);
}

std::string AeronArchive::pollForErrorResponse()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    if (m_controlResponsePoller.poll() != 0 && m_controlResponsePoller.isPollComplete())
    {
        if (m_controlResponsePoller.controlSessionId() == m_controlSessionId &&
            m_controlResponsePoller.isControlResponse() &&
            m_controlResponsePoller.isCodeError())
        {
            return m_controlResponsePoller.errorMessage();
        }
    }

    return std::string();
}

void AeronArchive::checkForErrorResponse()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    if (m_controlResponsePoller.poll() != 0 && m_controlResponsePoller.isPollComplete())
    {
        if (m_controlResponsePoller.controlSessionId() == m_controlSessionId &&
            m_controlResponsePoller.isControlResponse() &&
            m_controlResponsePoller.isCodeError())
        {
            const ArchiveException ex(
                m_controlResponsePoller.errorMessage(),
                ArchiveException::ErrorCode(m_controlResponsePoller.relevantId()),
                SOURCEINFO);

            if (m_errorHandler)
            {
                m_errorHandler(ex);
            }
            else
            {
                throw ex;
            }
        }
    }
}

namespace {

std::string addSessionId(std::string channelUri, const std::int32_t sessionId)
{
    constexpr char SESSION_ID_PARAM_NAME[] = "session-id";

    channelUri += (channelUri.find('?') == channelUri.npos) ? '?' : '|';
    channelUri += SESSION_ID_PARAM_NAME;
    channelUri += '=';
    channelUri += std::to_string(sessionId);
    return channelUri;
}

}

std::shared_ptr<Publication> AeronArchive::addRecordedPublication(const std::string& channel, const std::int32_t streamId)
{
    auto registrationId = m_aeron.addPublication(channel, streamId);
    std::shared_ptr<Publication> publication = m_aeron.findPublication(registrationId);
    while (!publication)
    {
        m_idleStrategy.idle();
        publication = m_aeron.findPublication(registrationId);
    }
    if (!publication->isOriginal())
    {
        throw ArchiveException(
            "publication already added for channel=" + channel + " streamId=" + std::to_string(streamId), SOURCEINFO);
    }
    startRecording(addSessionId(channel, publication->sessionId()), streamId, codecs::SourceLocation::Value::LOCAL);
    return publication;
}

std::shared_ptr<ExclusivePublication> AeronArchive::addRecordedExclusivePublication(
    const std::string& channel,
    const std::int32_t streamId)
{
    auto registrationId = m_aeron.addExclusivePublication(channel, streamId);
    std::shared_ptr<ExclusivePublication> publication = m_aeron.findExclusivePublication(registrationId);
    while (!publication)
    {
        m_idleStrategy.idle();
        publication = m_aeron.findExclusivePublication(registrationId);
    }
    if (!publication->isOriginal())
    {
        throw ArchiveException(
            "publication already added for channel=" + channel + " streamId=" + std::to_string(streamId), SOURCEINFO);
    }
    startRecording(addSessionId(channel, publication->sessionId()), streamId, codecs::SourceLocation::Value::LOCAL);
    return publication;
}

std::int64_t AeronArchive::startRecording(const std::string& channel,
    const std::int32_t streamId,
    const codecs::SourceLocation::Value sourceLocation)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    const std::int64_t correlationId = m_aeron.nextCorrelationId();
    if (!m_archiveProxy.startRecording(channel, streamId, sourceLocation, correlationId, m_controlSessionId))
    {
        throw ArchiveException("failed to send start recording request", SOURCEINFO);
    }
    return pollForResponse(correlationId);
}

std::int64_t AeronArchive::extendRecording(
    const std::int64_t recordingId,
    const std::string& channel,
    const std::int32_t streamId,
    const codecs::SourceLocation::Value sourceLocation)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    const std::int64_t correlationId = m_aeron.nextCorrelationId();
    if (!m_archiveProxy.extendRecording(channel, streamId, sourceLocation, recordingId, correlationId, m_controlSessionId))
    {
        throw ArchiveException("failed to send extend recording request", SOURCEINFO);
    }
    return pollForResponse(correlationId);
}

void AeronArchive::stopRecording(const std::string& channel, const std::int32_t streamId)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    const std::int64_t correlationId = m_aeron.nextCorrelationId();
    if (!m_archiveProxy.stopRecording(channel, streamId, correlationId, m_controlSessionId))
    {
        throw ArchiveException("failed to send stop recording request", SOURCEINFO);
    }
    pollForResponse(correlationId);
}

void AeronArchive::stopRecording(const Publication& publication)
{
    std::string recordingChannel = addSessionId(publication.channel(), publication.sessionId());
    stopRecording(recordingChannel, publication.streamId());
}

void AeronArchive::stopRecording(const ExclusivePublication& publication)
{
    std::string recordingChannel = addSessionId(publication.channel(), publication.sessionId());
    stopRecording(recordingChannel, publication.streamId());
}

void AeronArchive::stopRecording(const std::int64_t subscriptionId)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    const std::int64_t correlationId = m_aeron.nextCorrelationId();
    if (!m_archiveProxy.stopRecording(subscriptionId, correlationId, m_controlSessionId))
    {
        throw ArchiveException("failed to send stop recording request", SOURCEINFO);
    }
    pollForResponse(correlationId);
}

std::int64_t AeronArchive::startReplay(
    const std::int64_t recordingId,
    const std::int64_t position,
    const std::int64_t length,
    const std::string& replayChannel,
    const std::int32_t replayStreamId)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    const std::int64_t correlationId = m_aeron.nextCorrelationId();
    if (!m_archiveProxy.replay(
            recordingId,
            position,
            length,
            replayChannel,
            replayStreamId,
            correlationId,
            m_controlSessionId))
    {
        throw ArchiveException("failed to send replay request", SOURCEINFO);
    }
    return pollForResponse(correlationId);
}

std::int64_t AeronArchive::startBoundedReplay(
    const std::int64_t recordingId,
    const std::int64_t position,
    const std::int64_t length,
    const std::int32_t limitCounterId,
    const std::string& replayChannel,
    const std::int32_t replayStreamId)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    const std::int64_t correlationId = m_aeron.nextCorrelationId();
    if (!m_archiveProxy.boundedReplay(
            recordingId,
            position,
            length,
            limitCounterId,
            replayChannel,
            replayStreamId,
            correlationId,
            m_controlSessionId))
    {
        throw ArchiveException("failed to send replay request", SOURCEINFO);
    }
    return pollForResponse(correlationId);
}

void AeronArchive::stopReplay(const std::int64_t replaySessionId)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    const std::int64_t correlationId = m_aeron.nextCorrelationId();
    if (!m_archiveProxy.stopReplay(replaySessionId, correlationId, m_controlSessionId))
    {
        throw ArchiveException("failed to send stop replay request", SOURCEINFO);
    }
    pollForResponse(correlationId);
}

void AeronArchive::stopAllReplays(const std::int64_t recordingId)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    const std::int64_t correlationId = m_aeron.nextCorrelationId();
    if (!m_archiveProxy.stopAllReplays(recordingId, correlationId, m_controlSessionId))
    {
        throw ArchiveException("failed to send stop replay request", SOURCEINFO);
    }
    pollForResponse(correlationId);
}

std::int64_t AeronArchive::replay(
    const std::int64_t recordingId,
    const std::int64_t position,
    const std::int64_t length,
    const std::string& replayChannel,
    const std::int32_t replayStreamId)
{
    auto replaySessionId = static_cast<uint32_t>(startReplay(
        recordingId,
        position,
        length,
        replayChannel,
        replayStreamId));
    return m_aeron.addSubscription(addSessionId(replayChannel, replaySessionId), replayStreamId);
}

std::int64_t AeronArchive::replay(
    const std::int64_t recordingId,
    const std::int64_t position,
    const std::int64_t length,
    const std::string& replayChannel,
    const std::int32_t replayStreamId,
    const on_available_image_t& availableImageHandler,
    const on_unavailable_image_t& unavailableImageHandler)
{
    auto replaySessionId = static_cast<uint32_t>(startReplay(
        recordingId,
        position,
        length,
        replayChannel,
        replayStreamId));
    return m_aeron.addSubscription(addSessionId(
        replayChannel, replaySessionId), replayStreamId, availableImageHandler, unavailableImageHandler);
}

std::int64_t AeronArchive::getRecordingPosition(const std::int64_t recordingId)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    const std::int64_t correlationId = m_aeron.nextCorrelationId();
    if (!m_archiveProxy.getRecordingPosition(recordingId, correlationId, m_controlSessionId))
    {
        throw ArchiveException("failed to send get recording position request", SOURCEINFO);
    }
    return pollForResponse(correlationId);
}

std::int64_t AeronArchive::getStopPosition(const std::int64_t recordingId)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    const std::int64_t correlationId = m_aeron.nextCorrelationId();
    if (!m_archiveProxy.getStopPosition(recordingId, correlationId, m_controlSessionId))
    {
        throw ArchiveException("failed to send get stop position request", SOURCEINFO);
    }
    return pollForResponse(correlationId);
}

std::int64_t AeronArchive::findLastMatchingRecording(
        const std::int64_t minRecordingId,
        const std::string& channelFragment,
        const std::int32_t streamId,
        const std::int32_t sessionId)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    const std::int64_t correlationId = m_aeron.nextCorrelationId();
    if (!m_archiveProxy.findLastMatchingRecording(
        minRecordingId, channelFragment, streamId, sessionId, correlationId, m_controlSessionId))
    {
        throw ArchiveException("failed to send find last matching recording request", SOURCEINFO);
    }
    return pollForResponse(correlationId);
}

void AeronArchive::truncateRecording(const std::int64_t recordingId, const std::int64_t position)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    const std::int64_t correlationId = m_aeron.nextCorrelationId();
    if (!m_archiveProxy.truncateRecording(recordingId, position, correlationId, m_controlSessionId))
    {
        throw ArchiveException("failed to send truncate recording request", SOURCEINFO);
    }
    pollForResponse(correlationId);
}

namespace codecs = client;

namespace {

std::string toString(const codecs::ControlResponseCode::Value v)
{
    return std::to_string(static_cast<std::underlying_type<decltype(v)>::type>(v));
}

void pollNextResponse(
    const std::int64_t correlationId,
    const std::chrono::high_resolution_clock::time_point deadlineTs,
    ControlResponsePoller& poller,
    concurrent::YieldingIdleStrategy& idleStrategy,
    Aeron& aeron)
{
    while (true)
    {
        const int fragments = poller.poll();

        if (poller.isPollComplete())
        {
            break;
        }

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
                "awaiting response - correlationId=" + std::to_string(correlationId), SOURCEINFO);
        }

        idleStrategy.idle();
        aeron.conductorAgentInvoker().invoke();
    }
}

}

std::int64_t AeronArchive::pollForResponse(const std::int64_t correlationId)
{
    const auto deadlineTs = std::chrono::high_resolution_clock::now() + m_messageTimeout;
    auto& poller = m_controlResponsePoller;

    while (true)
    {
        pollNextResponse(correlationId, deadlineTs, poller, m_idleStrategy, m_aeron);

        if (poller.controlSessionId() != m_controlSessionId || !poller.isControlResponse())
        {
            m_aeron.conductorAgentInvoker().invoke();
            continue;
        }

        if (poller.isCodeError())
        {
            const ArchiveException ex("response for correlationId=" + std::to_string(correlationId) +
                ", error: " + poller.errorMessage(),
                ArchiveException::ErrorCode(poller.relevantId()),
                SOURCEINFO);

            if (poller.correlationId() == correlationId)
            {
                throw ex;
            }
            else if (m_errorHandler)
            {
                m_errorHandler(ex);
            }
        }
        else if (poller.correlationId() == correlationId)
        {
            if (!poller.isCodeOk())
            {
                throw ArchiveException("unexpected response code: " + toString(poller.code()), SOURCEINFO);
            }

            return poller.relevantId();
        }
    }
}

AsyncConnect::AsyncConnect(const Context& ctx, Aeron& aeron) :
    m_ctx(ctx),
    m_aeron(aeron),
    m_subscriptionId(aeron.addSubscription(
        ctx.controlResponseChannel(), ctx.controlResponseStreamId())),
    m_publicationId(aeron.addExclusivePublication(
        ctx.controlRequestChannel(), ctx.controlRequestStreamId()))
{
}

bool AsyncConnect::poll()
{
    if (!m_controlResponsePollerReady)
    {
        if (!m_subscription)
        {
            if (m_subscription = m_aeron.findSubscription(m_subscriptionId); m_subscription)
            {
                m_controlResponsePoller.emplace(m_subscription);
                if (m_controlResponsePoller->subscription().isConnected())
                {
                    m_controlResponsePollerReady = true;
                }
            }
        }
        else if (m_controlResponsePoller->subscription().isConnected())
        {
            m_controlResponsePollerReady = true;
        }
    }

    if (!m_archiveProxyReady)
    {
        if (m_correlationId == aeron::NULL_VALUE)
        {
            if (!m_publication)
            {
                if (m_publication = m_aeron.findExclusivePublication(m_publicationId); m_publication)
                {
                    m_archiveProxy.emplace(ArchiveProxy(m_publication, m_ctx.messageTimeout()));
                    if (m_archiveProxy->publication().isConnected())
                    {
                        m_correlationId = m_aeron.nextCorrelationId();
                        if (m_archiveProxy->tryConnect(
                            m_ctx.controlResponseChannel(), m_ctx.controlResponseStreamId(), m_correlationId))
                        {
                            m_archiveProxyReady = true;
                        }
                    }
                }
            }
            else if (m_archiveProxy->publication().isConnected())
            {
                m_correlationId = m_aeron.nextCorrelationId();
                if (m_archiveProxy->tryConnect(
                    m_ctx.controlResponseChannel(), m_ctx.controlResponseStreamId(), m_correlationId))
                {
                    m_archiveProxyReady = true;
                }
            }
        }
        else if (m_archiveProxy->tryConnect(
            m_ctx.controlResponseChannel(), m_ctx.controlResponseStreamId(), m_correlationId))
        {
            m_archiveProxyReady = true;
        }
    }

    if (m_controlResponsePollerReady && m_archiveProxyReady)
    {
        auto& poller = *m_controlResponsePoller;

        poller.poll();

        if (poller.isPollComplete() &&
            poller.correlationId() == m_correlationId &&
            poller.isControlResponse())
        {
            if (!poller.isCodeOk())
            {
                if (poller.isCodeError())
                {
                    throw ArchiveException(
                        "error: " + poller.errorMessage(),
                        ArchiveException::ErrorCode(poller.relevantId()),
                        SOURCEINFO);
                }

                throw ArchiveException(
                    "unexpected response: code=" + toString(poller.code()), SOURCEINFO);
            }
            return true;
        }
    }

    return false;
}

AeronArchive AsyncConnect::makeArchive()
{
    auto& poller = *m_controlResponsePoller;
    assert(
        m_controlResponsePollerReady &&
        m_archiveProxyReady &&
        poller.isPollComplete() &&
        poller.correlationId() == m_correlationId &&
        poller.isControlResponse() &&
        poller.isCodeOk());

    const std::int64_t sessionId = poller.controlSessionId();
    constexpr std::int32_t FRAGMENT_LIMIT = 10;

    return {
        m_ctx,
        sessionId,
        m_aeron,
        std::move(poller),
        std::move(*m_archiveProxy),
        RecordingDescriptorPoller(
            m_subscription, m_ctx.errorHandler(), sessionId, FRAGMENT_LIMIT),
        RecordingSubscriptionDescriptorPoller(
            m_subscription, m_ctx.errorHandler(), sessionId, FRAGMENT_LIMIT)};
}

}
