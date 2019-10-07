#include "pkgfilemodel.hpp"

#include <QDrag>
#include <QIcon>
#include <QLabel>
#include <QMimeDatabase>

#include "archivefilenode.hpp"
#include "pkgfilemodelsorter.hpp"

#include "dynindexfilefactory.hpp"
#include "dynpkgfilefactory.hpp"
#include "fsutils.hpp"
#include "miscutils.hpp"
#include "nodeextractionmgr.hpp"
#include "widgets/statuswidget.hpp"

PkgFileModel::PkgFileModel( QWidget* pParent /*= nullptr*/ )
    : QAbstractItemModel( pParent ), m_DirectoryNodes(), m_RootNode( "" ),
      m_iSortColumn( PFS_FileNameColumn ), m_SortOrder( Qt::AscendingOrder ),
      m_bForceSort( true ), m_bGenerated( false ), m_bIsBusy( false ),
      m_bIsIndexLoaded( false )
{
}

PkgFileModel::~PkgFileModel()
{
    this->ResetModel();
}

static QString GetIconNameForKnownExtension( const fs::path& filePath )
{
    const std::string ext = filePath.extension().generic_string();

    if ( ext == ".cfg" || ext == ".inf" || ext == ".res" || ext == ".vmt" ||
         ext == ".ecfg" || ext == ".edb" || ext == ".etxt" )
    {
        return QStringLiteral( "text-plain" );
    }

    if ( ext == ".ecsv" )
    {
        return QStringLiteral( "text-csv" );
    }

    return {};
}

static QIcon GetIconForNode( const gsl::not_null<ArchiveBaseNode*> pNode )
{
    if ( pNode->IsDirectory() == true )
    {
        return QIcon::fromTheme( QStringLiteral( "inode-directory" ) );
    }
    else
    {
        auto iconName = GetIconNameForKnownExtension( pNode->GetPath() );

        if ( iconName.isEmpty() == true )
        {
            QMimeDatabase db;

            QString convertedFilename = QString::fromStdString(
                pNode->GetPath().filename().generic_string() );
            QMimeType type = db.mimeTypeForFile( convertedFilename );

            iconName = type.iconName();
        }

        return QIcon::fromTheme( iconName );
    }
}

QVariant PkgFileModel::data( const QModelIndex& index, int role ) const
{
    if ( index.isValid() == false )
    {
        return QVariant();
    }

    ArchiveBaseNode* pNode =
        static_cast<ArchiveBaseNode*>( index.internalPointer() );
    Q_ASSERT( pNode != nullptr );

    switch ( role )
    {
        case Qt::DisplayRole:
            return pNode->GetData( index.column() );
        case Qt::DecorationRole:
            if ( index.column() == 0 )
            {
                return GetIconForNode( pNode );
            }
    }

    return QVariant();
}

bool PkgFileModel::setData( const QModelIndex& index, const QVariant& value,
                            int role /*= Qt::EditRole*/ )
{
    if ( role != Qt::CheckStateRole )
    {
        return QAbstractItemModel::setData( index, value, role );
    }

    if ( this->hasChildren( index ) )
    {
        this->UpdateNodeChildren( index, value );
    }

    emit this->dataChanged( index, index );
    return true;
}

QVariant PkgFileModel::headerData( int section, Qt::Orientation orientation,
                                   int role ) const
{
    switch ( role )
    {
        case Qt::TextAlignmentRole:
        {
            return Qt::AlignLeft;
        }

        case Qt::DisplayRole:
        {
            if ( orientation == Qt::Horizontal )
            {
                switch ( section )
                {
                    case PFS_FileNameColumn:
                        return tr( "Name" );
                    case PFS_TypeColumn:
                        return tr( "Type" );
                    case PFS_SizeColumn:
                        return tr( "Size" );
                    case PFS_OwnerPkgColumn:
                        return tr( "Owning PKG" );
                }
            }
            break;
        }
    }

    return QAbstractItemModel::headerData( section, orientation, role );
}

QModelIndex PkgFileModel::index( int row, int column,
                                 const QModelIndex& parent ) const
{
    if ( row < 0 || column < 0 || row >= this->rowCount( parent ) ||
         column >= this->columnCount( parent ) )
        return QModelIndex();

    const ArchiveDirectoryNode* pParentNode;

    if ( !parent.isValid() )
        pParentNode = &this->m_RootNode;
    else
        pParentNode =
            static_cast<ArchiveDirectoryNode*>( parent.internalPointer() );

    const int iChildVisRow = this->translateVisibleLocation(
        const_cast<ArchiveDirectoryNode*>( pParentNode ), row );

    ArchiveBaseNode* pChildNode =
        pParentNode->GetChild( gsl::narrow_cast<std::size_t>( iChildVisRow ) );

    if ( pChildNode )
        return this->createIndex( row, column, pChildNode );
    else
        return QModelIndex();
}

QModelIndex PkgFileModel::index( const ArchiveBaseNode* node, int column ) const
{
    ArchiveDirectoryNode* pParentNode = node ? node->GetParentNode() : nullptr;

    if ( node == &this->m_RootNode || pParentNode == nullptr )
        return QModelIndex();

    const int iVisualRow = pParentNode->GetLocationOf( node );
    Q_ASSERT( iVisualRow != std::numeric_limits<int>::max() );

    const int iActualVisRow =
        this->translateVisibleLocation( pParentNode, iVisualRow );
    return this->createIndex( iActualVisRow, column,
                              const_cast<ArchiveBaseNode*>( node ) );
}

QModelIndex PkgFileModel::parent( const QModelIndex& index ) const
{
    if ( !index.isValid() )
        return QModelIndex();

    auto pChildItem = static_cast<ArchiveBaseNode*>( index.internalPointer() );
    auto pParentItem = pChildItem->GetParentNode();

    if ( pParentItem == &this->m_RootNode )
        return QModelIndex();

    const int iVisualRow =
        this->translateVisibleLocation( pParentItem, pParentItem->GetRow() );

    return this->createIndex( iVisualRow, 0, pParentItem );
}

int PkgFileModel::rowCount( const QModelIndex& parent ) const
{
    if ( parent.column() > 0 )
    {
        return 0;
    }

    const ArchiveDirectoryNode* pParentItem = nullptr;

    if ( !parent.isValid() )
    {
        pParentItem = &this->m_RootNode;
    }
    else
    {
        pParentItem =
            static_cast<ArchiveDirectoryNode*>( parent.internalPointer() );
    }

    if ( pParentItem->IsDirectory() == false )
    {
        return 0;
    }

    return gsl::narrow_cast<int>( pParentItem->GetNumOfChildren() );
}

int PkgFileModel::columnCount( const QModelIndex& parent ) const
{
    return ( parent.column() > 0 ) ? 0 : PFS_NumColumns;
}

Qt::ItemFlags PkgFileModel::flags( const QModelIndex& index ) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags( index );

    if ( index.isValid() == true )
    {
        if ( index.column() == PFS_FileNameColumn )
        {
            defaultFlags |= Qt::ItemIsDragEnabled;
        }

        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | defaultFlags;
    }

    return NULL;
}

std::vector<std::pair<ArchiveBaseNode*, int>> PkgFileModel::GrabOldNodes(
    QModelIndexList& oldList )
{
    std::vector<std::pair<ArchiveBaseNode*, int>> oldNodes;

    const int nodeCount = oldList.count();
    oldNodes.reserve( gsl::narrow_cast<std::size_t>( nodeCount ) );

    for ( int i = 0; i < nodeCount; i++ )
    {
        const QModelIndex& oldNode = oldList.at( i );
        oldNodes.push_back(
            std::make_pair( this->GetNode( oldNode ), oldNode.column() ) );
    }

    return oldNodes;
}

QModelIndexList PkgFileModel::CreateNewNodeList(
    std::vector<std::pair<ArchiveBaseNode*, int>>& oldNodes )
{
    QModelIndexList newList;

    const std::size_t numOldNodes = oldNodes.size();
    newList.reserve( gsl::narrow_cast<int>( numOldNodes ) );

    for ( std::size_t i = 0; i < numOldNodes; i++ )
    {
        auto oldNode = oldNodes.at( i );
        newList.append( this->index( oldNode.first, oldNode.second ) );
    }

    return newList;
}

void PkgFileModel::sort( int column, Qt::SortOrder order )
{
    if ( this->m_SortOrder == order && this->m_iSortColumn == column &&
         this->m_bForceSort == false )
        return;

    emit this->layoutAboutToBeChanged();

    QModelIndexList oldList = this->persistentIndexList();
    auto vOldNodes = this->GrabOldNodes( oldList );

    if ( this->ShouldOrderColumn( column, order ) == true )
    {
        // we sort only from where we are, don't need to sort all the model
        for ( const auto& node : this->m_DirectoryNodes )
        {
            if ( node.second->HasChildren() == true )
            {
                this->sortChildren( column, node.second );
            }
        }

        this->m_iSortColumn = column;
        this->m_bForceSort = false;
    }
    this->m_SortOrder = order;

    QModelIndexList newList = this->CreateNewNodeList( vOldNodes );
    this->changePersistentIndexList( oldList, newList );

    emit this->layoutChanged();
}

void PkgFileModel::sortChildren( int column, ArchiveDirectoryNode* pIndexNode )
{
    Q_ASSERT( pIndexNode->IsDirectory() == true );

    if ( pIndexNode->HasChildren() == false )
    {
        return;
    }

    PkgFileModelSorter ms( column );
    pIndexNode->SortNodes<PkgFileModelSorter>( ms );

    size_t iChildrenNum = pIndexNode->GetNumOfChildren();

    for ( size_t i = 0; i < iChildrenNum; i++ )
    {
        auto pChild = BaseToDirectoryNode( pIndexNode->GetChild( i ) );

        if ( pChild != nullptr )
        {
            pChild->SortNodes<PkgFileModelSorter>( ms );
        }
    }
}

void PkgFileModel::OnSelectionChanged( const QItemSelection& selected,
                                       const QItemSelection& deselected )
{
    for ( auto&& index : selected.indexes() )
    {
        if ( index.isValid() == false || index.column() != PFS_FileNameColumn )
        {
            continue;
        }

        auto pNode = IndexToGenericNode( index );
        this->m_SelectedNodes.insert( pNode );
    }

    for ( auto&& index : deselected.indexes() )
    {
        if ( index.isValid() == false || index.column() != PFS_FileNameColumn )
        {
            continue;
        }

        auto pNode = IndexToGenericNode( index );
        this->m_SelectedNodes.erase( pNode );
    }
}

ArchiveBaseNode* PkgFileModel::GetNode( const QModelIndex& index ) const
{
    if ( !index.isValid() )
    {
        return const_cast<ArchiveDirectoryNode*>( &this->m_RootNode );
    }

    auto indexNode = static_cast<ArchiveBaseNode*>( index.internalPointer() );
    Q_ASSERT( indexNode );

    return indexNode;
}

bool PkgFileModel::LoadPackage( const fs::path& pkgPath, GameProvider provider,
                                bool bIndependentLoad /*= true*/ )
{
    this->m_bIsBusy = true;

    uc2::PkgFile::ptr_t pPkgFile;

    try
    {
        DynamicPkgFileFactory f( pkgPath, provider );
        pPkgFile = f.GetPkgFileOwnership();

        Q_ASSERT( pPkgFile != nullptr );

        pPkgFile->Parse();
        this->CreateChildren( pPkgFile->GetEntries(), pkgPath );

        if ( bIndependentLoad == true )
        {
            this->m_CurFileProps.SetPkgFileProperties( f.GetProvider(),
                                                       pPkgFile.get() );
        }

        pPkgFile->ReleaseDataBuffer();
    }
    catch ( const std::exception& e )
    {
        qDebug() << e.what();
        this->SetError( e.what() );

        this->m_bIsBusy = false;
        return false;
    }

    std::string szFilename = pkgPath.filename().generic_string();
    const size_t iPkgFileHash = GenerateHashFromString( szFilename );
    this->m_PkgFiles[iPkgFileHash] = std::move( pPkgFile );

    if ( bIndependentLoad == true )
    {
        this->m_bForceSort = true;
        this->sort( PFS_FileNameColumn );

        this->m_CurrentParentPath = pkgPath.parent_path();
    }

    this->m_bIsBusy = false;
    this->m_bGenerated = true;

    return true;
}

bool PkgFileModel::LoadIndex( const fs::path& indexPath, GameProvider provider,
                              int& outLoadProgress, std::size_t& outPkgNum )
{
    auto entryFilter = []( const std::vector<std::string_view>& fileEntries ) {
        std::vector<std::string_view> vFilenames;
        if ( fileEntries.empty() == false )
        {
            auto cur = fileEntries.begin();
            cur++;  // skip first entry since it's the index file's name
            for ( ; cur != fileEntries.end(); cur++ )
            {
                if ( cur->find( ".pkg" ) != std::string::npos )
                {
                    vFilenames.push_back( *cur );
                }
            }
        }
        return vFilenames;
    };

    this->m_bIsBusy = true;

    auto [bIndexRead, vIndexData] = ReadFileToBuffer( indexPath );

    if ( bIndexRead == false )
    {
        this->SetErrorQstr( tr( "Could not read index file" ) );

        this->m_bIsBusy = false;
        return false;
    }

    DynamicIndexFileFactory f( indexPath, vIndexData );
    auto pIndex = f.GetPkgIndexOwnership();

    if ( !pIndex )
    {
        this->SetErrorQstr( tr( "Could not detect index's game provider" ) );

        this->m_bIsBusy = false;
        return false;
    }

    auto fileEntries = entryFilter( pIndex->GetFilenames() );
    fs::path indexParentPath = indexPath.parent_path();

    outPkgNum = fileEntries.size();

    for ( auto&& entryFilename : fileEntries )
    {
        fs::path fullEntryPath = indexParentPath;
        fullEntryPath /= entryFilename;

        if ( this->LoadPackage( fullEntryPath, provider, false ) == false )
        {
            this->m_bGenerated = false;
            this->ResetModel();
            return false;
        }

        outLoadProgress++;
    }

    this->m_CurFileProps.SetIndexFileProperties( provider, this->m_PkgFiles );

    this->m_bForceSort = true;
    this->sort( PFS_FileNameColumn );

    this->m_CurrentParentPath = indexParentPath;
    this->m_bIsIndexLoaded = true;

    this->m_bIsBusy = false;

    return true;
}

void PkgFileModel::ResetModel()
{
    this->beginResetModel();

    this->m_DirectoryNodes.clear();
    this->m_PkgFiles.clear();

    this->m_RootNode.FreeChildren();

    this->m_SelectedNodes.clear();
    this->m_CurrentParentPath.clear();

    this->endResetModel();

    this->m_bForceSort = true;

    this->m_bGenerated = false;
    this->m_bIsBusy = false;
    this->m_bIsIndexLoaded = false;
}

Qt::DropActions PkgFileModel::supportedDropActions() const
{
    return Qt::CopyAction;
}

Qt::DropActions PkgFileModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

// TODO: drag files from uncso2 to desktop
QStringList PkgFileModel::mimeTypes() const
{
    QStringList types;

    types << QStringLiteral( "text/uri-list" ) << QStringLiteral( "text/plain" )
          << QStringLiteral( "text/x-moz-url" );

    types << QStringLiteral( "application/octet-stream" );

    return types;
}

QMimeData* PkgFileModel::mimeData( const QModelIndexList& indexes ) const
{
    QMimeData* mimeData = new QMimeData;
    QByteArray data;

    QDataStream stream( &data, QIODevice::WriteOnly );
    QList<ArchiveBaseNode*> nodes;

    for ( const auto& index : indexes )
    {
        ArchiveBaseNode* node = IndexToGenericNode( index );
        Q_ASSERT( node != nullptr );

        if ( nodes.contains( node ) == false )
        {
            nodes << node;
        }
    }

    for ( auto&& node : nodes )
    {
        stream << reinterpret_cast<qlonglong>( node );
    }

    mimeData->setData( QStringLiteral( "application/octet-stream" ), data );

    return mimeData;
}

void PkgFileModel::CreateChildren(
    const std::vector<uc2::PkgFile::entryptr_t>& pEntries,
    const fs::path& pkgPath )
{
    auto pParent = &this->m_RootNode;

    // A PKG file has files from only one directory
    // But a directory might come from multiple PKG files
    const fs::path filePath = pEntries[0]->GetFilePath();

    auto pathBegin = filePath.begin();
    auto pathEnd = filePath.end();
    auto pathLast = --pathEnd;

    // skip root directory
    size_t iCurIndex = 1;
    pathBegin++;

    for ( auto it = pathBegin; it != pathLast; it++ )
    {
        const fs::path subPath = *it;
        std::string szPathName = PathStrFromIterators( pathBegin, it );

        auto search = this->m_DirectoryNodes[szPathName];

        if ( search != nullptr )
        {
            pParent = search;
        }
        else
        {
            auto pNewParent = new ArchiveDirectoryNode( szPathName, pParent );
            this->m_DirectoryNodes[szPathName] = pNewParent;
            pParent->AddChild( pNewParent );
            pParent = pNewParent;
        }

        iCurIndex++;
    }

    for ( auto&& pEntry : pEntries )
    {
        pParent->AddChild(
            new ArchiveFileNode( pkgPath, pEntry.get(), pParent ) );
    }
}

void PkgFileModel::UpdateNodeChildren( const QModelIndex& modelIndex,
                                       const QVariant& value )
{
    Q_ASSERT( this->hasChildren( modelIndex ) == true );

    for ( int i = 0; i < this->rowCount( modelIndex ); i++ )
    {
        this->setData( this->index( i, 0, modelIndex ), value,
                       Qt::CheckStateRole );
    }
}

bool PkgFileModel::IsIndexFileNode( const QModelIndex& index ) const noexcept
{
    auto pFileNode = IndexToFileNode( index );
    return pFileNode != nullptr;
}

void PkgFileModel::SetError( std::string_view errView )
{
    this->m_szLastError = errView.data();
}

void PkgFileModel::SetErrorQstr( const QString& err )
{
    this->m_szLastError = err;
}
