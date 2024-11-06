#include "connectdlg.hpp"

#include <filesystem>

#include <QFileDialog>

namespace Erp
{

namespace Client
{

namespace Ui
{

ConnectDlg::~ConnectDlg()
{
    delete m_ui;
}

ConnectDlg::ConnectDlg(
    const std::vector<std::string>& endpoints,
    bool ssl,
    const std::string& rootCA,
    const std::string& certificate,
    const std::string& key,
    QWidget* parent
    )
    : QDialog(parent)
    , m_ui(new Ui_ConnectDlg())
{
    m_ui->setupUi(this);

    for (auto& ep : endpoints)
    {
        auto u16 = Erc::fromUtf8(ep);
        m_ui->comboEndpoints->addItem(u16);
    }

    if (!endpoints.empty())
        m_ui->comboEndpoints->setCurrentIndex(0);

    m_ui->comboEndpoints->setFocus();

    m_ui->editRootCA->setText(Erc::fromUtf8(rootCA));
    m_ui->editCertificate->setText(Erc::fromUtf8(certificate));
    m_ui->editKey->setText(Erc::fromUtf8(key));
    m_ui->checkSsl->setCheckState(ssl ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    enableSsl(ssl);

    if (!rootCA.empty())
    {
        std::filesystem::path certPath(rootCA);
        if (certPath.has_parent_path())
            m_certDir = certPath.parent_path().string();
    }
}

void ConnectDlg::onOk()
{
    auto endpoint = m_ui->comboEndpoints->currentText();
    if (endpoint.isEmpty())
    {
        return Erc::Ui::errorBoxLite(windowTitle(), tr("Please specify the enpoint address"), this);
    }

    m_selected = Erc::toUtf8(endpoint);
    m_ssl = (m_ui->checkSsl->checkState() == Qt::CheckState::Checked);
    auto rootCA = m_ui->editRootCA->text();
    if (m_ssl && rootCA.isEmpty())
    {
        return Erc::Ui::errorBoxLite(windowTitle(), tr("Please specify the CA certificate"), this);
    }

    auto certificate = m_ui->editCertificate->text();
    if (m_ssl && certificate.isEmpty())
    {
        return Erc::Ui::errorBoxLite(windowTitle(), tr("Please specify the certificate"), this);
    }

    auto key = m_ui->editKey->text();
    if (m_ssl && key.isEmpty())
    {
        return Erc::Ui::errorBoxLite(windowTitle(), tr("Please specify the certificate key"), this);
    }

    m_rootCA = Erc::toUtf8(rootCA);
    m_certificate = Erc::toUtf8(certificate);
    m_key = Erc::toUtf8(key);

    accept();
}

void ConnectDlg::onCancel()
{
    reject();
}

void ConnectDlg::onSslChecked(Qt::CheckState state)
{
    enableSsl(state == Qt::CheckState::Checked);
}

void ConnectDlg::onBrowseRootCA()
{
    auto fileName = QFileDialog::getOpenFileName(
        this,
        tr("Select CA Certificate"),
        Erc::fromUtf8(m_certDir),
        tr("Certificates (*.pem)")
    );

    m_ui->editRootCA->setText(fileName);
}

void ConnectDlg::onBrowseCertificate()
{
    auto fileName = QFileDialog::getOpenFileName(
        this,
        tr("Select Certificate"),
        Erc::fromUtf8(m_certDir),
        tr("Certificates (*.pem)")
        );

    m_ui->editCertificate->setText(fileName);
}

void ConnectDlg::onBrowseKey()
{
    auto fileName = QFileDialog::getOpenFileName(
        this,
        tr("Select Certificate Key"),
        Erc::fromUtf8(m_certDir),
        tr("Certificates (*.pem)")
        );

    m_ui->editKey->setText(fileName);
}

void ConnectDlg::enableSsl(bool enable)
{
    m_ui->labelRootCA->setEnabled(enable);
    m_ui->editRootCA->setEnabled(enable);
    m_ui->btnBrowseRootCA->setEnabled(enable);

    m_ui->labelCertificate->setEnabled(enable);
    m_ui->editCertificate->setEnabled(enable);
    m_ui->btnBrowseCertificate->setEnabled(enable);

    m_ui->labelKey->setEnabled(enable);
    m_ui->editKey->setEnabled(enable);
    m_ui->btnBrowseKey->setEnabled(enable);
}

} // namespace Ui {}

} // namespace Client {}

} // namespace Erp {}
