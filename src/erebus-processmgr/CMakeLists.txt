set(PROCESSMGR_PLUGIN erebus-processmgr-ui)

set(RESOURCE_FILES
    processmgr.qrc
)

set(UIS
    columnsdlg.ui
    processdlg.ui
)

qt6_add_resources(RESOURCE_SOURCES ${RESOURCE_FILES})
qt6_wrap_ui(UI_SOURCES ${UIS})

add_library(${PROCESSMGR_PLUGIN} SHARED
    columnsdlg.cpp
    columnsdlg.hpp
    iconcache.cpp
    iconcache.hpp
    itemmenu.cpp
    itemmenu.hpp
    posixresult.hpp
    processcolumns.cpp
    processcolumns.hpp
    processdlg.cpp
    processdlg.hpp
    processinfo.cpp
    processinfo.hpp
    processlist.cpp
    processlist.hpp
    processmgr.cpp
    processmgr.hpp
    processstub.cpp
    processstub.hpp
    processtab.cpp
    processtab.hpp
    proclistworker.cpp
    proclistworker.hpp
    proctreemodel.cpp
    proctreemodel.hpp
    settings.hpp
    ${RESOURCE_SOURCES} 
    ${UI_SOURCES}
)


add_compile_definitions(EREBUSPROCMGR_EXPORTS=1)

set_property(TARGET ${PROCESSMGR_PLUGIN} PROPERTY PREFIX "")

target_link_libraries(${PROCESSMGR_PLUGIN} PUBLIC ${Qt6Widgets_LIBRARIES} ${EREBUS_RTLLIB} ${EREBUS_CLTLIB} ${EREBUS_GUILIB})
