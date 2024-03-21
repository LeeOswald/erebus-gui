#include "../appsettings.hpp"
#include "logview.hpp"

#include <erebus/util/utf16.hxx>

#include <QCoreApplication>

namespace Erc
{

namespace Private
{

namespace Ui
{

LogView::~LogView()
{
    m_log->flush();
    m_logCtl->removeDelegate("view");
}

LogView::LogView(
        Er::Log::ILog* log,
        Er::Log::ILogControl* logCtl,
        Erc::ISettingsStorage* settings,
        QMainWindow* mainWindow,
        QWidget* parent,
        QAction* actionLog
    )
    : QObject(mainWindow)
    , m_log(log)
    , m_logCtl(logCtl)
    , m_settings(settings)
    , m_view(new QPlainTextEdit(parent))
    , m_menu(new QMenu(mainWindow))
    , m_actionGroup(new QActionGroup(mainWindow))
    , m_actionDebug(new QAction(QCoreApplication::translate("LogMenu", "Debug", nullptr), m_actionGroup))
    , m_actionInfo(new QAction(QCoreApplication::translate("LogMenu", "Info", nullptr), m_actionGroup))
    , m_actionWarning(new QAction(QCoreApplication::translate("LogMenu", "Warning", nullptr), m_actionGroup))
    , m_actionError(new QAction(QCoreApplication::translate("LogMenu", "Error", nullptr), m_actionGroup))
    , m_actionFatal(new QAction(QCoreApplication::translate("LogMenu", "Fatal", nullptr), m_actionGroup))
    , m_actionOff(new QAction(QCoreApplication::translate("LogMenu", "Off", nullptr), m_actionGroup))
    , m_actionClear(new QAction(QCoreApplication::translate("LogMenu", "Clear Log", nullptr), m_menu))
{
    QFont font;
    font.setFamilies({QString::fromUtf8("Courier New")});
    font.setPointSize(10);
    m_view->setFont(font);
    m_view->setUndoRedoEnabled(false);
    m_view->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_view->setReadOnly(true);
    m_view->setPlainText(QString::fromUtf8(""));
    m_view->setPlaceholderText(QString::fromUtf8(""));

    m_actionGroup->setExclusive(true);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(setLogLevel(QAction*)));

    actionLog->setMenu(m_menu);

    auto level = m_log->level();
    m_actionDebug->setCheckable(true);
    m_menu->addAction(m_actionDebug);
    m_actionDebug->setChecked(level == Er::Log::Level::Debug);

    m_actionInfo->setCheckable(true);
    m_menu->addAction(m_actionInfo);
    m_actionInfo->setChecked(level == Er::Log::Level::Info);

    m_actionWarning->setCheckable(true);
    m_menu->addAction(m_actionWarning);
    m_actionWarning->setChecked(level == Er::Log::Level::Warning);

    m_actionError->setCheckable(true);
    m_menu->addAction(m_actionError);
    m_actionError->setChecked(level == Er::Log::Level::Error);

    m_actionFatal->setCheckable(true);
    m_menu->addAction(m_actionFatal);
    m_actionFatal->setChecked(level == Er::Log::Level::Fatal);

    m_actionOff->setCheckable(true);
    m_menu->addAction(m_actionOff);
    m_actionOff->setChecked(level == Er::Log::Level::Off);

    m_menu->addSeparator();

    connect(m_actionClear, SIGNAL(triggered()), this, SLOT(clearLog()));
    m_menu->addAction(m_actionClear);

    m_logCtl->addDelegate("view", [this](std::shared_ptr<Er::Log::Record> r) { logDelegate(r); });
    m_logCtl->unmute();
}

void LogView::clearLog()
{
    m_view->clear();
}

void LogView::log(QString text)
{
    m_view->appendPlainText(text);
    m_view->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    m_view->moveCursor(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
}

void LogView::setLogLevel(QAction* action)
{
    auto prevLevel = m_log->level();
    if (action == m_actionDebug) m_logCtl->setLevel(Er::Log::Level::Debug);
    else if (action == m_actionInfo) m_logCtl->setLevel(Er::Log::Level::Info);
    else if (action == m_actionWarning) m_logCtl->setLevel(Er::Log::Level::Warning);
    else if (action == m_actionError) m_logCtl->setLevel(Er::Log::Level::Error);
    else if (action == m_actionFatal) m_logCtl->setLevel(Er::Log::Level::Fatal);
    else if (action == m_actionOff) m_logCtl->setLevel(Er::Log::Level::Off);

    action->setChecked(true);

    Erc::Option<int>::set(m_settings, Erc::Private::AppSettings::Log::level, int(m_log->level()));

    m_view->setVisible(m_log->level() < Er::Log::Level::Off);

    emit logLevelChanged(prevLevel, m_log->level());
}

void LogView::logDelegate(std::shared_ptr<Er::Log::Record> r)
{
    const char* strLevel = "?";
    switch (r->level)
    {
    case Er::Log::Level::Debug: strLevel = "D"; break;
    case Er::Log::Level::Info: strLevel = "I"; break;
    case Er::Log::Level::Warning: strLevel = "W"; break;
    case Er::Log::Level::Error: strLevel = "E"; break;
    case Er::Log::Level::Fatal: strLevel = "!"; break;
    }

    char prefix[256];
    ::snprintf(prefix,
        _countof(prefix),
        "[%02d:%02d:%02d.%03d @%zu:%zu %s] ",
        r->time.hour,
        r->time.minute,
        r->time.second,
        r->time.milli,
        r->pid,
        r->tid,
        strLevel
    );

    std::string message = std::string(prefix);
    if (r->location.component)
    {
        message.append("[");
        message.append(r->location.component);
        if (r->location.instance)
        {
            char tmp[64];
            ::snprintf(tmp, _countof(tmp), " %p", r->location.instance);
            message.append(tmp);
        }

        message.append("] ");
    }

    message.append(r->message);

    auto utf16Message = Erc::fromUtf8(message);
    QMetaObject::invokeMethod(this, "log", Qt::AutoConnection, Q_ARG(QString, utf16Message));

#if ER_WINDOWS && ER_DEBUG
    utf16Message.append(L"\n");
    ::OutputDebugStringW(reinterpret_cast<const wchar_t*>(utf16Message.utf16()));
#endif
}

} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
