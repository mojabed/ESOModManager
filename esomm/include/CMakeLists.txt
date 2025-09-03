#set(ESOMM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(INCLUDE_SOURCES
    
    logger.h
    pathing.h
    http_client.h
    esomm.h
    esomm_style.h
    ModType.h
    manager.h
)

target_sources(esomm
    PUBLIC
        ${INCLUDE_SOURCES}
)