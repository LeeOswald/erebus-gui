#pragma once

#include <erebus/tree.hxx>

#include "processcolumns.hpp"
#include "proclistworker.hpp"

#include <QAbstractItemModel>

namespace Erp
{

namespace Private
{

class ProcessTreeModel final
    : public QAbstractItemModel
{
    Q_OBJECT

public:
    using Changeset = IProcessList::Changeset;

    ~ProcessTreeModel();
    explicit ProcessTreeModel(Er::Log::ILog* log, std::shared_ptr<Changeset> changeset, const ProcessColumns& columns, QObject* parent = nullptr);

    void update(std::shared_ptr<Changeset> changeset);

    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;

private:
    using Item = IProcessList::Item;
    using ItemPtr = std::shared_ptr<Item>;

    struct NodeData
    {
        Item::TimePoint timePainted = Item::Never;
        Item::State statePainted = Item::State::Existing;
    };

    using ItemTree = Er::Tree<Item, ItemPtr, NodeData>;
    using ItemTreeNode = ItemTree::Node;

    QVariant formatItemProperty(ItemTreeNode* item, Er::PropId id) const noexcept;
    QVariant textForCell(ItemTreeNode* item, int column) const;
    QVariant tooltipForCell(ItemTreeNode* item, int column) const;
    QVariant backgroundForRow(const ItemTreeNode* item) const;
    QVariant iconForItem(const ItemTreeNode* item) const;
    QModelIndex index(const ItemTree::Node* node) const;

    Er::Log::ILog* m_log;
    std::shared_ptr<ItemTree> m_tree;
    const ProcessColumns& m_columns;
};



} // namespace Private {}

} // namespace Erp {}
