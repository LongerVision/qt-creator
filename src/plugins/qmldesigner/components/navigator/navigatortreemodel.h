/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "navigatormodelinterface.h"

#include <modelnode.h>
#include <nodemetainfo.h>

#include <QAbstractItemModel>
#include <QPointer>
#include <QDateTime>

QT_FORWARD_DECLARE_CLASS(QPixmap)

namespace QmlDesigner {

class Model;
class NavigatorView;
class ModelNode;
class DesignerActionManager;

class NavigatorTreeModel : public QAbstractItemModel, public NavigatorModelInterface
{
    Q_OBJECT

public:

    enum ColumnType {
        Name = 0,
        Alias,
        Visibility,
        Lock,
        Count
    };

    explicit NavigatorTreeModel(QObject *parent = nullptr);
    ~NavigatorTreeModel() override;

    QVariant data(const QModelIndex &index, int role) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void setView(NavigatorView *view);

    ModelNode modelNodeForIndex(const QModelIndex &index) const;
    bool hasModelNodeForIndex(const QModelIndex &index) const;

    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex &parent) override;


    QModelIndex indexForModelNode(const ModelNode &node) const override;

    QModelIndex createIndexFromModelNode(int row, int column, const ModelNode &modelNode) const;

    Qt::DropActions supportedDropActions() const override;
    Qt::DropActions supportedDragActions() const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    void notifyDataChanged(const ModelNode &modelNode) override;
    void notifyModelNodesRemoved(const QList<ModelNode> &modelNodes) override;
    void notifyModelNodesInserted(const QList<ModelNode> &modelNodes) override;
    void notifyModelNodesMoved(const QList<ModelNode> &modelNodes) override;
    void notifyIconsChanged() override;
    void setFilter(bool showOnlyVisibleItems) override;
    void setNameFilter(const QString &filter) override;
    void setOrder(bool reverseItemOrder) override;
    void resetModel() override;

    void updateToolTipPixmap(const ModelNode &node, const QPixmap &pixmap);

signals:
    void toolTipPixmapUpdated(const QString &id, const QPixmap &pixmap) const;

private:
    void moveNodesInteractive(NodeAbstractProperty &parentProperty, const QList<ModelNode> &modelNodes,
                              int targetIndex, bool executeInTransaction = true);
    void handleInternalDrop(const QMimeData *mimeData, int rowNumber, const QModelIndex &dropModelIndex);
    void handleItemLibraryItemDrop(const QMimeData *mimeData, int rowNumber, const QModelIndex &dropModelIndex);
    ModelNode handleItemLibraryImageDrop(const QString &imagePath, NodeAbstractProperty targetProperty,
                                         const QModelIndex &rowModelIndex, bool &outMoveNodesAfter);
    ModelNode handleItemLibraryFontDrop(const QString &fontFamily, NodeAbstractProperty targetProperty,
                                        const QModelIndex &rowModelIndex);
    ModelNode handleItemLibraryShaderDrop(const QString &shaderPath, bool isFragShader,
                                          NodeAbstractProperty targetProperty,
                                          const QModelIndex &rowModelIndex,
                                          bool &outMoveNodesAfter);
    ModelNode handleItemLibrarySoundDrop(const QString &soundPath, NodeAbstractProperty targetProperty,
                                         const QModelIndex &rowModelIndex);
    ModelNode handleItemLibraryTexture3dDrop(const QString &tex3DPath, NodeAbstractProperty targetProperty,
                                             const QModelIndex &rowModelIndex, bool &outMoveNodesAfter);
    bool dropAsImage3dTexture(const ModelNode &targetNode, const NodeAbstractProperty &targetProp,
                              const QString &imagePath, ModelNode &newNode, bool &outMoveNodesAfter);
    ModelNode createTextureNode(const NodeAbstractProperty &targetProp, const QString &imagePath);
    void assignMaterialToModel(const ModelNode &node);
    QList<QPersistentModelIndex> nodesToPersistentIndex(const QList<ModelNode> &modelNodes);
    void addImport(const QString &importName);
    QList<ModelNode> filteredList(const NodeListProperty &property, bool filter, bool reverseOrder) const;
    bool moveNodeToParent(const NodeAbstractProperty &targetProperty, const ModelNode &newModelNode);

    QPointer<NavigatorView> m_view;
    mutable QHash<ModelNode, QModelIndex> m_nodeIndexHash;
    mutable QHash<ModelNode, QList<ModelNode> > m_rowCache;
    bool m_showOnlyVisibleItems = true;
    bool m_reverseItemOrder = false;
    DesignerActionManager *m_actionManager = nullptr;
    QString m_nameFilter;
    QList<ModelNode> m_nameFilteredList;
};

} // namespace QmlDesigner
