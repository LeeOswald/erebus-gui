#include "client-version.h"

#include "errorbox.hpp"

namespace Erc
{

namespace Private
{

namespace Ui
{

ErrorBox::~ErrorBox()
{
    delete m_ui;
}

ErrorBox::ErrorBox(const QString& title, const QString& message, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui_ErrorBox())
{
    m_ui->setupUi(this);
    setWindowTitle(title);

    m_ui->textDetails->appendPlainText(message);
}

void ErrorBox::onOk()
{
    accept();
}


} // namespace Ui {}

} // namespace Private {}



namespace Ui
{

EREBUSGUI_EXPORT void errorBox(const QString& title, const QString& message, QWidget* parent)
{
    Erc::Private::Ui::ErrorBox box(title, message, parent);
    box.exec();
}

} // namespace Ui {}

} // namespace Erc {}
