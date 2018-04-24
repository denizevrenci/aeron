#ifndef AERON_ARCHIVE_CONTROLRESPONSEPOLLER_H
#define AERON_ARCHIVE_CONTROLRESPONSEPOLLER_H

#include <memory>
#include <string>

#include <FragmentAssembler.h>

#include <aeron_archive_client/ControlResponseCode.h>

namespace aeron {

class Subscription;

namespace concurrent {

class AtomicBuffer;

namespace logbuffer {

class Header;

}}

namespace archive {

namespace codecs = client;

class ControlResponsePoller
{
public:
    /**
     * Create a poller for a given subscription to an archive for control response messages.
     *
     * @param subscription  to poll for new events.
     * @param fragmentLimit to apply when polling.
     */
    ControlResponsePoller(std::shared_ptr<Subscription> subscription, int fragmentLimit);

    /**
     * Create a poller for a given subscription to an archive for control response messages with a default
     * fragment limit for polling as #FRAGMENT_LIMIT.
     *
     * @param subscription  to poll for new events.
     */
    ControlResponsePoller(std::shared_ptr<Subscription> subscription);

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
     * Poll for control response events.
     *
     * @return the number of fragments read during the operation. Zero if no events are available.
     */
    std::int32_t poll();

    /**
     * Control session id of the last polled message or NULL_VALUE if poll returned nothing.
     *
     * @return control session id of the last polled message or NULL_VALUE if poll returned nothing.
     */
    std::int64_t controlSessionId() const
    {
        return m_controlSessionId;
    }

    /**
     * Correlation id of the last polled message or NULL_VALUE if poll returned nothing.
     *
     * @return correlation id of the last polled message or NULL_VALUE if poll returned nothing.
     */
    std::int64_t correlationId() const
    {
        return m_correlationId;
    }

    /**
     * Get the relevant id returned with the response, e.g. replay session id.
     *
     * @return the relevant id returned with the response.
     */
    std::int64_t relevantId() const
    {
        return m_relevantId;
    }

    /**
     * Has the last polling action received a complete message?
     *
     * @return true if the last polling action received a complete message?
     */
    bool isPollComplete() const
    {
        return m_pollComplete;
    }

    /**
     * Get the template id of the last received message.
     *
     * @return the template id of the last received message.
     */
    std::int32_t templateId() const
    {
        return m_templateId;
    }

    /**
     * Was last received message a Control Response?
     *
     * @return whether the last received message was a Control Response.
     */
    bool isControlResponse() const;

    /**
     * Get the response code of the last response.
     *
     * @return the response code of the last response.
     */
    codecs::ControlResponseCode::Value code() const
    {
        return m_code;
    }

    /**
     * Did the last received control response have a response code of OK?
     *
     * @return whether the last received control response had a response code of OK?
     */
    inline bool isCodeOk() const
    {
        return m_code == codecs::ControlResponseCode::Value::OK;
    }

    /**
     * Did the last received control response have a response code of ERROR?
     *
     * @return whether the last received control response had a response code of ERROR?
     */
    inline bool isCodeError() const
    {
        return m_code == codecs::ControlResponseCode::Value::ERROR;
    }

    /**
     * Get the error message of the last response.
     *
     * @return the error message of the last response.
     */
    const std::string& errorMessage() const
    {
        return m_errorMessage;
    }

private:
    static constexpr int NULL_VALUE = -1;

    /**
     * Limit to apply when polling response messages.
     */
    static constexpr int FRAGMENT_LIMIT = 10;

    std::shared_ptr<Subscription> m_subscription;
    FragmentAssembler m_fragmentAssembler;
    std::int64_t m_controlSessionId = NULL_VALUE;
    std::int64_t m_correlationId = NULL_VALUE;
    std::int64_t m_relevantId = NULL_VALUE;
    std::int32_t m_templateId = NULL_VALUE;
    const std::int32_t m_fragmentLimit;
    codecs::ControlResponseCode::Value m_code;
    std::string m_errorMessage;
    bool m_pollComplete = false;

    ControlledPollAction onFragment(
        concurrent::AtomicBuffer& buffer, std::int32_t offset, std::int32_t length, Header& header);
};

}}

#endif // AERON_ARCHIVE_CONTROLRESPONSEPOLLER_H
