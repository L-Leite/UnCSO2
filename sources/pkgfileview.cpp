#include "pkgfileview.hpp"

#include <QDebug>
#include <QDragMoveEvent>
#include <QMimeData>

PkgFileView::PkgFileView( QWidget* parent /*= nullptr*/ ) : QTreeView( parent )
{
}

void PkgFileView::startDrag( Qt::DropActions supportedActions )
{
    // only start the drag if it's over the filename column. this allows
    // dragging selection in tree/detail view
    if ( currentIndex().column() != 0 )
    {
        return;
    }

    QTreeView::startDrag( supportedActions );
}

void PkgFileView::dragMoveEvent( QDragMoveEvent* event )
{
    qDebug() << event;

    if ( event->source() == this )
    {
        // we don't support internal drops yet.
        return;
    }

    QTreeView::dragMoveEvent( event );
    if ( event->mimeData()->hasFormat( QStringLiteral( "text/uri-list" ) ) )
    {
        event->acceptProposedAction();
    }
}

void PkgFileView::selectionChanged( const QItemSelection& selected,
                                    const QItemSelection& deselected )
{
    QTreeView::selectionChanged( selected, deselected );
    emit this->selected( selected, deselected );
}
