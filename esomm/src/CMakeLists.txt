#set(ESOMM_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(SRC
    logger.cpp
    pathing.cpp
    http_client.cpp
    esomm.cpp
    esomm_style.cpp
    manager.cpp
)

target_sources(esomm
    PRIVATE
        ${SRC}
)