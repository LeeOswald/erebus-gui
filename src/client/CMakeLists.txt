
set(SOURCE_FILES
    application/application.cpp
    application/application.hpp
    application/applicationbase.cpp
    application/applicationbase.hpp
    application/messageclient.cpp
    application/messageclient.hpp
    application/messageserver.cpp
    application/messageserver.hpp
    application/sharedsection.hpp
    connectdlg/connectdlg.cpp
    connectdlg/connectdlg.hpp
    mainwindow/logview.cpp
    mainwindow/logview.hpp
    mainwindow/mainmenu.hpp
    mainwindow/mainwindow.cpp
    mainwindow/mainwindow.hpp
    mainwindow/trayicon.hpp
    plugindlg/plugindlg.cpp
    plugindlg/plugindlg.hpp
    appsettings.hpp
    endpoints.hpp
    main.cpp
    pluginlist.hpp
    pluginmgr.cpp
    pluginmgr.hpp
    settings.hpp
)

set(RESOURCE_FILES
    erebus.qrc
)

set(UIS
    connectdlg/connectdlg.ui
    plugindlg/plugindlg.ui
)

qt6_add_resources(RESOURCE_SOURCES ${RESOURCE_FILES})
qt6_wrap_ui(UI_SOURCES ${UIS})

if(ER_WINDOWS)
    set(WINDOWS_RESOURCE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/erebus.rc")
    set(IMAGE_SUBSYSTEM WIN32)
    set(CMAKE_EXE_LINKER_FLAGS /MANIFEST:NO)
endif()    
    
add_executable(${EREBUS_CLIENT} ${IMAGE_SUBSYSTEM} ${SOURCE_FILES} ${RESOURCE_SOURCES} ${UI_SOURCES} ${WINDOWS_RESOURCE_SCRIPT})

target_compile_features(${EREBUS_CLIENT} PUBLIC ${EREBUS_CXX_FEATURES})

# Qt6Widgets_LIBRARIES variable also includes QtGui and QtCore
target_link_libraries (${EREBUS_CLIENT} PRIVATE ${Qt6Widgets_LIBRARIES} ${Qt6Network_LIBRARIES} ${PLATFORM_LIBRARIES} ${EREBUS_RTLLIB} ${EREBUS_CLTLIB} ${EREBUS_GUILIB})



