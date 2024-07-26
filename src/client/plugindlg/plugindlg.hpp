#pragma once

#include <erebus-gui/erebus-gui.hpp>


#include "ui_plugindlg.h"
#include "../pluginlist.hpp"

#include <QDialog>


namespace Erp
{

namespace Client
{

namespace Ui
{


class PluginDlg final
    : public QDialog
{
    Q_OBJECT

public:
    ~PluginDlg();

    explicit PluginDlg(const QStringList& pluginList, const QString& lastPluginDir, QWidget* parent = nullptr);

    const QString& pluginDir() const noexcept
    {
        return m_pluginDir;
    }

    const QStringList& plugins() const noexcept
    {
        return m_plugins;
    }

public slots:
    void onOk();
    void onCancel();
    void onAdd();
    void onRemove();
    void onSelectionChanged(int index);

private:
    QString m_pluginDir;
    QStringList m_plugins;
    Ui_PluginDlg* m_ui;
};


} // namespace Ui {}

} // namespace Client {}

} // namespace Erp {}
