SET(SOURCE
    AeronArchive.cpp
    ArchiveProxy.cpp
    ControlResponsePoller.cpp
    RecordingDescriptorPoller.cpp
    RecordingSubscriptionDescriptorPoller.cpp
    ReplayMerge.cpp)

SET(HEADERS
    AeronArchive.h
    ArchiveConfiguration.h
    ArchiveContext.h
    ArchiveException.h
    ArchiveProxy.h
    ControlResponsePoller.h
    ErrorHandler.h
    RecordingDescriptorConsumer.h
    RecordingDescriptorPoller.h
    RecordingEventsAdapter.h
    RecordingEventsListener.h
    RecordingSubscriptionDescriptorConsumer.h
    RecordingSubscriptionDescriptorPoller.h
    ReplayMerge.h)

# static library
add_library(aeron_archive_client STATIC ${SOURCE} ${HEADERS})

find_package(Java REQUIRED)

set(CODEC_SCHEMA ${ARCHIVE_CODEC_SCHEMA_DIR}/aeron-archive-codecs.xml)
set(GENERATED_CODECS ${ARCHIVE_CODEC_TARGET_DIR})

add_custom_command(
    OUTPUT ${GENERATED_CODECS}
    COMMAND ./gradlew -Dcodec.target.dir=${ARCHIVE_CODEC_TARGET_DIR} :aeron-archive:generateCppCodecs
    DEPENDS ${CODEC_SCHEMA}
    WORKING_DIRECTORY ${ARCHIVE_CODEC_WORKING_DIR}
    COMMENT "Generating C++ Archive codecs"
)

add_custom_target(archive-codecs DEPENDS ${GENERATED_CODECS})

target_include_directories(aeron_archive_client
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${ARCHIVE_CODEC_TARGET_DIR}
    PUBLIC "${PROJECT_SOURCE_DIR}/cppbuild/sbe/include"
)

target_compile_definitions(aeron_archive_client
    PUBLIC $<$<NOT:$<CONFIG:Debug>>:SBE_NO_BOUNDS_CHECK>
)

target_link_libraries(aeron_archive_client
    PUBLIC aeron_client
)

add_dependencies(aeron_archive_client archive-codecs)

if (AERON_INSTALL_TARGETS)
    install(TARGETS aeron_archive_client ARCHIVE DESTINATION lib)
    install(DIRECTORY . DESTINATION  include FILES_MATCHING PATTERN "*.h")
endif()

# static PIC library
add_library(aeron_archive_client_pic STATIC ${SOURCE} ${HEADERS})

target_include_directories(aeron_archive_client_pic
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${ARCHIVE_CODEC_TARGET_DIR}
    PUBLIC "${PROJECT_SOURCE_DIR}/cppbuild/sbe/include"
)

target_compile_definitions(aeron_archive_client_pic
    PUBLIC $<$<NOT:$<CONFIG:Debug>>:SBE_NO_BOUNDS_CHECK>
)

set_target_properties(aeron_archive_client_pic
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(aeron_archive_client_pic
    PUBLIC aeron_client_pic
)

add_dependencies(aeron_archive_client_pic archive-codecs)

if (AERON_INSTALL_TARGETS)
    install(TARGETS aeron_archive_client_pic ARCHIVE DESTINATION lib)
endif()
