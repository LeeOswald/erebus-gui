#pragma once

#include <erebus/lrucache.hxx>
#include <erebus/tree.hxx>

#include "processcolumns.hpp"
#include "proclistworker.hpp"

#include <QAbstractItemModel>

#include <vector>


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

    void setColumns(const ProcessColumns& columns);
    std::vector<QModelIndex> update(std::shared_ptr<Changeset> changeset);

    uint64_t pid(const QModelIndex& index) const;

    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;

private:
    using Item = IProcessList::Item;

    struct NodeData
    {
        Item::State statePainted = Item::State::Undefined;
    };

    using ItemPtr = std::shared_ptr<Item>;
    using ItemTree = Er::Tree<Item, ItemPtr, NodeData>;
    using ItemTreeNode = ItemTree::Node;

    QVariant formatItemProperty(ItemTreeNode* item, Er::PropId id) const noexcept;
    QVariant textForCell(ItemTreeNode* item, int column) const;
    QVariant tooltipForCell(ItemTreeNode* item, int column) const;
    QVariant backgroundForRow(const ItemTreeNode* item) const;
    QVariant iconForItem(const ItemTreeNode* item) const;
    QModelIndex index(const ItemTree::Node* node) const;

    Er::Log::ILog* m_log;
    std::unique_ptr<ItemTree> m_tree;
    const ProcessColumns* m_columns;
    static constexpr size_t IconCacheSize = 1024;
    mutable Er::LruCache<QString, QIcon> m_iconCache; // comm -> icon
    bool m_firstRun = false;
    double m_rTime = 0.0;
};



} // namespace Private {}

} // namespace Erp {}
