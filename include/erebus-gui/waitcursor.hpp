#pragma once

#include <erebus-gui/erebus-gui.hpp>

#include <QApplication>


namespace Erc
{

namespace Ui
{

class WaitCursorScope final
{
public:
    WaitCursorScope() noexcept
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QApplication::processEvents();
    }

    ~WaitCursorScope()
    {
        QApplication::restoreOverrideCursor();
        QApplication::processEvents();
    }
};


} // namespace Ui {}

} // namespace Erc {}
