#include "plugindlg.hpp"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>


namespace Erc
{

namespace Private
{

namespace Ui
{

PluginDlg::~PluginDlg()
{
    delete m_ui;
}


PluginDlg::PluginDlg(const QStringList& pluginList, const QString& lastPluginDir, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui_PluginDlg)
{
    m_ui->setupUi(this);

    m_ui->btnRemove->setEnabled(false);
    m_ui->buttonOk->setEnabled(false);

    for (auto& plugin: pluginList)
    {
        m_ui->listPlugins->addItem(plugin);
    }

    if (!lastPluginDir.isEmpty())
    {
        m_pluginDir = lastPluginDir;
    }
    else if (!pluginList.empty())
    {
        QFileInfo fi(pluginList.first());
        auto dir = fi.dir();
        m_pluginDir = dir.absolutePath();
    }
}

void PluginDlg::onAdd()
{
    auto fileName = QFileDialog::getOpenFileName(
        this,
        tr("Select plugin file"),
        m_pluginDir,
#if ER_WINDOWS
        tr("Plugins (*.dll)")
#elif ER_LINUX
        tr("Plugins (*.so)")
#endif
    );

    if (fileName.isEmpty())
        return;

    QFileInfo fi(fileName);
    if (!fi.exists())
    {
        return Erc::Ui::errorBoxLite(tr("The file specified does not exist"), this);
    }

    auto dir = fi.dir();
    m_pluginDir = dir.absolutePath();

    m_ui->listPlugins->addItem(fileName);

    m_ui->buttonOk->setEnabled(true);
}

void PluginDlg::onSelectionChanged(int index)
{
    m_ui->btnRemove->setEnabled(index >= 0);
}

void PluginDlg::onRemove()
{
    auto index = m_ui->listPlugins->currentRow();
    auto item = m_ui->listPlugins->takeItem(index);
    if (item)
    {
        delete item;
        if (!m_ui->listPlugins->count())
        {
            m_ui->btnRemove->setEnabled(false);
            m_ui->buttonOk->setEnabled(false);
        }
    }
}

void PluginDlg::onOk()
{
    auto count = m_ui->listPlugins->count();
    if (!count)
    {
        return Erc::Ui::errorBoxLite(tr("At least one plugin is required"), this);
    }

    for (int i = 0; i < count; ++i)
    {
        auto item = m_ui->listPlugins->item(i);
        Q_ASSERT(item);
        auto plugin = item->data(Qt::DisplayRole).toString();

        m_plugins.push_back(std::move(plugin));
    }

    accept();
}

void PluginDlg::onCancel()
{
    reject();
}


} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
