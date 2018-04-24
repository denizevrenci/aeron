#ifndef AERON_ARCHIVE_ARCHIVECONTEXT_H
#define AERON_ARCHIVE_ARCHIVECONTEXT_H

#include <chrono>
#include <string>

#include "ArchiveConfiguration.h"
#include "ErrorHandler.h"

namespace aeron::archive {

class Context {
public:
    /**
     * Set the message timeout in nanoseconds to wait for sending or receiving a message.
     *
     * @param messageTimeout to wait for sending or receiving a message.
     * @return *this for a fluent API.
     */
    Context& messageTimeout(const std::chrono::nanoseconds messageTimeout)
    {
        m_messageTimeout = messageTimeout;
        return *this;
    }

    /**
     * The message timeout in nanoseconds to wait for sending or receiving a message.
     *
     * @return the message timeout in nanoseconds to wait for sending or receiving a message.
     */
    [[nodiscard]] std::chrono::nanoseconds messageTimeout() const
    {
        return m_messageTimeout;
    }

    /**
     * Should the control streams use sparse file term buffers.
     *
     * @param controlTermBufferSparse for the control stream.
     * @return *this for a fluent API.
     */
    Context& controlTermBufferSparse(bool controlTermBufferSparse)
    {
        m_controlTermBufferSparse = controlTermBufferSparse;
        return *this;
    }

    /**
     * Should the control streams use sparse file term buffers.
     *
     * @return true if the control stream should use sparse file term buffers.
     */
    [[nodiscard]] bool controlTermBufferSparse() const
    {
        return m_controlTermBufferSparse;
    }

    /**
     * Set the channel URI on which the recording events publication will publish.
     * <p>
     * To support dynamic subscribers then this can be set to multicast or MDC (Multi-Destination-Cast) if
     * multicast cannot be supported for on the available the network infrastructure.
     *
     * @param recordingEventsChannel channel URI on which the recording events publication will publish.
     * @return *this for a fluent API.
     */
    Context& recordingEventsChannel(const std::string& recordingEventsChannel)
    {
        m_recordingEventsChannel = recordingEventsChannel;
        return *this;
    }

    /**
     * Get the channel URI on which the recording events publication will publish.
     *
     * @return the channel URI on which the recording events publication will publish.
     */
    [[nodiscard]] const std::string& recordingEventsChannel() const
    {
        return m_recordingEventsChannel;
    }

    /**
     * Set the stream id on which the recording events publication will publish.
     *
     * @param recordingEventsStreamId stream id on which the recording events publication will publish.
     * @return *this for a fluent API.
     */
    Context& recordingEventsStreamId(const std::int32_t recordingEventsStreamId)
    {
        m_recordingEventsStreamId = recordingEventsStreamId;
        return *this;
    }

    /**
     * Get the stream id on which the recording events publication will publish.
     *
     * @return the stream id on which the recording events publication will publish.
     */
    [[nodiscard]] std::int32_t recordingEventsStreamId() const
    {
        return m_recordingEventsStreamId;
    }

    /**
     * Set the channel parameter for the control request channel.
     *
     * @param channel parameter for the control request channel.
     * @return *this for a fluent API.
     */
    Context& controlRequestChannel(const std::string& channel)
    {
        m_controlRequestChannel = channel;
        return *this;
    }

    /**
     * Get the channel parameter for the control request channel.
     *
     * @return the channel parameter for the control request channel.
     */
    [[nodiscard]] const std::string& controlRequestChannel() const
    {
        return m_controlRequestChannel;
    }

    /**
     * Set the stream id for the control request channel.
     *
     * @param streamId for the control request channel.
     * @return *this for a fluent API
     */
    Context& controlRequestStreamId(const std::int32_t streamId)
    {
        m_controlRequestStreamId = streamId;
        return *this;
    }

    /**
     * Get the stream id for the control request channel.
     *
     * @return the stream id for the control request channel.
     */
    [[nodiscard]] std::int32_t controlRequestStreamId() const
    {
        return m_controlRequestStreamId;
    }

    /**
     * Set the channel parameter for the control response channel.
     *
     * @param channel parameter for the control response channel.
     * @return *this for a fluent API.
     */
    Context& controlResponseChannel(const std::string& channel)
    {
        m_controlResponseChannel = channel;
        return *this;
    }

    /**
     * Get the channel parameter for the control response channel.
     *
     * @return the channel parameter for the control response channel.
     */
    [[nodiscard]] const std::string& controlResponseChannel() const
    {
        return m_controlResponseChannel;
    }

    /**
     * Set the stream id for the control response channel.
     *
     * @param streamId for the control response channel.
     * @return *this for a fluent API
     */
    Context& controlResponseStreamId(const std::int32_t streamId)
    {
        m_controlResponseStreamId = streamId;
        return *this;
    }

    /**
     * Get the stream id for the control response channel.
     *
     * @return the stream id for the control response channel.
     */
    [[nodiscard]] std::int32_t controlResponseStreamId() const
    {
        return m_controlResponseStreamId;
    }

    /**
     * Set the term buffer length for the control stream.
     *
     * @param controlTermBufferLength for the control streams.
     * @return *this for a fluent API.
     */
    Context& controlTermBufferLength(const std::int32_t controlTermBufferLength)
    {
        m_controlTermBufferLength = controlTermBufferLength;
        return *this;
    }

    /**
     * Get the term buffer length for the control streams.
     *
     * @return the term buffer length for the control streams.
     */
    [[nodiscard]] std::int32_t controlTermBufferLength() const
    {
        return m_controlTermBufferLength;
    }

    /**
     * Set the MTU length for the control streams.
     *
     * @param controlMtuLength for the control streams.
     * @return *this for a fluent API.
     */
    Context& controlMtuLength(const std::int32_t controlMtuLength)
    {
        m_controlMtuLength = controlMtuLength;
        return *this;
    }

    /**
     * Get the MTU length for the control streams.
     *
     * @return the MTU length for the control streams.
     */
    [[nodiscard]] std::int32_t controlMtuLength() const
    {
        return m_controlMtuLength;
    }

    /**
     * Handle errors returned asynchronously from the archive for a control session.
     *
     * @param errorHandler method to handle objects of type Throwable.
     * @return this for a fluent API.
     */
    Context& errorHandler(on_error_t errorHandler)
    {
        m_errorHandler = std::move(errorHandler);
        return *this;
    }

    /**
     * Get the error handler that will be called for asynchronous errors.
     *
     * @return the error handler that will be called for asynchronous errors.
     */
    [[nodiscard]] const on_error_t& errorHandler() const
    {
        return m_errorHandler;
    }

private:
    std::chrono::nanoseconds m_messageTimeout = configuration::defaults::MESSAGE_TIMEOUT;
    bool m_controlTermBufferSparse = configuration::defaults::CONTROL_TERM_BUFFER_SPARSE;
    std::string m_recordingEventsChannel = configuration::defaults::RECORDING_EVENTS_CHANNEL;
    std::int32_t m_recordingEventsStreamId = configuration::defaults::RECORDING_EVENTS_STREAM_ID;
    std::string m_controlRequestChannel = configuration::defaults::CONTROL_REQUEST_CHANNEL;
    std::int32_t m_controlRequestStreamId = configuration::defaults::CONTROL_REQUEST_STREAM_ID;
    std::string m_controlResponseChannel = configuration::defaults::CONTROL_RESPONSE_CHANNEL;
    std::int32_t m_controlResponseStreamId = configuration::defaults::CONTROL_RESPONSE_STREAM_ID;
    std::int32_t m_controlTermBufferLength = configuration::defaults::CONTROL_TERM_BUFFER_LENGTH;
    std::int32_t m_controlMtuLength = configuration::defaults::CONTROL_MTU_LENGTH;
    on_error_t m_errorHandler;
};

}

#endif // AERON_ARCHIVE_ARCHIVECONTEXT_H
