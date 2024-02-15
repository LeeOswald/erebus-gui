
add_library(${EREBUS_GUILIB} SHARED
    ../../include/erebus-gui/erebus-gui.hpp
    ../../include/erebus-gui/exceptionutil.hpp
    ../../include/erebus-gui/plugin.hpp
    ../../include/erebus-gui/qvariantconverter.hpp
    ../../include/erebus-gui/settings.hpp
    exceptionutil.cpp
)


add_compile_definitions(EREBUSGUI_EXPORTS=1)

target_compile_features(${EREBUS_GUILIB} PUBLIC ${ER_CXX_FEATURES})

target_link_libraries(${EREBUS_GUILIB} PUBLIC ${Qt6Widgets_LIBRARIES} ${EREBUS_RTLLIB})
