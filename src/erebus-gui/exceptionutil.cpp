#include <erebus/knownprops.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/format.hxx>

#include <sstream>

#include <erebus-gui/exceptionutil.hpp>

namespace Erc
{

namespace
{

constexpr size_t kIndentSize = 4;

void formatException(const Er::Exception& e, std::ostringstream& out, int level);


void formatException(const std::exception& e, std::ostringstream& out, int level)
{
    std::string indent(level * kIndentSize, ' ');

    if (level > 0)
        out << '\n' << indent << "------------------------------------------------\n";

    auto message = e.what();
    if (!message || !*message)
        out << indent << "Unknown exception";
    else
        out << indent << e.what();

    try
    {
        std::rethrow_if_nested(e);
    }
    catch (Er::Exception& nested)
    {
        formatException(nested, out, level + 1);
    }
    catch (std::exception& nested)
    {
        formatException(nested, out, level + 1);
    }
    catch (...)
    {
    }
}

void formatException(const Er::Exception& e, std::ostringstream& out, int level)
{
    std::string indent(level * kIndentSize, ' ');
    std::string smallIndent(level * kIndentSize / 2, ' ');

    if (level > 0)
        out << '\n' << indent << "------------------------------------------------\n";

    auto message = e.message();
    if (message)
    {
        out << indent << *message;
    }
    else
    {
        out << indent << e.what();
    }

    auto location = e.location();
    if (location)
    {
        if (location->source)
        {
            out << "\n" << indent << smallIndent << "File: " << location->source->file();
            out << "\n" << indent << smallIndent << "Line: " << location->source->line();
        }

        Er::DecodedStackTrace ds;
        const Er::DecodedStackTrace* stack = nullptr;
        if (location->decoded)
        {
            stack = &(*location->decoded);
        }
        else if (location->stack)
        {
            ds = Er::decodeStackTrace(*location->stack);
            stack = &ds;
        }

        if (stack && !stack->empty())
        {
            out << "\n" << indent << smallIndent << "Call Stack:";
            for (auto& frame : *stack)
            {
                out << "\n" << indent << smallIndent << " ";
                if (!frame.empty())
                    out << frame;
                else
                    out << "???";
            }
        }
    }

    auto properties = e.properties();
    if (properties)
    {
        for (auto& prop : *properties)
        {
            out << "\n" << indent << smallIndent;

            auto pi = Er::lookupProperty(prop.id);
            if (pi)
            {
                out << pi->name() << ": ";
                pi->format(prop, out);
            }
            else
            {
                out << Er::Util::format("0x%08x: ???", prop.id);
            }
        }
    }

    try
    {
        std::rethrow_if_nested(e);
    }
    catch (Er::Exception& nested)
    {
        formatException(nested, out, level + 1);
    }
    catch (std::exception& nested)
    {
        formatException(nested, out, level + 1);
    }
    catch (...)
    {
    }
}

} // namespace {}

    
EREBUSGUI_EXPORT std::string formatException(const std::exception& e) noexcept
{
    try
    {
        std::ostringstream out;

        formatException(e, out, 0);

        return out.str();
    }
    catch (...)
    {
    }

    return std::string();
}

EREBUSGUI_EXPORT std::string formatException(const Er::Exception& e) noexcept
{
    try
    {
        std::ostringstream out;

        formatException(e, out, 0);

        return out.str();
    }
    catch (...)
    {
    }

    return std::string();
}
    
} // namespace Erc {}