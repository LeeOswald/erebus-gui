
set(SOURCE_FILES
    main.cpp
    settings.hpp
)

set(RESOURCE_FILES
    erebus.qrc
)


qt6_add_resources(RESOURCE_SOURCES ${RESOURCE_FILES})
#qt6_wrap_ui(UI_SOURCES ${UIS})

if(ER_WINDOWS)
    set(WINDOWS_RESOURCE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/erebus.rc")
    set(IMAGE_SUBSYSTEM WIN32)
    set(CMAKE_EXE_LINKER_FLAGS /MANIFEST:NO)
endif()    
    
add_executable(${EREBUS_CLIENT} ${IMAGE_SUBSYSTEM} ${SOURCE_FILES} ${RESOURCE_SOURCES} ${UI_SOURCES} ${WINDOWS_RESOURCE_SCRIPT})

target_compile_features(${EREBUS_CLIENT} PUBLIC ${EREBUS_CXX_FEATURES})

# Qt6Widgets_LIBRARIES variable also includes QtGui and QtCore
target_link_libraries (${EREBUS_CLIENT} PRIVATE ${Qt6Widgets_LIBRARIES} ${PLATFORM_LIBRARIES} ${EREBUS_RTLLIB} ${EREBUS_CLTLIB})



