#set(ESOMM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(INCLUDE_SOURCES
    esomm.h
    logger.h
    pathing.h
)

target_sources(esomm
    PUBLIC
        ${INCLUDE_SOURCES}
)