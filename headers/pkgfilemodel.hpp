#pragma once

#include <QAbstractItemModel>
#include <QItemSelection>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QVariant>

#include <filesystem>
#include <gsl/gsl>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

#include <uc2/uc2.hpp>

#include "archivedirectorynode.hpp"
#include "fileproperties.hpp"
#include "gamedatainfo.hpp"

class ArchiveBaseNode;
class ArchiveFileNode;

class StatusWidget;

class PkgFileModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    PkgFileModel( QWidget* pParent = nullptr );
    ~PkgFileModel();

    QVariant data( const QModelIndex& index, int role ) const override;
    bool setData( const QModelIndex& index, const QVariant& value,
                  int role = Qt::EditRole ) override;

    QVariant headerData( int section, Qt::Orientation orientation,
                         int role = Qt::DisplayRole ) const override;

    QModelIndex index(
        int row, int column,
        const QModelIndex& parent = QModelIndex() ) const override;
    QModelIndex index( const ArchiveBaseNode* node, int column = 0 ) const;

    QModelIndex parent( const QModelIndex& index ) const override;

    int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
    int columnCount( const QModelIndex& parent = QModelIndex() ) const override;

    Qt::ItemFlags flags( const QModelIndex& index ) const override;

    void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override;
    void sortChildren( int column, ArchiveDirectoryNode* pIndexNode );

    void OnSelectionChanged( const QItemSelection& selected,
                             const QItemSelection& deselected );

    ArchiveBaseNode* GetNode( const QModelIndex& index ) const;

    bool LoadPackage( const fs::path& pkgPath, GameProvider provider,
                      bool bIndependentLoad = true );
    bool LoadIndex( const fs::path& indexPath, GameProvider provider,
                    int& outLoadProgress, std::size_t& outPkgNum );

    bool IsIndexFileNode( const QModelIndex& index ) const noexcept;

    void ResetModel();

    //
    // qt drag and drop overrides
    //
    virtual QStringList mimeTypes() const override;
    virtual QMimeData* mimeData(
        const QModelIndexList& indexes ) const override;

    virtual Qt::DropActions supportedDropActions() const override;
    virtual Qt::DropActions supportedDragActions() const override;

    inline std::size_t GetSelectedNodesCount() const;
    inline const FileProperties& GetCurrentFileProperties() const;
    inline const fs::path& GetCurrentParentPath() const;
    inline const std::unordered_map<std::size_t, uc2::PkgFile::ptr_t>&
    GetLoadedPkgFiles() const;
    inline std::vector<ArchiveBaseNode*> GetCopyOfSelectedNodes() const;

    inline const QString& GetError() const noexcept;

    inline bool IsGenerated() const noexcept;
    inline bool IsBusy() const noexcept;
    inline bool IsIndexLoaded() const noexcept;

private:
    void CreateChildren( const std::vector<uc2::PkgFile::entryptr_t>& pEntries,
                         const fs::path& pkgPath );
    void UpdateNodeChildren( const QModelIndex& index, const QVariant& value );

    inline int translateVisibleLocation( ArchiveDirectoryNode* parent,
                                         int row ) const;

    inline bool ShouldOrderColumn( int column, Qt::SortOrder order ) const;

    std::vector<std::pair<ArchiveBaseNode*, int>> GrabOldNodes(
        QModelIndexList& oldList );
    QModelIndexList CreateNewNodeList(
        std::vector<std::pair<ArchiveBaseNode*, int>>& oldNodes );

    void SetError( std::string_view errView );
    void SetErrorQstr( const QString& err );

private:
    std::unordered_map<std::string, ArchiveDirectoryNode*> m_DirectoryNodes;
    std::unordered_map<std::size_t, uc2::PkgFile::ptr_t> m_PkgFiles;

    ArchiveDirectoryNode m_RootNode;

    std::unordered_set<ArchiveBaseNode*> m_SelectedNodes;

    fs::path m_CurrentParentPath;
    FileProperties m_CurFileProps;

    QString m_szLastError;

    int m_iSortColumn;
    Qt::SortOrder m_SortOrder;
    bool m_bForceSort;

    bool m_bGenerated;
    bool m_bIsBusy;
    bool m_bIsIndexLoaded;
};

inline std::size_t PkgFileModel::GetSelectedNodesCount() const
{
    return this->m_SelectedNodes.size();
}

inline const FileProperties& PkgFileModel::GetCurrentFileProperties() const
{
    return this->m_CurFileProps;
}

inline const fs::path& PkgFileModel::GetCurrentParentPath() const
{
    return this->m_CurrentParentPath;
}

inline const std::unordered_map<std::size_t, uc2::PkgFile::ptr_t>&
PkgFileModel::GetLoadedPkgFiles() const
{
    return this->m_PkgFiles;
}

inline std::vector<ArchiveBaseNode*> PkgFileModel::GetCopyOfSelectedNodes()
    const
{
    return { this->m_SelectedNodes.begin(), this->m_SelectedNodes.end() };
}

inline const QString& PkgFileModel::GetError() const noexcept
{
    return this->m_szLastError;
}

inline bool PkgFileModel::IsGenerated() const noexcept
{
    return this->m_bGenerated;
}

inline bool PkgFileModel::IsBusy() const noexcept
{
    return this->m_bIsBusy;
}

inline bool PkgFileModel::IsIndexLoaded() const noexcept
{
    return this->m_bIsIndexLoaded;
}

inline int PkgFileModel::translateVisibleLocation( ArchiveDirectoryNode* parent,
                                                   int row ) const
{
    Q_ASSERT( parent != nullptr );
    Q_ASSERT( parent->IsDirectory() == true );
    Q_ASSERT( parent->HasChildren() == true );

    if ( this->m_SortOrder == Qt::DescendingOrder )
    {
        return gsl::narrow_cast<int>( parent->GetNumOfChildren() ) - row - 1;
    }
    else
    {
        return row;
    }
}

inline bool PkgFileModel::ShouldOrderColumn( int column,
                                             Qt::SortOrder order ) const
{
    return !( this->m_iSortColumn == column && this->m_SortOrder != order &&
              this->m_bForceSort == false );
}
