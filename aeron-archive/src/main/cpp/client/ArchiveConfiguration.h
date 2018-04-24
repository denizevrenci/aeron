#ifndef AERON_ARCHIVE_ARCHIVECONFIGURATION_H
#define AERON_ARCHIVE_ARCHIVECONFIGURATION_H

#include <chrono>

#include <util/MacroUtil.h>

namespace aeron {
namespace archive {
namespace configuration {

namespace details {

constexpr std::uint8_t CLIENT_MAJOR_VERSION = 0;
constexpr std::uint8_t CLIENT_MINOR_VERSION = 2;
constexpr std::uint8_t CLIENT_PATCH_VERSION = 1;

}

constexpr std::int32_t CLIENT_SEMANTIC_VERSION =
    aeron::util::semanticVersionCompose(
        details::CLIENT_MAJOR_VERSION,
        details::CLIENT_MINOR_VERSION,
        details::CLIENT_PATCH_VERSION);

namespace defaults {

/**
 * Timeout when waiting on a message to be sent or received.
 */
constexpr std::chrono::seconds MESSAGE_TIMEOUT{5};

/**
 * Channel for sending control messages to an archive.
 */
constexpr char CONTROL_REQUEST_CHANNEL[] = "aeron:udp?endpoint=localhost:8010";

/**
 * Stream id within a channel for sending control messages to an archive.
 */
constexpr std::int32_t CONTROL_REQUEST_STREAM_ID = 10;

/**
 * Channel for receiving control response messages from an archive.
 */
constexpr char CONTROL_RESPONSE_CHANNEL[] = "aeron:udp?endpoint=localhost:8020";

/**
 * Stream id within a channel for receiving control messages from an archive.
 */
constexpr std::int32_t CONTROL_RESPONSE_STREAM_ID = 20;

/**
 * Channel for receiving progress events of recordings from an archive.
 * For production it is recommended that multicast or dynamic multi-destination-cast (MDC) is used to allow
 * for dynamic subscribers, an endpoint can be added to the subscription side for controlling port usage.
 */
constexpr char RECORDING_EVENTS_CHANNEL[] = "aeron:udp?control-mode=dynamic|control=localhost:8030";

/**
 * Stream id within a channel for receiving progress of recordings from an archive.
 */
constexpr std::int32_t RECORDING_EVENTS_STREAM_ID = 30;

/**
 * Controls whether term buffer files are sparse on the control channel.
 */
constexpr bool CONTROL_TERM_BUFFER_SPARSE = true;

/**
 * Low term length for control channel reflects expected low bandwidth usage.
 */
constexpr std::int32_t CONTROL_TERM_BUFFER_LENGTH = 64 * 1024;

/**
 * MTU to reflect default for the control streams.
 */
constexpr std::int32_t CONTROL_MTU_LENGTH = 1408;

}}}}

#endif // AERON_ARCHIVE_ARCHIVECONFIGURATION_H
