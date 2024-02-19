#pragma once

#include <erebus-gui/erebus-gui.hpp>

#include "ui_connectdlg.h"

#include <QDialog>


namespace Erc
{

namespace Private
{

namespace Ui
{


class ConnectDlg final
    : public QDialog
{
    Q_OBJECT

public:
    ~ConnectDlg();

    explicit ConnectDlg(
        const std::vector<std::string>& endpoints,
        const std::string& user,
        bool ssl,
        const std::string& rootCA,
        QWidget* parent = nullptr
    );

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

    bool ssl() const noexcept
    {
        return m_ssl;
    }

    const std::string& rootCA() const noexcept
    {
        return m_rootCA;
    }

public slots:
    void onOk();
    void onCancel();
    void onSsl();
    void onBrowseRootCA();

private:
    void enableSsl(bool enable);

    Ui_ConnectDlg* m_ui;
    std::string m_certDir;
    std::string m_selected;
    std::string m_user;
    std::string m_password;
    bool m_ssl = false;
    std::string m_rootCA;
};

} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
