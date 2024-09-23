#pragma once

#include "processcolumns.hpp"

#include "ui_processdlg.h"

#include <QDialog>


namespace Erp
{

namespace ProcessMgr
{

class ProcessDlg final
    : public QDialog
{
    Q_OBJECT

public:
    ~ProcessDlg();
    explicit ProcessDlg(QWidget* parent = nullptr);

public slots:
    void onClose();

private:
    Ui::ProcessDlg* m_ui;
};

} // namespace ProcessMgr {}

} // namespace Erp {}
