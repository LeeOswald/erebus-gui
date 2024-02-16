#include "client-version.h"

#include "connectdlg.hpp"

#include <QMessageBox>

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

ConnectDlg::ConnectDlg(const std::vector<std::string>& endpoints, const std::string& user, QWidget* parent)
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
    {
        m_ui->comboEndpoints->setCurrentIndex(0);
    }

    m_ui->editUser->setText(Erc::fromUtf8(user));
}

void ConnectDlg::onOk()
{
    auto endpoint = m_ui->comboEndpoints->currentText();
    if (endpoint.isEmpty())
    {
        QMessageBox::warning(this, QString::fromUtf8(EREBUS_APPLICATION_NAME), tr("Please specify the enpoint address"), QMessageBox::Ok);
        return;
    }

    m_selected = Erc::toUtf8(endpoint);

    auto user = m_ui->editUser->text();
    if (user.isEmpty())
    {
        QMessageBox::warning(this, QString::fromUtf8(EREBUS_APPLICATION_NAME), tr("Please specify the user name"), QMessageBox::Ok);
        return;
    }

    m_user = Erc::toUtf8(user);

    auto password = m_ui->editPassword->text();
    if (password.isEmpty())
    {
        QMessageBox::warning(this, QString::fromUtf8(EREBUS_APPLICATION_NAME), tr("Please specify the password"), QMessageBox::Ok);
        return;
    }

    m_password = Erc::toUtf8(password);

    accept();
}

void ConnectDlg::onCancel()
{
    reject();
}

} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
