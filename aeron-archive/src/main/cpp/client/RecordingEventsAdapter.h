#ifndef AERON_ARCHIVE_RECORDINGEVENTSADAPTER_H
#define AERON_ARCHIVE_RECORDINGEVENTSADAPTER_H

#include <functional>
#include <memory>
#include <string>

#include "RecordingEventsListener.h"

namespace aeron::archive { namespace recordingEvents {

/**
 * Fired when a recording is started.
 *
 * @param recordingId    assigned to the new recording.
 * @param startPosition  in the stream at which the recording started.
 * @param sessionId      of the publication being recorded.
 * @param streamId       of the publication being recorded.
 * @param channel        of the publication being recorded.
 * @param sourceIdentity of the publication being recorded.
 */
typedef std::function<void(
    std::int64_t, // recordingId
    std::int64_t, // startPosition
    std::int32_t, // sessionId
    std::int32_t, // streamId
    const std::string&, // channel
    const std::string&)> // sourceIdentity
    on_start_t;

/**
 * Progress indication of an active recording.
 *
 * @param recordingId   for which progress is being reported.
 * @param startPosition in the stream at which the recording started.
 * @param position      reached in recording the publication.
 */
typedef std::function<void(
    std::int64_t, // recordingId
    std::int64_t, // startPosition
    std::int64_t)> // position
    on_progress_t;

/**
 * Fired when a recording is stopped.
 *
 * @param recordingId   of the publication that has stopped recording.
 * @param startPosition in the stream at which the recording started.
 * @param stopPosition  at which the recording stopped.
 */
typedef std::function<void(
    std::int64_t, // recordingId
    std::int64_t, // startPosition
    std::int64_t)> // stopPosition
    on_stop_t;

}

class RecordingEventsAdapter
{
public:
    RecordingEventsAdapter(
            recordingEvents::on_start_t on_start,
            recordingEvents::on_progress_t on_progress,
            recordingEvents::on_stop_t on_stop,
            std::shared_ptr<Subscription> subscription,
            const int fragmentLimit) :
        m_onStart(std::move(on_start)),
        m_onProgress(std::move(on_progress)),
        m_onStop(std::move(on_stop)),
        m_listener(std::move(subscription)),
        m_fragmentLimit(fragmentLimit)
    {
    }

    int poll()
    {
        return m_listener.poll(m_onStart, m_onProgress, m_onStop, m_fragmentLimit);
    }

    const std::shared_ptr<Subscription>& subscription() const {
        return m_listener.subscription();
    }

    std::shared_ptr<Subscription>& subscription() {
        return m_listener.subscription();
    }

private:
    recordingEvents::on_start_t m_onStart;
    recordingEvents::on_progress_t m_onProgress;
    recordingEvents::on_stop_t m_onStop;
    RecordingEventsListener m_listener;
    const std::int32_t m_fragmentLimit;
};

}

#endif // AERON_ARCHIVE_RECORDINGEVENTSADAPTER_H
