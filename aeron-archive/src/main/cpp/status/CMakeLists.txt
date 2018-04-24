add_library(aeron_archive_status INTERFACE)

target_include_directories(aeron_archive_status
   INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aeron_archive_status
   INTERFACE aeron_client
)
