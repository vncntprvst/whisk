## Author: Nathan Clack <clackn@janelia.hhmi.org>
## Date  : 2010
##
## Copyright 2010 Howard Hughes Medical Institute.
## All rights reserved.
## Use is subject to Janelia Farm Research Campus Software Copyright 1.1
## license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
##

#
# Parameter file parser
#
cmake_minimum_required(VERSION 3.15)
project(WhiskerTrackingParameterParser)

BISON_TARGET(
  SCHEMA
  schema.y
  "${CMAKE_CURRENT_BINARY_DIR}/schema.tab.c"
  VERBOSE REPORT_FILE
    schema.grammer
  COMPILE_FLAGS "-t")
#message("BISON_SCHEMA_OUTPUTS is ${BISON_SCHEMA_OUTPUTS}")
add_executable(schema ${BISON_SCHEMA_OUTPUT_SOURCE})

set(PARAM_PARSER_SRC
    ${CMAKE_CURRENT_BINARY_DIR}/param.y
    ${CMAKE_CURRENT_BINARY_DIR}/param.h
    )
set_source_files_properties(${PARAM_PARSER_SRC} PROPERTIES GENERATED 1)
add_custom_command(
  OUTPUT ${PARAM_PARSER_SRC}
  COMMAND
    #${CMAKE_CURRENT_BINARY_DIR}/schema ${CMAKE_CURRENT_SOURCE_DIR}/parameters.schema param.y param.h
    schema ${CMAKE_CURRENT_SOURCE_DIR}/parameters.schema param.y param.h
  DEPENDS
    schema
    parameters.schema
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}
  )
BISON_TARGET(
  PARAM
  ${CMAKE_CURRENT_BINARY_DIR}/param.y
  ${CMAKE_CURRENT_BINARY_DIR}/param.tab.c
  VERBOSE REPORT_FILE
    param.grammer
  COMPILE_FLAGS "-t")
#message("BISON_PARAM_OUTPUTS is ${BISON_PARAM_OUTPUTS}")
set(PARAM_SRCS ${BISON_PARAM_OUTPUT_SOURCE})
#message(":: PARAM_SRCS is ${PARAM_SRCS}")
set(PARAM_HDRS ${PARAM_PARSER_SRC} ${BISON_PARAM_OUTPUT_HEADER})
#message(":: PARAM_HDRS is ${PARAM_HDRS}")
set(PARAM_MODULE
    ${PARAM_SRCS}
    ${PARAM_HDRS}
    PARENT_SCOPE
    # Comment PARENT_SCOPE to make PARAM_MODULE not empty
)

# message(":: PARAM_MODULE is ${PARAM_MODULE}")
# message(":: PARAM_SRCS is ${PARAM_SRCS}")
# message(":: PARAM_HDRS is ${PARAM_HDRS}")

source_group("Source Files\\Parameters" FILES ${PARAM_MODULE})

add_custom_target(ParameterParser DEPENDS ${PARAM_SRCS} ${PARAM_HDRS} ${PARAM_PARSER_SRC})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(validate
    validate.c
    ${PARAM_PARSER_SRC}
)
add_dependencies(validate
    schema
    ParameterParser
)
