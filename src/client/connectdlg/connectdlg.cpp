#include "connectdlg.hpp"

#include <filesystem>

#include <QFileDialog>

namespace Erc
{

namespace Private
{

namespace Ui
{

ConnectDlg::~ConnectDlg()
{
    delete m_ui;
}

ConnectDlg::ConnectDlg(
    const std::vector<std::string>& endpoints,
    const std::string& user,
    bool ssl,
    const std::string& rootCA,
    QWidget* parent
    )
    : QDialog(parent)
    , m_ui(new Ui_ConnectDlg())
{
    m_ui->setupUi(this);

    QWidget* whoNeedsFocus = nullptr;

    for (auto& ep : endpoints)
    {
        auto u16 = Erc::fromUtf8(ep);
        m_ui->comboEndpoints->addItem(u16);
    }

    if (!endpoints.empty())
        m_ui->comboEndpoints->setCurrentIndex(0);
    else
        whoNeedsFocus = m_ui->comboEndpoints;

    if (!user.empty())
        m_ui->editUser->setText(Erc::fromUtf8(user));
    else
        whoNeedsFocus = m_ui->editUser;

    if (!whoNeedsFocus)
        whoNeedsFocus = m_ui->editPassword;

    m_ui->editRootCA->setText(Erc::fromUtf8(rootCA));
    m_ui->checkSsl->setCheckState(ssl ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    enableSsl(ssl);

    if (!rootCA.empty())
    {
        std::filesystem::path certPath(rootCA);
        if (certPath.has_parent_path())
            m_certDir = certPath.parent_path().string();
    }

    Q_ASSERT(whoNeedsFocus);
    whoNeedsFocus->setFocus();
}

void ConnectDlg::onOk()
{
    auto endpoint = m_ui->comboEndpoints->currentText();
    if (endpoint.isEmpty())
    {
        return Erc::Ui::errorBoxLite(windowTitle(), tr("Please specify the enpoint address"), this);
    }

    m_selected = Erc::toUtf8(endpoint);

    auto user = m_ui->editUser->text();
    if (user.isEmpty())
    {
        return Erc::Ui::errorBoxLite(windowTitle(), tr("Please specify the user name"), this);
    }

    m_user = Erc::toUtf8(user);

    auto password = m_ui->editPassword->text();
    if (password.isEmpty())
    {
        return Erc::Ui::errorBoxLite(windowTitle(), tr("Please specify the password"), this);
    }

    m_password = Erc::toUtf8(password);

    m_ssl = (m_ui->checkSsl->checkState() == Qt::CheckState::Checked);
    auto rootCA = m_ui->editRootCA->text();
    if (m_ssl && rootCA.isEmpty())
    {
        return Erc::Ui::errorBoxLite(windowTitle(), tr("Please specify the root CA certificate"), this);
    }

    m_rootCA = Erc::toUtf8(rootCA);

    accept();
}

void ConnectDlg::onCancel()
{
    reject();
}

void ConnectDlg::onSsl()
{
    enableSsl(m_ui->checkSsl->checkState() == Qt::CheckState::Checked);
}

void ConnectDlg::onBrowseRootCA()
{
    auto fileName = QFileDialog::getOpenFileName(
        this,
        tr("Select Root CA Certificate"),
        Erc::fromUtf8(m_certDir),
        tr("Certificates (*.pem)")
    );

    m_ui->editRootCA->setText(fileName);
}

void ConnectDlg::enableSsl(bool enable)
{
    m_ui->labelRootCA->setEnabled(enable);
    m_ui->editRootCA->setEnabled(enable);
    m_ui->btnBrowseRootCA->setEnabled(enable);
}

} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
