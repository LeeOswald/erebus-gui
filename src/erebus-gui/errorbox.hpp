#pragma once

#include <erebus-gui/erebus-gui.hpp>

#include "ui_errorbox.h"

#include <QDialog>
#include <QWidget>

namespace Erc
{

namespace Private
{

namespace Ui
{


class ErrorBox final
    : public QDialog
{
    Q_OBJECT

public:
    ~ErrorBox();

    explicit ErrorBox(const QString& title, const QString& message, QWidget* parent = nullptr);

public slots:
    void onOk();

private:
    Ui_ErrorBox* m_ui;
};

} // namespace Ui {}

} // namespace Private {}

} // namespace Erc {}
