#pragma once

#include <erebus-gui/erebus-gui.hpp>

#include "ui_connectdlg.h"

#include <QDialog>


namespace Erp
{

namespace Client
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
        bool ssl,
        const std::string& rootCA,
        const std::string& certificate,
        const std::string& key,
        QWidget* parent = nullptr
    );

    const std::string& selected() const noexcept
    {
        return m_selected;
    }

    bool ssl() const noexcept
    {
        return m_ssl;
    }

    const std::string& rootCA() const noexcept
    {
        return m_rootCA;
    }

    const std::string& certificate() const noexcept
    {
        return m_certificate;
    }

    const std::string& key() const noexcept
    {
        return m_key;
    }

public slots:
    void onOk();
    void onCancel();
    void onSsl();
    void onBrowseRootCA();
    void onBrowseCertificate();
    void onBrowseKey();

private:
    void enableSsl(bool enable);

    Ui_ConnectDlg* m_ui;
    std::string m_certDir;
    std::string m_selected;
    bool m_ssl = false;
    std::string m_rootCA;
    std::string m_certificate;
    std::string m_key;
};

} // namespace Ui {}

} // namespace Client {}

} // namespace Erp {}
