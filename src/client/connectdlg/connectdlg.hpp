#pragma once

#include <erebus-gui/erebus-gui.hpp>
#include <erebus-gui/settings.hpp>

#include "ui_connectdlg.h"

#include <QDialog>


namespace Erc
{

namespace Private
{

namespace Ui
{


class ConnectDlg:
    public QDialog
{
    Q_OBJECT

public:
    ~ConnectDlg();
    ConnectDlg(const std::vector<std::string>& endpoints, const std::string& user, QWidget* parent = nullptr);

    const std::string& selected() const noexcept
    {
        return m_selected;
    }

    const std::string& user() const noexcept
    {
        return m_user;
    }

    const std::string& password() const noexcept
    {
        return m_password;
    }

public slots:
    void onOk();
    void onCancel();

private:
    Ui_ConnectDlg* m_ui;
    std::string m_selected;
    std::string m_user;
    std::string m_password;
};

} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
