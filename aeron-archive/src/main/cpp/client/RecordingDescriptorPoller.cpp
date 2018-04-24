#include "RecordingDescriptorPoller.h"

#include <concurrent/AtomicBuffer.h>
#include <concurrent/logbuffer/Header.h>
#include <Subscription.h>

#include <aeron_archive_client/ControlResponse.h>
#include <aeron_archive_client/ControlResponseCode.h>
#include <aeron_archive_client/MessageHeader.h>
#include <aeron_archive_client/RecordingDescriptor.h>

#include "ArchiveException.h"

namespace aeron::archive {

RecordingDescriptorPoller::RecordingDescriptorPoller(
    std::shared_ptr<Subscription> subscription,
    on_error_t errorHandler,
    const std::int64_t controlSessionId,
    const int fragmentLimit) :
        m_controlSessionId(controlSessionId),
        m_fragmentLimit(fragmentLimit),
        m_subscription(std::move(subscription)),
        m_errorHandler(std::move(errorHandler))
{
}

std::int32_t RecordingDescriptorPoller::poll()
{
    m_isDispatchComplete = false;

    return m_subscription->controlledPoll(m_fragmentAssembler.handler(
        [this] (concurrent::AtomicBuffer& buffer, const std::int32_t offset, const std::int32_t length, Header& header)
        {
            return onFragment(buffer, offset, length, header);
        }),
        m_fragmentLimit);
}

ControlledPollAction RecordingDescriptorPoller::onFragment(
    concurrent::AtomicBuffer& buffer, const std::int32_t offset, const std::int32_t length, Header&)
{
    namespace codecs = client;

    codecs::MessageHeader messageHeader(
        buffer.sbeData() + offset, length, codecs::MessageHeader::sbeSchemaVersion());

    const std::uint16_t schemaId = messageHeader.schemaId();
    if (schemaId != messageHeader.sbeSchemaId())
    {
        throw ArchiveException(
            "expected schemaId=" + std::to_string(messageHeader.sbeSchemaId()) + ", actual=" + std::to_string(schemaId), SOURCEINFO);
    }

    const std::uint16_t templateId = messageHeader.templateId();
    switch (templateId)
    {
        case codecs::ControlResponse::sbeTemplateId():
        {
            codecs::ControlResponse response(
                buffer.sbeData() + offset + codecs::MessageHeader::encodedLength(),
                static_cast<std::uint64_t>(length) - codecs::MessageHeader::encodedLength(),
                messageHeader.blockLength(),
                messageHeader.version());

            if (response.controlSessionId() == m_controlSessionId)
            {
                const codecs::ControlResponseCode::Value code = response.code();
                const std::int64_t correlationId = response.correlationId();

                if (codecs::ControlResponseCode::Value::RECORDING_UNKNOWN == code &&
                    correlationId == m_correlationId)
                {
                    m_isDispatchComplete = true;
                    return ControlledPollAction::BREAK;
                }

                if (codecs::ControlResponseCode::Value::ERROR == code)
                {
                    const ArchiveException ex(
                        "response for correlationId=" + std::to_string(m_correlationId) +
                            ", error: " + response.errorMessage(),
                        ArchiveException::ErrorCode(response.relevantId()),
                        SOURCEINFO);

                    if (correlationId == m_correlationId)
                    {
                        throw ex;
                    }
                    else if (m_errorHandler)
                    {
                        m_errorHandler(ex);
                    }
                }
            }
            break;
        }

        case codecs::RecordingDescriptor::sbeTemplateId():
        {
            codecs::RecordingDescriptor descriptor(
                buffer.sbeData() + offset + codecs::MessageHeader::encodedLength(),
                static_cast<std::uint64_t>(length) - codecs::MessageHeader::encodedLength(),
                messageHeader.blockLength(),
                messageHeader.version());

            {
                const std::int64_t correlationId = descriptor.correlationId();
                if (descriptor.controlSessionId() == m_controlSessionId &&
                    correlationId == m_correlationId)
                {
                    m_consumer(
                        m_controlSessionId,
                        correlationId,
                        descriptor.recordingId(),
                        descriptor.startTimestamp(),
                        descriptor.stopTimestamp(),
                        descriptor.startPosition(),
                        descriptor.stopPosition(),
                        descriptor.initialTermId(),
                        descriptor.segmentFileLength(),
                        descriptor.termBufferLength(),
                        descriptor.mtuLength(),
                        descriptor.sessionId(),
                        descriptor.streamId(),
                        descriptor.strippedChannel(),
                        descriptor.originalChannel(),
                        descriptor.sourceIdentity());
                }
            }

            if (0 == --m_remainingRecordCount)
            {
                m_isDispatchComplete = true;
                return ControlledPollAction::BREAK;
            }
            break;
        }
    }

    return ControlledPollAction::CONTINUE;
}

}
