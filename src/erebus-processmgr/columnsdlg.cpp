#include "columnsdlg.hpp"


namespace Erp
{

namespace ProcessMgr
{


ColumnsDlg::~ColumnsDlg()
{
    delete m_ui;
}

ColumnsDlg::ColumnsDlg(const ProcessColumns& columns, std::span<const ProcessColumnDef> columnDefs, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::ColumnsDlg())
    , m_columns()
{
    m_ui->setupUi(this);
    m_ui->listActive->setFocusPolicy(Qt::ClickFocus);
    m_ui->listInactive->setFocusPolicy(Qt::ClickFocus);

    // active columns
    for (auto& column: columns)
    {
        if (column.type < ProcessColumnDef::Type::Mandatory)
        {
            auto item = new Item(column, m_ui->listActive);
            m_active.push_back(item);
            m_ui->listActive->addItem(item);
        }
        else
        {
            m_mandatory.append(column);
        }
    }

    // inactive columns
    for (auto& def: columnDefs)
    {
        auto it = std::find_if(columns.begin(), columns.end(), [id = def.id](const ProcessColumn& c) { return c.id == id; });
        if (it == columns.end())
        {
            auto item = new Item(def, m_ui->listInactive);
            m_inactive.push_back(item);
            m_ui->listInactive->addItem(item);
        }
    }

    if (!m_active.empty())
    {
        m_activeSelected = m_active.back();
        m_ui->listActive->setCurrentItem(m_activeSelected, QItemSelectionModel::ClearAndSelect);

        m_activeSelected->setSelected(true);
    }
    else if (!m_inactive.empty())
    {
        m_inactiveSelected = m_inactive.back();
        m_ui->listInactive->setCurrentItem(m_inactiveSelected, QItemSelectionModel::ClearAndSelect);

        m_inactiveSelected->setSelected(true);
    }

    updateButtons();
}

void ColumnsDlg::updateButtons()
{
    if (m_activeSelected)
    {
        auto index = getItemIndex(m_active, m_activeSelected);

        m_ui->buttonActivate->setEnabled(false);
        m_ui->buttonInactivate->setEnabled(true);
        m_ui->buttonUp->setEnabled(index > 0);
        m_ui->buttonDown->setEnabled(index + 1 < m_active.size());
    }
    else if (m_inactiveSelected)
    {
        m_ui->buttonActivate->setEnabled(true);
        m_ui->buttonInactivate->setEnabled(false);
        m_ui->buttonUp->setEnabled(false);
        m_ui->buttonDown->setEnabled(false);
    }
    else
    {
        m_ui->buttonActivate->setEnabled(false);
        m_ui->buttonInactivate->setEnabled(false);
        m_ui->buttonUp->setEnabled(false);
        m_ui->buttonDown->setEnabled(false);
    }
}

void ColumnsDlg::onOk()
{
    // force mandatory columns
    for (auto& c: m_mandatory)
    {
        m_columns.append(c);
    }

    for (auto& c: m_active)
    {
        auto it = std::find_if(m_columns.begin(), m_columns.end(), [id = c->column.id](const ProcessColumn& col) { return col.id == id; });
        if (it == m_columns.end())
        {
            m_columns.append(ProcessColumn(c->column));
        }
    }

    accept();
}

void ColumnsDlg::onCancel()
{
    reject();
}

void ColumnsDlg::onActivate()
{
    if (m_inactiveSelected)
    {
        auto item = m_inactiveSelected;
        auto it = std::find(m_inactive.begin(), m_inactive.end(), item);
        auto index = std::distance(m_inactive.begin(), it);
        m_ui->listInactive->takeItem(int(index));
        m_inactive.erase(it);
        m_active.push_back(item);
        m_ui->listActive->addItem(item);

        if (m_activeSelected)
        {
            m_activeSelected->setSelected(false);
            m_activeSelected = nullptr;
        }

        m_inactiveSelected = nullptr;
        item->setSelected(false);

        m_ui->listActive->setCurrentItem(nullptr, QItemSelectionModel::ClearAndSelect);

        if (!m_inactive.empty())
        {
            if (index > 0)
                --index;

            m_inactiveSelected = m_inactive[index];
            m_inactive[index]->setSelected(true);
            m_ui->listInactive->setCurrentItem(m_inactive[index], QItemSelectionModel::ClearAndSelect);
        }
        else
        {
            m_ui->listInactive->setCurrentItem(nullptr, QItemSelectionModel::ClearAndSelect);
        }

        updateButtons();
    }
}

void ColumnsDlg::onInactivate()
{
    if (m_activeSelected)
    {
        auto item = m_activeSelected;
        auto it = std::find(m_active.begin(), m_active.end(), item);
        auto index = std::distance(m_active.begin(), it);
        m_ui->listActive->takeItem(int(index));
        m_active.erase(it);
        m_inactive.push_back(item);
        m_ui->listInactive->addItem(item);

        if (m_inactiveSelected)
        {
            m_inactiveSelected->setSelected(false);
            m_inactiveSelected = nullptr;
        }

        m_activeSelected = nullptr;
        item->setSelected(false);

        m_ui->listInactive->setCurrentItem(nullptr, QItemSelectionModel::ClearAndSelect);

        if (!m_active.empty())
        {
            if (index > 0)
                --index;

            m_activeSelected = m_active[index];
            m_active[index]->setSelected(true);
            m_ui->listActive->setCurrentItem(m_active[index], QItemSelectionModel::ClearAndSelect);
        }
        else
        {
            m_ui->listActive->setCurrentItem(nullptr, QItemSelectionModel::ClearAndSelect);
        }

        updateButtons();
    }
}

void ColumnsDlg::onUp()
{
    if (m_activeSelected)
    {
        auto index = getItemIndex(m_active, m_activeSelected);
        if (index > 0)
        {
            m_activeSelected->setSelected(false);
            m_activeSelected = nullptr;
            std::swap(m_active[index], m_active[index - 1]);
            auto item = m_ui->listActive->takeItem(int(index));
            m_ui->listActive->insertItem(int(index - 1), item);
            m_activeSelected = static_cast<Item*>(item);
            m_activeSelected->setSelected(true);
            m_ui->listActive->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
        }
    }
}

void ColumnsDlg::onDown()
{
    if (m_activeSelected)
    {
        auto index = getItemIndex(m_active, m_activeSelected);
        if (index + 1 < m_active.size())
        {
            m_activeSelected->setSelected(false);
            m_activeSelected = nullptr;
            std::swap(m_active[index], m_active[index + 1]);
            auto item = m_ui->listActive->takeItem(int(index));
            m_ui->listActive->insertItem(int(index + 1), item);
            m_activeSelected = static_cast<Item*>(item);
            m_activeSelected->setSelected(true);
            m_ui->listActive->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
        }
    }
}

size_t ColumnsDlg::getItemIndex(const std::vector<Item*>& v, QListWidgetItem* item)
{
    auto it = std::find(v.begin(), v.end(), item);
    assert(it != v.end());
    auto index = std::distance(v.begin(), it);
    return index;
}

void ColumnsDlg::onInactiveClicked(QListWidgetItem* item)
{
    if (m_activeSelected)
    {
        m_activeSelected->setSelected(false);
        m_activeSelected = nullptr;
    }

    m_inactiveSelected = static_cast<Item*>(item);
    m_ui->listInactive->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);

    updateButtons();
}

void ColumnsDlg::onActiveClicked(QListWidgetItem* item)
{
    if (m_inactiveSelected)
    {
        m_inactiveSelected->setSelected(false);
        m_inactiveSelected = nullptr;
    }

    m_activeSelected = static_cast<Item*>(item);
    m_ui->listActive->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);

    updateButtons();
}

void ColumnsDlg::onActiveCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    if (m_inactiveSelected)
    {
        m_inactiveSelected->setSelected(false);
        m_inactiveSelected = nullptr;
    }

    if (previous)
        previous->setSelected(false);

    if (current)
        current->setSelected(true);

    m_activeSelected = static_cast<Item*>(current);

    updateButtons();
}

void ColumnsDlg::onInactiveCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    if (m_activeSelected)
    {
        m_activeSelected->setSelected(false);
        m_activeSelected = nullptr;
    }

    if (previous)
        previous->setSelected(false);

    if (current)
        current->setSelected(true);

    m_inactiveSelected = static_cast<Item*>(current);

    updateButtons();
}

void ColumnsDlg::onActiveSelectionChanged()
{
}

void ColumnsDlg::onInactiveSelectionChanged()
{
}


} // namespace ProcessMgr {}

} // namespace Erp {}
