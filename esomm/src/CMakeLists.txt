#set(ESOMM_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(SRC
    esomm.cpp
    logger.cpp
    pathing.cpp
)

target_sources(esomm
    PRIVATE
        ${SRC}
)