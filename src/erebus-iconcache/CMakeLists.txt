
set(SOURCE_FILES
    main.cpp
)

    
add_executable(${EREBUS_ICONCACHE} ${SOURCE_FILES})

# Qt6Widgets_LIBRARIES variable also includes QtGui and QtCore
target_link_libraries (${EREBUS_ICONCACHE} PRIVATE Boost::program_options ${Qt6Widgets_LIBRARIES} ${EREBUS_RTLLIB})



