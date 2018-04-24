#include "RecordingSubscriptionDescriptorPoller.h"

#include <concurrent/AtomicBuffer.h>
#include <concurrent/logbuffer/Header.h>
#include <Subscription.h>

#include <aeron_archive_client/ControlResponse.h>
#include <aeron_archive_client/ControlResponseCode.h>
#include <aeron_archive_client/MessageHeader.h>
#include <aeron_archive_client/RecordingSubscriptionDescriptor.h>

#include "ArchiveException.h"

namespace aeron::archive {

RecordingSubscriptionDescriptorPoller::RecordingSubscriptionDescriptorPoller(
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

std::int32_t RecordingSubscriptionDescriptorPoller::poll()
{
    m_isDispatchComplete = false;

    return m_subscription->controlledPoll(m_fragmentAssembler.handler(
        [this] (concurrent::AtomicBuffer& buffer, const std::int32_t offset, const std::int32_t length, Header& header)
        {
            return onFragment(buffer, offset, length, header);
        }),
        m_fragmentLimit);
}

ControlledPollAction RecordingSubscriptionDescriptorPoller::onFragment(
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

                if (codecs::ControlResponseCode::Value::SUBSCRIPTION_UNKNOWN == code &&
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

        case codecs::RecordingSubscriptionDescriptor::sbeTemplateId():
        {
            codecs::RecordingSubscriptionDescriptor descriptor(
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
                        descriptor.subscriptionId(),
                        descriptor.streamId(),
                        descriptor.strippedChannel());
                }
            }

            if (0 == --m_remainingSubscriptionCount)
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
