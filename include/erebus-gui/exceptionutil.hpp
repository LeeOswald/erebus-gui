#pragma once

#include <erebus/exception.hxx>

#include <erebus-gui/erebus-gui.hpp>

namespace Erc
{
    
EREBUSGUI_EXPORT std::string formatException(const std::exception& e) noexcept;
EREBUSGUI_EXPORT std::string formatException(const Er::Exception& e) noexcept;   
    
} // namespace Erc {}