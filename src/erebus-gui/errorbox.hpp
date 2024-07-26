#pragma once

#include <erebus-gui/erebus-gui.hpp>

#include "ui_errorbox.h"

#include <QDialog>
#include <QWidget>

namespace Erp
{

namespace Ui
{


class ErrorBoxDlg final
    : public QDialog
{
    Q_OBJECT

public:
    ~ErrorBoxDlg();

    explicit ErrorBoxDlg(const QString& title, const QString& message, QWidget* parent = nullptr);

public slots:
    void onOk();

private:
    Ui_ErrorBox* m_ui;
};


} // namespace Ui {}

} // namespace Erp {}
