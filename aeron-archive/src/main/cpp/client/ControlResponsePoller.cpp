#include "ControlResponsePoller.h"

#include <concurrent/AtomicBuffer.h>
#include <concurrent/logbuffer/Header.h>
#include <Subscription.h>

#include <aeron_archive_client/ControlResponse.h>
#include <aeron_archive_client/MessageHeader.h>
#include <aeron_archive_client/RecordingDescriptor.h>

#include "ArchiveException.h"

namespace aeron::archive {

ControlResponsePoller::ControlResponsePoller(std::shared_ptr<Subscription> subscription, const int fragmentLimit) :
    m_subscription(std::move(subscription)),
    m_fragmentLimit(fragmentLimit)
{
}

ControlResponsePoller::ControlResponsePoller(std::shared_ptr<Subscription> subscription) :
    ControlResponsePoller(std::move(subscription), FRAGMENT_LIMIT)
{
}

bool ControlResponsePoller::isControlResponse() const
{
    return m_templateId == codecs::ControlResponse::sbeTemplateId();
}

std::int32_t ControlResponsePoller::poll()
{
    m_controlSessionId = -1;
    m_correlationId = -1;
    m_relevantId = -1;
    m_templateId = -1;
    m_errorMessage.clear();
    m_pollComplete = false;

    return m_subscription->controlledPoll(m_fragmentAssembler.handler(
        [this] (concurrent::AtomicBuffer& buffer, const std::int32_t offset, const std::int32_t length, Header& header)
        {
            return onFragment(buffer, offset, length, header);
        }),
        m_fragmentLimit);
}

ControlledPollAction ControlResponsePoller::onFragment(
    concurrent::AtomicBuffer& buffer, const std::int32_t offset, const std::int32_t length, Header&)
{
    if (m_pollComplete)
    {
        return ControlledPollAction::ABORT;
    }

    codecs::MessageHeader messageHeader(
        buffer.sbeData() + offset, length, codecs::MessageHeader::sbeSchemaVersion());

    const std::uint16_t schemaId = messageHeader.schemaId();
    if (schemaId != messageHeader.sbeSchemaId())
    {
        throw ArchiveException(
            "expected schemaId=" + std::to_string(messageHeader.sbeSchemaId()) + ", actual=" + std::to_string(schemaId), SOURCEINFO);
    }

    m_templateId = messageHeader.templateId();
    if (isControlResponse())
    {
        codecs::ControlResponse controlResponse(
            buffer.sbeData() + offset + codecs::MessageHeader::encodedLength(),
            static_cast<std::uint64_t>(length) - codecs::MessageHeader::encodedLength(),
            messageHeader.blockLength(),
            messageHeader.version());

        m_controlSessionId = controlResponse.controlSessionId();
        m_correlationId = controlResponse.correlationId();
        m_relevantId = controlResponse.relevantId();
        m_code = controlResponse.code();

        m_errorMessage = controlResponse.getErrorMessageAsString();

        m_pollComplete = true;

        return ControlledPollAction::BREAK;
    }

    return ControlledPollAction::CONTINUE;
}

}
