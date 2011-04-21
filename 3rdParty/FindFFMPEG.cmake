# Locate ffmpeg
#
# Output
# ------
# FFMPEG_LIBRARIES
# FFMPEG_FOUND
# FFMPEG_INCLUDE_DIR
#
# Input
# -----
# ROOT_3RDPARTY_DIR
#    Location of 3rd party libraries for a project
#    It is searched last, so system libraries will be have priority.
# $FFMPEG_DIR       
#    an environment variable that would
#    correspond to the ./configure --prefix=$FFMPEG_DIR
#
# Created by Robert Osfield.
# Modified by Nathan Clack.


# Notes
# -----
# In ffmpeg code, old version use "#include <header.h>" and newer use "#include <libname/header.h>"
# Use "#include <header.h>" for compatibility with old version of ffmpeg
# We have to search the path which contain the header.h (usefull for old version)
# and search the path which contain the libname/header.h (usefull for new version)
# Then we need to include ${FFMPEG_libname_INCLUDE_DIRS} (in old version case, use by ffmpeg header and osg plugin code)
#                                                       (in new version case, use by ffmpeg header) 
# and ${FFMPEG_libname_INCLUDE_DIRS/libname}             (in new version case, use by osg plugin code)

if(WIN32)
  set(FFMPEG_INCLUDE_PATH_SUFFIXES
            ffmpeg/w32/msvc/include
            ffmpeg/w32/ming/include
            ffmpeg/w32/msvc/include/lib${shortname} 
            ffmpeg/w32/ming/include/lib${shortname} 
  )
  set(FFMPEG_LIB_PATH_SUFFIXES
            ffmpeg/w32/msvc/lib
            ffmpeg/w32/ming/lib
  )
else()
  set(FFMPEG_INCLUDE_PATH_SUFFIXES
            ffmpeg
            ffmpeg/lib${shortname}
  )
  set(FFMPEG_LIB_PATH_SUFFIXES
            lib
            lib64
  )
endif()
# Macro to find header and lib directories
# example: FFMPEG_FIND(AVFORMAT avformat avformat.h)
MACRO(FFMPEG_FIND varname shortname headername)
#message("++ ROOT: ${ROOT_3RDPARTY_DIR}")
#message("++     : ${headername}")
#message("++     : ${shortname}")
    FIND_PATH(FFMPEG_${varname}_INCLUDE_DIRS lib${shortname}/${headername}
        HINTS
          ${ROOT_3RDPARTY_DIR}
        PATHS
          ${FFMPEG_ROOT}/include
          $ENV{FFMPEG_DIR}/include
          ~/Library/Frameworks
          /Library/Frameworks
          /usr/local/include
          /usr/include
          /sw/include # Fink
          /opt/local/include # DarwinPorts
          /opt/csw/include # Blastwave
          /opt/include
          /usr/freeware/include
        PATH_SUFFIXES 
          ${FFMPEG_INCLUDE_PATH_SUFFIXES}
        DOC "Location of FFMPEG Headers"
        NO_DEFAULT_PATH
    )

    FIND_FILE(FFMPEG_${varname}_LIBRARIES
        "lib${shortname}.a"                 # same on windows
        HINTS
          ${ROOT_3RDPARTY_DIR}
        PATHS
          ${FFMPEG_ROOT}
          $ENV{FFMPEG_DIR}
          ~/Library/Frameworks
          /Library/Frameworks
          /usr/local
          /usr
          /sw
          /opt/local
          /opt/csw
          /opt
          /usr/freeware
        PATH_SUFFIXES 
          ${FFMPEG_LIB_PATH_SUFFIXES}
        DOC "Location of FFMPEG Libraries"
    )

#message("++ ${FFMPEG_${varname}_LIBRARIES}")
#message("++ ${FFMPEG_${varname}_INCLUDE_DIRS}")
    IF (FFMPEG_${varname}_LIBRARIES AND FFMPEG_${varname}_INCLUDE_DIRS)
        SET(FFMPEG_${varname}_FOUND 1)
    ENDIF(FFMPEG_${varname}_LIBRARIES AND FFMPEG_${varname}_INCLUDE_DIRS)

ENDMACRO(FFMPEG_FIND)

SET(FFMPEG_ROOT "$ENV{FFMPEG_DIR}" CACHE PATH "Location of FFMPEG")

FFMPEG_FIND(LIBAVFORMAT avformat avformat.h)
FFMPEG_FIND(LIBAVDEVICE avdevice avdevice.h)
FFMPEG_FIND(LIBAVCODEC  avcodec  avcodec.h)
FFMPEG_FIND(LIBAVUTIL   avutil   avutil.h)
FFMPEG_FIND(LIBSWSCALE  swscale  swscale.h)

SET(FFMPEG_FOUND "NO")
IF   (FFMPEG_LIBAVFORMAT_FOUND AND FFMPEG_LIBAVDEVICE_FOUND AND FFMPEG_LIBAVCODEC_FOUND AND FFMPEG_LIBAVUTIL_FOUND AND FFMPEG_LIBSWSCALE_FOUND)

    SET(FFMPEG_FOUND "YES")

    SET(FFMPEG_INCLUDE_DIRS ${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS})

    SET(FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBAVFORMAT_LIBRARY_DIRS})

    if(WIN32) 
      get_filename_component(
          FFMPEG_KITCHEN_SINK_PATH
          ${FFMPEG_LIBAVCODEC_LIBRARIES}
          PATH)
      FILE(GLOB FFMPEG_LIBRARIES ${FFMPEG_KITCHEN_SINK_PATH}/*.a)      
#message("FFMPEG_KITCHEN_SINK_PATH is ${FFMPEG_KITCHEN_SINK_PATH}")        
#message("FFMPEG_LIBRARIES is ${FFMPEG_LIBRARIES}")
    else()
      find_package(ZLIB)
      find_package(BZip2)
      find_package(Vorbis)
      find_package(Lame)
      find_package(x264)
      find_package(xvid)
      find_package(theora)
      find_package(va)
      find_package(VPX)
      find_package(Schroedinger)
      find_package(Speex)
      find_package(GSM)

      set(CMAKE_THREAD_PREFER_PTHREAD)
      find_package(Threads)
      set(THREAD_LIBRARY ${CMAKE_THREAD_LIBS_INIT})

macro(ADDLIB VAR NAME)
  if(${NAME}_FOUND)
    set(${VAR} ${${VAR}} ${${NAME}_LIBRARY})
  endif()
endmacro()
macro(ADDLIBS VAR NAME)
  if(${NAME}_FOUND)
    set(${VAR} ${${VAR}} ${${NAME}_LIBRARIES})
  endif()
endmacro()

      SET(FFMPEG_LIBRARIES
          ${FFMPEG_LIBAVFORMAT_LIBRARIES}
          ${FFMPEG_LIBAVDEVICE_LIBRARIES}
          ${FFMPEG_LIBAVCODEC_LIBRARIES}
          ${FFMPEG_LIBAVUTIL_LIBRARIES}
          ${FFMPEG_LIBSWSCALE_LIBRARIES}
          ${ZLIB_LIBRARY}
          ${BZIP2_LIBRARIES}
          ${OGG_LIBRARY}
          ${VORBIS_LIBRARIES}
          ${Lame_LIBRARY}
          ${X264_LIBRARY}
          ${XVID_LIBRARY}
          ${THREAD_LIBRARY}
          )
      ADDLIBS(FFMPEG_LIBRARIES THEORA)
      ADDLIB (FFMPEG_LIBRARIES VAAPI)
      ADDLIB (FFMPEG_LIBRARIES VPX)
      ADDLIB (FFMPEG_LIBRARIES SCHROEDINGER)
      ADDLIB (FFMPEG_LIBRARIES SPEEX)
      ADDLIB (FFMPEG_LIBRARIES GSM)
    endif()
#message("FFMPEG_LIBRARIES are ${FFMPEG_LIBRARIES}")        

ELSE ()

    MESSAGE(STATUS "Could not find FFMPEG")

ENDIF()
