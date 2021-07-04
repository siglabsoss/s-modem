
# Looks in our s-modem build folder for the .so
find_library( HIGGS_DRIVER_LIBRARY NAMES libHiggsDriver.so PATHS ${PROJECT_SOURCE_DIR}/../build )
 
if( HIGGS_DRIVER_LIBRARY )
    set( HIGGS_FOUND TRUE )
endif( HIGGS_DRIVER_LIBRARY )
 
 
if( HIGGS_FOUND )
    message( STATUS "Found Higgs:" )
    message( STATUS "  (Library)       ${HIGGS_DRIVER_LIBRARY}" )
   
else( HIGGS_FOUND )
        message( FATAL_ERROR "Could not find Higgs" )
endif( HIGGS_FOUND )
