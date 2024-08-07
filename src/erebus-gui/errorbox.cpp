#include "client-version.h"

#include "errorbox.hpp"

#include <QMessageBox>

namespace Erp
{

namespace Ui
{



ErrorBoxDlg::~ErrorBoxDlg()
{
    delete m_ui;
}

ErrorBoxDlg::ErrorBoxDlg(const QString& title, const QString& message, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui_ErrorBox())
{
    m_ui->setupUi(this);
    setWindowTitle(title);

    m_ui->textDetails->appendPlainText(message);
}

void ErrorBoxDlg::onOk()
{
    accept();
}


} // namespace Ui {}

} // namespace Erp {}


namespace Erc
{

namespace Ui
{

EREBUSGUI_EXPORT void errorBox(const QString& title, const QString& message, QWidget* parent)
{
    Erp::Ui::ErrorBoxDlg box(title, message, parent);
    box.exec();
}

EREBUSGUI_EXPORT void errorBoxLite(const QString& title, const QString& message, QWidget* parent)
{
    QMessageBox::warning(parent, title, message, QMessageBox::Ok);
}

} // namespace Ui {}

} // namespace Erc {}
