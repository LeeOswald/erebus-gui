#pragma once

#include "processcolumns.hpp"

#include "ui_columnsdlg.h"


#include <QDialog>

#include <span>


namespace Erp
{

namespace ProcessMgr
{

class ColumnsDlg final
    : public QDialog
{
    Q_OBJECT

public:
    ~ColumnsDlg();
    explicit ColumnsDlg(const ProcessColumns& columns, std::span<const ProcessColumnDef> columnDefs, QWidget* parent = nullptr);

    const ProcessColumns& columns() const
    {
        return m_columns;
    }

public slots:
    void onOk();
    void onCancel();
    void onActivate();
    void onInactivate();
    void onUp();
    void onDown();
    void onInactiveClicked(QListWidgetItem* item);
    void onActiveClicked(QListWidgetItem* item);
    void onActiveCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onInactiveCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onActiveSelectionChanged();
    void onInactiveSelectionChanged();
    void updateButtons();

private:
    struct Item
        : public QListWidgetItem
    {
        ProcessColumn column;

        explicit Item(const ProcessColumn& col, QListWidget* parent)
            : QListWidgetItem(col.label, parent, QListWidgetItem::UserType + 1)
            , column(col)
        {}

        explicit Item(const ProcessColumnDef& col, QListWidget* parent)
            : Item(ProcessColumn(col), parent)
        {}
    };

    static size_t getItemIndex(const std::vector<Item*>& v, QListWidgetItem* item);

    Ui::ColumnsDlg* m_ui;
    ProcessColumns m_mandatory;
    ProcessColumns m_columns;
    Item* m_activeSelected = nullptr;
    Item* m_inactiveSelected = nullptr;
    std::vector<Item*> m_active;
    std::vector<Item*> m_inactive;
};

} // namespace ProcessMgr {}

} // namespace Erp {}
