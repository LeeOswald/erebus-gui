#include "../appsettings.hpp"
#include "logview.hpp"

#include <erebus/util/utf16.hxx>

#include <QCoreApplication>

namespace Erp
{

namespace Client
{

namespace Ui
{

LogView::~LogView()
{
    m_log->flush();
    m_log->removeSink("view");
}

LogView::LogView(
        Er::Log::ILog* log,
        Erc::ISettingsStorage* settings,
        QMainWindow* mainWindow,
        QWidget* parent,
        QAction* actionLog
    )
    : QObject(mainWindow)
    , m_log(log)
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

    auto formatter = Er::Log::SimpleFormatter::make({ 
        Er::Log::SimpleFormatter::Option::Time, 
        Er::Log::SimpleFormatter::Option::Level, 
        Er::Log::SimpleFormatter::Option::Tid,
        Er::Log::SimpleFormatter::Option::NoNewLine
    });
    auto sink = std::make_shared<LogSink>(this, formatter);
    m_log->addSink("view", sink);
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
    if (action == m_actionDebug) m_log->setLevel(Er::Log::Level::Debug);
    else if (action == m_actionInfo) m_log->setLevel(Er::Log::Level::Info);
    else if (action == m_actionWarning) m_log->setLevel(Er::Log::Level::Warning);
    else if (action == m_actionError) m_log->setLevel(Er::Log::Level::Error);
    else if (action == m_actionFatal) m_log->setLevel(Er::Log::Level::Fatal);
    else if (action == m_actionOff) m_log->setLevel(Er::Log::Level::Off);

    action->setChecked(true);

    Erc::Option<int>::set(m_settings, Erp::Client::AppSettings::Log::level, int(m_log->level()));

    m_view->setVisible(m_log->level() < Er::Log::Level::Off);

    emit logLevelChanged(prevLevel, m_log->level());
}

void LogView::LogSink::write(Er::Log::Record::Ptr r)
{
    auto formatted = formatter->format(r.get());

    auto utf16Message = Erc::fromUtf8(formatted);
    QMetaObject::invokeMethod(owner, "log", Qt::AutoConnection, Q_ARG(QString, utf16Message));
}

void LogView::LogSink::flush()
{
}

} // namespace Ui {}

} // namespace Client {}

} // namespace Erp {}
