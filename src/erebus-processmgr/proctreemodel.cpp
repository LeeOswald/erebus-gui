#include "proctreemodel.hpp"


namespace Erp
{

namespace Private
{

ProcessTreeModel::~ProcessTreeModel()
{
}

ProcessTreeModel::ProcessTreeModel(Er::Log::ILog* log, std::shared_ptr<Changeset> changeset, const ProcessColumns& columns, QObject* parent)
    : QAbstractItemModel(parent)
    , m_log(log)
    , m_columns(columns)
{
    update(changeset);
}

void ProcessTreeModel::update(std::shared_ptr<Changeset> changeset)
{
    m_log->write(Er::Log::Level::Debug, LogInstance("Model"), "update() ->");

    // sort items being added in ascending order of PIDs so that parents get inserted prior to children thus avoiding unnecessary reparentings in the process
    std::sort(changeset->items.begin(), changeset->items.end(), [](const ItemPtr& a, const ItemPtr& b) { return (a->pid < b->pid); });
    // for the same pupose sort items being removed in descending order of PIDs
    std::sort(changeset->removed.begin(), changeset->removed.end(), [](const ItemPtr& a, const ItemPtr& b) { return (a->pid > b->pid); });

    if (!m_tree)
    {
        beginResetModel();

        assert(changeset->removed.empty());
        m_tree.reset(new ItemTree(changeset->items));

        endResetModel();

        m_log->write(Er::Log::Level::Debug, LogInstance("Model"), "TREE RESET");
    }
    else
    {
        auto beginInsert = [this](ItemTree::Node* node, ItemTree::Node* parent, size_t idx)
        {
            beginInsertRows(index(parent), idx, idx);
        };

        auto endInsert = [this]()
        {
            endInsertRows();
        };

        auto beginMove = [this](ItemTree::Node* node, ItemTree::Node* oldParent, size_t oldIndex, ItemTree::Node* newParent, size_t newIndex)
        {
            beginMoveRows(index(oldParent), oldIndex, oldIndex, index(newParent), newIndex);
        };

        auto endMove = [this]()
        {
            endMoveRows();
        };

        auto beginRemove = [this](ItemTree::Node* node, ItemTree::Node* parent, size_t idx)
        {
            beginRemoveRows(index(parent), idx, idx);
        };

        auto endRemove = [this]()
        {
            endRemoveRows();
        };

        // handle removed processes
        for (auto removed: changeset->removed)
        {
            LogDebug(m_log, LogComponent("ProcessTreeModel"), "REMOVING %zu [%s]", removed->pid, Erc::toUtf8(removed->comm).c_str());

            m_tree->remove(removed.get(), beginRemove, endRemove, beginMove, endMove);
        }

        // handle new/modified processes
        for (auto item: changeset->items)
        {
            auto node = static_cast<ItemTree::Node*>(item->context());

            if (!node)
            {
                // new item
                LogDebug(m_log, LogComponent("ProcessTreeModel"), "INSERTING %zu [%s]", item->pid, Erc::toUtf8(item->comm).c_str());

                m_tree->insert(item, beginInsert, endInsert, beginMove, endMove);
                assert(item->context());
                node = static_cast<ItemTree::Node*>(item->context());

                node->statePainted = item->state();
                node->timePainted = item->timeModified();
            }
            else if ((node->statePainted != item->state()) || (node->timePainted < item->timeModified()))
            {
                // item changed state
                QVector<int> roles;

                if ((node->statePainted != item->state()))
                {
                    LogDebug(m_log, LogComponent("ProcessTreeModel"), "STATE CHANGED for %zu [%s]", item->pid, Erc::toUtf8(item->comm).c_str());

                    roles.push_back(Qt::BackgroundRole);
                }

                if (node->timePainted < item->timeModified())
                {
                    LogDebug(m_log, LogComponent("ProcessTreeModel"), "DATA CHANGED for %zu [%s]", item->pid, Erc::toUtf8(item->comm).c_str());

                    roles.push_back(Qt::DisplayRole);
                }

                emit dataChanged(index(0, 0, index(node->parent())), index(0, m_columns.size(), index(node->parent())), roles);

                node->statePainted = item->state();
                node->timePainted = item->timeModified();
            }
        }
    }

    m_log->write(Er::Log::Level::Debug, LogInstance("Model"), "update() <-");
}

QVariant ProcessTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto node = static_cast<ItemTreeNode*>(index.internalPointer());

    switch (role)
    {
    case Qt::DisplayRole: return textForCell(node, index.column());
    case Qt::ToolTipRole: return tooltipForCell(node, index.column());
    case Qt::BackgroundRole: return backgroundForRow(node);
    case Qt::DecorationRole: return (index.column() == 0) ? iconForItem(node) : QVariant();
    }

    return QVariant();
}

Qt::ItemFlags ProcessTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}

QVariant ProcessTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (section >= m_columns.size())
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        return QVariant(m_columns[section].label);
    }

    return QVariant();
}

QModelIndex ProcessTreeModel::index(const ItemTree::Node* node) const
{
    if (node == m_tree->root())
    {
        return QModelIndex();
    }

    auto parent = node->parent();
    assert(parent);
    auto index = parent->indexOfChild(node);

    return createIndex(index, 0, node);
}

QModelIndex ProcessTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    auto parentItem = parent.isValid() ? static_cast<const ItemTreeNode*>(parent.internalPointer()) : m_tree->root();

    auto childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);

    return QModelIndex();
}

QModelIndex ProcessTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    auto item = static_cast<ItemTreeNode*>(index.internalPointer());
    auto parentItem = item->parent();

    if (parentItem == m_tree->root())
        return QModelIndex();

    auto grandParent = parentItem->parent();
    auto i = grandParent->indexOfChild(parentItem);

    return createIndex(int(i == ItemTreeNode::InvalidIndex ? 0 : i), 0, parentItem);
}

int ProcessTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;

    auto parentItem = parent.isValid() ? static_cast<const ItemTreeNode*>(parent.internalPointer()) : m_tree->root();

    return static_cast<int>(parentItem->children().size());
}

int ProcessTreeModel::columnCount(const QModelIndex& parent) const
{
    return static_cast<int>(m_columns.size());
}

QVariant ProcessTreeModel::formatItemProperty(ItemTreeNode* item, Er::PropId id) const noexcept
{
    return Erc::Ui::protectedCall<QVariant>(
        m_log,
        [this, item, id]()
        {
            auto it = item->data()->properties.find(id);
            if (it == item->data()->properties.end())
                return QVariant();

            auto& property = it->second;
            auto info = Er::cachePropertyInfo(property);
            Q_ASSERT(info);
            if (!info)
                return QVariant();

            std::ostringstream ss;
            info->format(property, ss);

            return QVariant(Erc::fromUtf8(ss.str()));
        }
    );
}

QVariant ProcessTreeModel::textForCell(ItemTreeNode* item, int column) const
{
    if (column >= m_columns.size())
        return QVariant();

    auto id = m_columns[column].id;

    if (!item->data()->valid)
    {
        // this process could not be normally read (maybe access denied)
        // still show its PID and error message
        switch (id)
        {
        case Er::ProcessProps::PropIndices::Comm:
            return QVariant(item->data()->error);

        case Er::ProcessProps::PropIndices::Pid:
            return QVariant(QString::number(item->data()->pid));
        }

        return QVariant();
    }

    switch (id)
    {
    case Er::ProcessProps::PropIndices::Comm:
        return QVariant(item->data()->comm);

    case Er::ProcessProps::PropIndices::Pid:
        return QVariant(QString::number(item->data()->pid));

    case Er::ProcessProps::PropIndices::PPid:
        return QVariant(QString::number(item->data()->ppid));

    case Er::ProcessProps::PropIndices::PGrp:
        return formatItemProperty(item, Er::ProcessProps::PGrp::Id::value);

    case Er::ProcessProps::PropIndices::Tpgid:
        return formatItemProperty(item, Er::ProcessProps::Tpgid::Id::value);

    case Er::ProcessProps::PropIndices::Session:
        return formatItemProperty(item, Er::ProcessProps::Session::Id::value);

    case Er::ProcessProps::PropIndices::Ruid:
        return formatItemProperty(item, Er::ProcessProps::Ruid::Id::value);

    case Er::ProcessProps::PropIndices::CmdLine:
        return formatItemProperty(item, Er::ProcessProps::CmdLine::Id::value);

    case Er::ProcessProps::PropIndices::Exe:
        return formatItemProperty(item, Er::ProcessProps::Exe::Id::value);

    case Er::ProcessProps::PropIndices::StartTime:
        return QVariant(item->data()->startTimeUtc);

    case Er::ProcessProps::PropIndices::State:
        return QVariant(item->data()->processState);

    default:
        return QVariant();
    }
}

QVariant ProcessTreeModel::tooltipForCell(ItemTreeNode* item, int column) const
{
    if (column >= m_columns.size())
        return QVariant();

    auto id = m_columns[column].id;

    if (!item->data()->valid)
    {
        // this process could not be normally read (maybe access denied)
        // still show its PID and error message
        return QVariant(item->data()->error);
    }

    switch (id)
    {
    case Er::ProcessProps::PropIndices::Comm:
    {
        QString tooltip;

        auto cmdLine = formatItemProperty(item, Er::ProcessProps::CmdLine::Id::value).toString();
        if (!cmdLine.isEmpty())
        {
            tooltip.append(cmdLine);
        }

        auto imagePath = formatItemProperty(item, Er::ProcessProps::Exe::Id::value).toString();
        if (!imagePath.isEmpty())
        {
            if (!tooltip.isEmpty())
                tooltip.append(QLatin1String("\n\n"));

            tooltip.append(tr("File: ") + imagePath);
        }

        if (!tooltip.isEmpty())
            return QVariant(tooltip);

        return QVariant();
    }

    default:
        return QVariant();
    }
}

QVariant ProcessTreeModel::backgroundForRow(const ItemTreeNode* item) const
{
    auto state = item->data()->state();
    if (state == Item::State::Deleted)
        return QVariant(QColor(255, 0, 0));
    else if (state == Item::State::New)
        return QVariant(QColor(0, 255, 0));

    if (!item->data()->valid)
        return QVariant(QColor(127, 127, 127));

    return QVariant();
}

QVariant ProcessTreeModel::iconForItem(const ItemTreeNode* item) const
{
    return QVariant();
}


} // namespace Private {}

} // namespace Erp {}
