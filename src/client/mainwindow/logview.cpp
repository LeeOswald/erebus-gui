#include "../appsettings.hpp"
#include "logview.hpp"

#include <erebus/util/utf16.hxx>

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
        Erc::ISettingsStorage* settings,
        QMainWindow* mainWindow,
        QWidget* parent
    )
    : QObject(mainWindow)
    , m_log(log)
    , m_logCtl(dynamic_cast<Er::Log::ILogControl*>(log))
    , m_settings(settings)
    , m_view(new QPlainTextEdit(parent))
    , m_menu(new QMenu())
    , m_actionGroup(new QActionGroup(mainWindow))
    , m_actionLog(new QAction(mainWindow))
    , m_actionDebug(new QAction(m_actionGroup))
    , m_actionInfo(new QAction(m_actionGroup))
    , m_actionWarning(new QAction(m_actionGroup))
    , m_actionError(new QAction(m_actionGroup))
    , m_actionFatal(new QAction(m_actionGroup))
    , m_actionOff(new QAction(m_actionGroup))
    , m_actionClear(new QAction(m_actionGroup))
{
    assert(m_logCtl);

    m_view->setObjectName("logView");
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

    m_actionLog->setObjectName("actionLog");
    m_actionDebug->setObjectName("actionDebug");
    m_actionInfo->setObjectName("actionInfo");
    m_actionWarning->setObjectName("actionWarning");
    m_actionError->setObjectName("actionError");
    m_actionFatal->setObjectName("actionFatal");
    m_actionOff->setObjectName("actionOff");
    m_actionClear->setObjectName("actionClear");

    m_actionLog->setMenu(m_menu);

    auto level = m_log->level();
    m_actionLog->setCheckable(true);
    m_menu->addAction(m_actionLog);
    m_actionLog->setChecked(level == Er::Log::Level::Debug);

    m_actionInfo = new QAction(tr("Info"), m_actionGroup);
    m_actionInfo->setCheckable(true);
    m_menu->addAction(m_actionInfo);
    m_actionInfo->setChecked(level == Er::Log::Level::Info);

    m_actionWarning = new QAction(tr("Warning"), m_actionGroup);
    m_actionWarning->setCheckable(true);
    m_menu->addAction(m_actionWarning);
    m_actionWarning->setChecked(level == Er::Log::Level::Warning);

    m_actionError = new QAction(tr("Error"), m_actionGroup);
    m_actionError->setCheckable(true);
    m_menu->addAction(m_actionError);
    m_actionError->setChecked(level == Er::Log::Level::Error);

    m_actionFatal = new QAction(tr("Fatal"), m_actionGroup);
    m_actionFatal->setCheckable(true);
    m_menu->addAction(m_actionFatal);
    m_actionFatal->setChecked(level == Er::Log::Level::Fatal);

    m_actionOff = new QAction(tr("Off"), m_actionGroup);
    m_actionOff->setCheckable(true);
    m_menu->addAction(m_actionOff);
    m_actionOff->setChecked(level == Er::Log::Level::Off);

    m_menu->addSeparator();

    m_actionClear = new QAction(tr("Clear Log"), m_menu);
    connect(m_actionClear, SIGNAL(triggered()), this, SLOT(clearLog()));
    m_menu->addAction(m_actionClear);

    retranslateUi();

    m_logCtl->addDelegate("view", [this](std::shared_ptr<Er::Log::Record> r) { logDelegate(r); });
    m_logCtl->unmute();
}

QWidget* LogView::widget() const noexcept
{
    return m_view;
}

QAction* LogView::action() const noexcept
{
    return m_actionLog;
}

void LogView::retranslateUi()
{
    m_actionDebug->setText(QCoreApplication::translate("MainWindow", "Debug", nullptr));
    m_actionInfo->setText(QCoreApplication::translate("MainWindow", "Info", nullptr));
    m_actionWarning->setText(QCoreApplication::translate("MainWindow", "Warning", nullptr));
    m_actionError->setText(QCoreApplication::translate("MainWindow", "Error", nullptr));
    m_actionFatal->setText(QCoreApplication::translate("MainWindow", "Fatal", nullptr));
    m_actionOff->setText(QCoreApplication::translate("MainWindow", "Off", nullptr));
    m_actionClear->setText(QCoreApplication::translate("MainWindow", "Clear", nullptr));
    m_actionLog->setText(QCoreApplication::translate("MainWindow", "Log", nullptr));
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
    message.append(r->message);
    message.append("\n");

    auto utf16Message = Erc::fromUtf8(message);

#if ER_WINDOWS && ER_DEBUG
    ::OutputDebugStringW(reinterpret_cast<const wchar_t*>(utf16Message.utf16()));
#endif

    QMetaObject::invokeMethod(this, "log", Qt::AutoConnection, Q_ARG(QString, utf16Message));
}

} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
