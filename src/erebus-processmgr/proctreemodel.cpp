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
    , m_columns(&columns)
{
    // some fixed columns
    Q_ASSERT(columns.size() >= 3);
    Q_ASSERT(columns[0].id == Er::ProcessProps::PropIndices::Comm);
    Q_ASSERT(columns[1].id == Er::ProcessProps::PropIndices::Pid);
    Q_ASSERT(columns[2].id == Er::ProcessProps::PropIndices::CpuUsage);

    update(changeset);
}

void ProcessTreeModel::setColumns(const ProcessColumns& columns)
{
    // some fixed columns
    Q_ASSERT(columns.size() >= 3);
    Q_ASSERT(columns[0].id == Er::ProcessProps::PropIndices::Comm);
    Q_ASSERT(columns[1].id == Er::ProcessProps::PropIndices::Pid);
    Q_ASSERT(columns[2].id == Er::ProcessProps::PropIndices::CpuUsage);

    // remove everything except [Comm], [Pid] & [%CPU] which are mandatory
    auto toRemove = (m_columns->size() > 3) ? (m_columns->size() - 3) : 0;
    if (toRemove)
        removeColumns(3, toRemove);

    // and then insert any columns required
    auto toInsert = (columns.size() > 3) ? (columns.size() - 3) : 0;
    if (toInsert)
        insertColumns(3, toInsert);

    m_columns = &columns;
}

std::vector<QModelIndex> ProcessTreeModel::update(std::shared_ptr<Changeset> changeset)
{
    std::vector<QModelIndex> parentsToExpand;

    m_firstRun = changeset->firstRun;
    m_rTime = changeset->realTime;

    if (!m_tree)
    {
        beginResetModel();

        assert(changeset->firstRun);
        m_tree.reset(new ItemTree(changeset->modified));

        endResetModel();
    }
    else
    {
        auto beginInsert = [this, &parentsToExpand](ItemTree::Node* node, ItemTree::Node* parent, size_t idx)
        {
            auto parentIndex = index(parent);
            beginInsertRows(parentIndex, idx, idx);
            parentsToExpand.push_back(parentIndex);
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
        for (auto& removed: changeset->purged)
        {
            Er::ObjectLock<Item> locked(removed.get());

            m_tree->remove(removed.get(), beginRemove, endRemove, beginMove, endMove);
        }

        assert(!changeset->firstRun);

        // handle modified processes
        for (auto& modified : changeset->modified)
        {
            Er::ObjectLock<Item> locked(modified.get());

            auto node = static_cast<ItemTree::Node*>(locked->context());
            if (!node)
            {
                // new item
                m_tree->insert(modified, beginInsert, endInsert, beginMove, endMove);
                assert(locked->context());
            }
            else
            {
                // existing item
                QVector<int> roles;
                roles.push_back(Qt::DisplayRole);
                emit dataChanged(index(0, 0, index(node->parent())), index(0, m_columns->size(), index(node->parent())), roles);
            }
        }

        // handle modified processes
        for (auto& iconed : changeset->iconed)
        {
            Er::ObjectLock<Item> locked(iconed.get());

            auto node = static_cast<ItemTree::Node*>(locked->context());
            if (node)
            {
                // existing item
                QVector<int> roles;
                roles.push_back(Qt::DecorationRole);
                emit dataChanged(index(0, 0, index(node->parent())), index(0, m_columns->size(), index(node->parent())), roles);
            }
        }

        // handle tracked processes
        for (auto& tracked : changeset->tracked)
        {
            Er::ObjectLock<Item> locked(tracked.get());

            auto node = static_cast<ItemTree::Node*>(locked->context());
            assert(node);
            if (!node)
                continue;

            if (node->statePainted != locked->state())
            {
                QVector<int> roles;
                roles.push_back(Qt::BackgroundRole);
                emit dataChanged(index(0, 0, index(node->parent())), index(0, m_columns->size(), index(node->parent())), roles);

                node->statePainted = locked->state();
            }
        }

        // handle untracked processes
        for (auto& untracked : changeset->untracked)
        {
            Er::ObjectLock<Item> locked(untracked.get());

            auto node = static_cast<ItemTree::Node*>(locked->context());
            assert(node);
            if (!node)
                continue;

            if (node->statePainted != locked->state())
            {
                QVector<int> roles;
                roles.push_back(Qt::BackgroundRole);
                emit dataChanged(index(0, 0, index(node->parent())), index(0, m_columns->size(), index(node->parent())), roles);
            }
        }
    }

    return parentsToExpand;
}

uint64_t ProcessTreeModel::pid(const QModelIndex& index) const
{
    if (!index.isValid())
        return uint64_t(-1);

    auto node = static_cast<ItemTreeNode*>(index.internalPointer());
    Er::ObjectLock<Item> lock(node->data());

    return node->data()->pid;
}

QVariant ProcessTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto node = static_cast<ItemTreeNode*>(index.internalPointer());
    Er::ObjectLock<Item> lock(node->data());

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

    if (section >= m_columns->size())
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        return QVariant((*m_columns)[section].label);
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
    return static_cast<int>(m_columns->size());
}

QVariant ProcessTreeModel::formatItemProperty(ItemTreeNode* item, Er::PropId id) const noexcept
{
    return Er::protectedCall<QVariant>(
        m_log,
        ErLogInstance("ProcessTreeModel"),
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
    if (column >= m_columns->size())
        return QVariant();

    auto id = (*m_columns)[column].id;

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

    case Er::ProcessProps::PropIndices::User:
        return formatItemProperty(item, Er::ProcessProps::User::Id::value);

    case Er::ProcessProps::PropIndices::CmdLine:
        return formatItemProperty(item, Er::ProcessProps::CmdLine::Id::value);

    case Er::ProcessProps::PropIndices::Exe:
        return formatItemProperty(item, Er::ProcessProps::Exe::Id::value);

    case Er::ProcessProps::PropIndices::ThreadCount:
        return formatItemProperty(item, Er::ProcessProps::ThreadCount::Id::value);

    case Er::ProcessProps::PropIndices::Tty:
        return formatItemProperty(item, Er::ProcessProps::Tty::Id::value);

    case Er::ProcessProps::PropIndices::STime:
        return formatItemProperty(item, Er::ProcessProps::STime::Id::value);

    case Er::ProcessProps::PropIndices::UTime:
        return formatItemProperty(item, Er::ProcessProps::UTime::Id::value);

    case Er::ProcessProps::PropIndices::StartTime:
        return QVariant(item->data()->startTimeUtc);

    case Er::ProcessProps::PropIndices::State:
        return QVariant(item->data()->processState);

    case Er::ProcessProps::PropIndices::CpuUsage:
    {
        if (m_firstRun || (m_rTime < 0.000001))
            return QVariant();

        auto s = item->data()->sTimeDiff;
        auto u = item->data()->uTimeDiff;

        if (!s && !u)
            return QVariant();

        auto usage = ((s ? *s : 0.0) + (u ? *u : 0.0)) * 100.0 / m_rTime;
        usage = std::clamp(usage, 0.0, 100.0);
        if (usage < 0.01)
            return QVariant();

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << usage;

        return QVariant(QString::fromLatin1(ss.str()));
    }

    default:
        return QVariant();
    }
}

QVariant ProcessTreeModel::tooltipForCell(ItemTreeNode* item, int column) const
{
    if (column >= m_columns->size())
        return QVariant();

    auto id = (*m_columns)[column].id;

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
    return Er::protectedCall<QVariant>(
        m_log,
        ErLogInstance("ProcessTreeModel"),
        [this, item]()
        {
            // look in cache
            auto& iconData = item->data()->icon;
            if (iconData.state == ProcessInformation::IconData::State::Valid)
            {
                return QVariant(iconData.icon);
            }

            return QVariant();
        }
    );
}


} // namespace Private {}

} // namespace Erp {}
