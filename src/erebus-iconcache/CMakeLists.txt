
set(SOURCE_FILES
    main.cpp
)

    
add_executable(${EREBUS_ICONCACHE} ${SOURCE_FILES})

target_compile_features(${EREBUS_ICONCACHE} PUBLIC ${EREBUS_CXX_FEATURES})

# Qt6Widgets_LIBRARIES variable also includes QtGui and QtCore
target_link_libraries (${EREBUS_ICONCACHE} PRIVATE ${Qt6Widgets_LIBRARIES} ${EREBUS_RTLLIB})



