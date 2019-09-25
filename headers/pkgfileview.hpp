#pragma once

#include <QTreeView>

class PkgFileView : public QTreeView
{
    Q_OBJECT

public:
    PkgFileView( QWidget* parent = nullptr );

protected:
    virtual void startDrag( Qt::DropActions supportedActions ) override;

    virtual void dragMoveEvent( QDragMoveEvent* event ) override;

Q_SIGNALS:
    void selected( const QItemSelection& selected,
                   const QItemSelection& deselected );

protected Q_SLOTS:
    virtual void selectionChanged( const QItemSelection& selected,
                                   const QItemSelection& deselected ) override;
};
