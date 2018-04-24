#ifndef AERON_ARCHIVE_RECORDINGSUBSCRIPTIONDESCRIPTORPOLLER_H
#define AERON_ARCHIVE_RECORDINGSUBSCRIPTIONDESCRIPTORPOLLER_H

#include <functional>
#include <memory>
#include <string>

#include <FragmentAssembler.h>

#include "ErrorHandler.h"
#include "RecordingSubscriptionDescriptorConsumer.h"

namespace aeron {

class Subscription;

namespace concurrent {

class AtomicBuffer;

namespace logbuffer {

class Header;

}}

namespace archive {

class RecordingSubscriptionDescriptorPoller
{
public:
    /**
     * Create a poller for a given subscription to an archive for control response messages.
     *
     * @param subscription     to poll for new events.
     * @param errorHandler     to call for asynchronous errors.
     * @param controlSessionId to filter the responses.
     * @param fragmentLimit    to apply for each polling operation.
     */
    RecordingSubscriptionDescriptorPoller(
        std::shared_ptr<Subscription> subscription,
        on_error_t errorHandler,
        std::int64_t controlSessionId,
        int fragmentLimit);

    /**
     * Get the Subscription used for polling responses.
     *
     * @return the Subscription used for polling responses.
     */
    Subscription& subscription()
    {
        return *m_subscription;
    }

    /**
     * Get the Subscription used for polling responses.
     *
     * @return the Subscription used for polling responses.
     */
    const Subscription& subscription() const
    {
        return *m_subscription;
    }

    /**
     * Poll for recording subscriptions.
     *
     * @return the number of fragments read during the operation. Zero if no events are available.
     */
    std::int32_t poll();

    /**
     * Control session id for filtering responses.
     *
     * @return control session id for filtering responses.
     */
    std::int64_t controlSessionId() const
    {
        return m_controlSessionId;
    }

    /**
     * Is the dispatch of descriptors complete?
     *
     * @return true if the dispatch of descriptors complete?
     */
    bool isDispatchComplete() const
    {
        return m_isDispatchComplete;
    }

    /**
     * Get the expected number of remaining subscriptions.
     *
     * @return the expected number of remaining subscriptions.
     */
    std::int32_t remainingSubscriptionCount() const
    {
        return m_remainingSubscriptionCount;
    }

    /**
     * Reset the poller to dispatch the descriptors returned from a query.
     *
     * @param correlationId for the response.
     * @param recordCount   of descriptors to expect.
     * @param consumer      to which the recording subscription descriptors are to be dispatched.
     */
    template <typename RecordingSubscriptionDescriptorConsumer>
    void reset(
        const std::int64_t correlationId,
        const std::int32_t subscriptionCount,
        RecordingSubscriptionDescriptorConsumer&& consumer)
    {
        m_correlationId = correlationId;
        m_consumer = std::ref(consumer);
        m_remainingSubscriptionCount = subscriptionCount;
        m_isDispatchComplete = false;
    }

private:
    std::int64_t m_controlSessionId;
    int m_fragmentLimit;
    std::shared_ptr<Subscription> m_subscription;
    FragmentAssembler m_fragmentAssembler;
    on_error_t m_errorHandler;

    std::int64_t m_correlationId;
    std::int32_t m_remainingSubscriptionCount;
    bool m_isDispatchComplete = false;
    on_recording_subscription_descriptor_t m_consumer;

    ControlledPollAction onFragment(
        concurrent::AtomicBuffer& buffer, std::int32_t offset, std::int32_t length, Header& header);
};

}}

#endif // AERON_ARCHIVE_RECORDINGSUBSCRIPTIONDESCRIPTORPOLLER_H
