#ifndef AERON_ARCHIVE_RECORDINGEVENTSLISTENER_H
#define AERON_ARCHIVE_RECORDINGEVENTSLISTENER_H

#include <string>
#include <utility>

#include <Subscription.h>

#include <concurrent/AtomicBuffer.h>
#include <concurrent/logbuffer/Header.h>

#include <aeron_archive_client/MessageHeader.h>
#include <aeron_archive_client/RecordingStarted.h>
#include <aeron_archive_client/RecordingProgress.h>
#include <aeron_archive_client/RecordingStopped.h>

#include "ArchiveException.h"

namespace aeron::archive {

namespace codecs = client;

class RecordingEventsListener
{
public:
    RecordingEventsListener(std::shared_ptr<Subscription> subscription) :
        m_subscription(std::move(subscription))
    {
    }

    template <typename OnStart, typename OnProgress, typename OnStop>
    int poll(OnStart&& onStart, OnProgress&& onProgress, OnStop&& onStop, const int fragmentLimit) {
        return m_subscription->poll(
                [this,
                 onStartF = std::forward<OnStart>(onStart),
                 onProgressF = std::forward<OnProgress>(onProgress),
                 onStopF = std::forward<OnStop>(onStop)]
                (const concurrent::AtomicBuffer& buffer, const std::int32_t offset, const std::int32_t length, Header& header) {
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
                case codecs::RecordingStarted::sbeTemplateId():
                {
                    codecs::RecordingStarted event(
                        buffer.sbeData() + offset + codecs::MessageHeader::encodedLength(),
                        static_cast<std::uint64_t>(length) - codecs::MessageHeader::encodedLength(),
                        messageHeader.blockLength(),
                        messageHeader.version());

                    onStartF(
                        event.recordingId(),
                        event.startPosition(),
                        event.sessionId(),
                        event.streamId(),
                        event.channel(),
                        event.sourceIdentity());
                    break;
                }

                case codecs::RecordingProgress::sbeTemplateId():
                {
                    codecs::RecordingProgress event(
                        buffer.sbeData() + offset + codecs::MessageHeader::encodedLength(),
                        static_cast<std::uint64_t>(length) - codecs::MessageHeader::encodedLength(),
                        messageHeader.blockLength(),
                        messageHeader.version());

                    onProgressF(
                        event.recordingId(),
                        event.startPosition(),
                        event.position());
                    break;
                }

                case codecs::RecordingStopped::sbeTemplateId():
                {
                    codecs::RecordingStopped event(
                        buffer.sbeData() + offset + codecs::MessageHeader::encodedLength(),
                        static_cast<std::uint64_t>(length) - codecs::MessageHeader::encodedLength(),
                        messageHeader.blockLength(),
                        messageHeader.version());

                    onStopF(
                        event.recordingId(),
                        event.startPosition(),
                        event.stopPosition());
                    break;
                }
            }
        },
        fragmentLimit);
    }

    const std::shared_ptr<Subscription>& subscription() const {
        return m_subscription;
    }

    std::shared_ptr<Subscription>& subscription() {
        return m_subscription;
    }

private:
    std::shared_ptr<Subscription> m_subscription;
};

}

#endif // AERON_ARCHIVE_RECORDINGEVENTSLISTENER_H
