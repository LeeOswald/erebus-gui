#include "processdlg.hpp"


namespace Erp
{

namespace ProcessMgr
{


ProcessDlg::~ProcessDlg()
{
    delete m_ui;
}

ProcessDlg::ProcessDlg(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::ProcessDlg())
{
    m_ui->setupUi(this);
}

void ProcessDlg::onClose()
{
    reject();
}


} // namespace ProcessMgr {}

} // namespace Erp {}
