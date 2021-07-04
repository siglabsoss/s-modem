# --- 0MQ ZMQ ---   https://www.mail-archive.com/zeromq-dev@lists.zeromq.org/msg17331.html http://pastebin.com/fLsm9SYf http://pastebin.com/sSJJVw6u


set( ZEROMQ_FIND_REQUIRED TRUE )
 
### ZeroMQ ###
 
find_path( ZEROMQ_INCLUDE_DIR NAMES zmq.h PATHS /usr/include/ /usr/local/include/ )
find_library( ZEROMQ_LIBRARY NAMES zmq PATHS /usr/lib /usr/local/lib )
 
if( ZEROMQ_INCLUDE_DIR AND ZEROMQ_LIBRARY )
    set( ZEROMQ_FOUND TRUE )
endif( ZEROMQ_INCLUDE_DIR AND ZEROMQ_LIBRARY )
 
 
if( ZEROMQ_FOUND )
    
    message( STATUS "Found ZeroMQ:" )
    message( STATUS "  (Headers)       ${ZEROMQ_INCLUDE_DIR}" )
    message( STATUS "  (Library)       ${ZEROMQ_LIBRARY}" )
   
else( ZEROMQ_FOUND )
    if( ZEROMQ_FIND_REQUIRED )
        message( FATAL_ERROR "Could not find ZeroMQ" )
    endif( ZEROMQ_FIND_REQUIRED )
endif( ZEROMQ_FOUND )


# --- End 0MQ ---
