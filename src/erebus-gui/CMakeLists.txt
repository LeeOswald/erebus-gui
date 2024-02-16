
set(RESOURCE_FILES
    erebus.qrc
)

set(UIS
    errorbox.ui
)

qt6_add_resources(RESOURCE_SOURCES ${RESOURCE_FILES})
qt6_wrap_ui(UI_SOURCES ${UIS})

add_library(${EREBUS_GUILIB} SHARED
    ../../include/erebus-gui/erebus-gui.hpp
    ../../include/erebus-gui/exceptionutil.hpp
    ../../include/erebus-gui/plugin.hpp
    ../../include/erebus-gui/qvariantconverter.hpp
    ../../include/erebus-gui/settings.hpp
    errorbox.hpp
    errorbox.cpp
    exceptionutil.cpp
    ${RESOURCE_SOURCES} 
    ${UI_SOURCES}
)


add_compile_definitions(EREBUSGUI_EXPORTS=1)

target_compile_features(${EREBUS_GUILIB} PUBLIC ${ER_CXX_FEATURES})

target_link_libraries(${EREBUS_GUILIB} PUBLIC ${Qt6Widgets_LIBRARIES} ${EREBUS_RTLLIB})
