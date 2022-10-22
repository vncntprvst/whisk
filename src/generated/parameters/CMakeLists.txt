cmake_minimum_required(VERSION 3.15)
project(WhiskerTrackingParameterParser)

add_executable(schema
    ${CMAKE_CURRENT_BINARY_DIR}/schema.tab.c
    ${CMAKE_CURRENT_BINARY_DIR}/schema.tab.h
)
set(PARAM_PARSER_SRC ${CMAKE_CURRENT_BINARY_DIR}/param.h)

set(PARAM_SRCS ${CMAKE_CURRENT_BINARY_DIR}/param.tab.c)
set(PARAM_HDRS ${PARAM_PARSER_SRC} ${CMAKE_CURRENT_BINARY_DIR}/param.tab.h)

set(PARAM_MODULE
    ${PARAM_SRCS}
    ${PARAM_HDRS}
    PARENT_SCOPE
)
source_group("Source Files\\Parameters" FILES ${PARAM_MODULE})

add_custom_target(ParameterParser DEPENDS ${PARAM_SRCS} ${PARAM_HDRS} ${PARAM_PARSER_SRC})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(validate
    validate.c
    ${PARAM_HDRS}
    ${PARAM_SRCS}
)
add_dependencies(validate
    schema
    ParameterParser
)

#set(PARAM_SRCS ${CMAKE_CURRENT_BINARY_DIR}/param.tab.c)
#set(PARAM_HDRS ${CMAKE_CURRENT_BINARY_DIR}/param.h ${CMAKE_CURRENT_BINARY_DIR}/param.tab.h)
#set(PARAM_MODULE
#    ${PARAM_SRCS}
#    ${PARAM_HDRS}
#    PARENT_SCOPE
#)
#source_group("Source Files\\Parameters" FILES ${PARAM_MODULE})

#add_custom_target(ParameterParser DEPENDS ${PARAM_SRCS} ${PARAM_HDRS})
