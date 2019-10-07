#include "archivedirectorynode.hpp"

#include <gsl/gsl>

#include "archivefilenode.hpp"
#include "pkgfilesystemshared.hpp"

ArchiveDirectoryNode::ArchiveDirectoryNode(
    const fs::path& directoryPath,
    ArchiveDirectoryNode* pParentNode /*= nullptr*/ )
    : ArchiveBaseNode( directoryPath, pParentNode )
{
}

ArchiveDirectoryNode::~ArchiveDirectoryNode()
{
    for ( auto&& child : this->m_vChildNodes )
    {
        delete child;
    }
}

void ArchiveDirectoryNode::AddChild( ArchiveBaseNode* pNewChild )
{
    return this->m_vChildNodes.push_back( pNewChild );
}

ArchiveBaseNode* ArchiveDirectoryNode::GetChild( size_t index ) const
{
    return this->m_vChildNodes[index];
}

size_t ArchiveDirectoryNode::GetNumOfChildren() const
{
    return this->m_vChildNodes.size();
}

bool ArchiveDirectoryNode::HasChildren() const
{
    return this->m_vChildNodes.empty() == false;
}

void ArchiveDirectoryNode::FreeChildren()
{
    for ( auto&& child : this->m_vChildNodes )
    {
        auto pDirChild = BaseToDirectoryNode( child );

        if ( pDirChild != nullptr )
        {
            pDirChild->FreeChildren();
        }

        delete child;
    }

    this->m_vChildNodes.clear();
}

ArchiveBaseNode* ArchiveDirectoryNode::GetChildContaining(
    const fs::path& path ) const
{
    size_t iPathHash = std::hash<std::string>{}( path.generic_string() );

    for ( const auto& child : this->m_vChildNodes )
    {
        if ( child->GetPathHash() == iPathHash )
            return child;
    }

    return nullptr;
}

int ArchiveDirectoryNode::GetLocationOf( const ArchiveBaseNode* pChild ) const
{
    if ( this->m_vChildNodes.empty() == false )
    {
        auto found = std::find( this->m_vChildNodes.begin(),
                                this->m_vChildNodes.end(), pChild );

        if ( found != this->m_vChildNodes.end() )
        {
            return gsl::narrow_cast<int>(
                std::distance( this->m_vChildNodes.begin(), found ) );
        }
    }

    return std::numeric_limits<int>::max();
}

QVariant ArchiveDirectoryNode::GetData( int column )
{
    switch ( column )
    {
        case PFS_FileNameColumn:
            return QString::fromStdString(
                this->GetPath().filename().generic_string() );
        case PFS_TypeColumn:
            return tr( "Directory" );
        case PFS_SizeColumn:
            return {};
        case PFS_OwnerPkgColumn:
            return {};
    }

    return QVariant();
}

int ArchiveDirectoryNode::GetRow() const
{
    if ( this->m_pParentNode )
    {
        return this->m_pParentNode->GetLocationOf(
            const_cast<ArchiveDirectoryNode*>( this ) );
    }

    return 0;
}

uint64_t ArchiveDirectoryNode::GetDecryptedSize() const
{
    return 0;
}

bool ArchiveDirectoryNode::IsDirectory() const
{
    return true;
}

std::set<std::string_view> ArchiveDirectoryNode::GetChildrenPkgOwners() const
{
    std::set<std::string_view> vChildrenPkgOwners;

    if ( this->HasChildren() == true )
    {
        for ( auto&& child : this->m_vChildNodes )
        {
            if ( child->IsDirectory() == false )
            {
                auto pFileChild = static_cast<ArchiveFileNode*>( child );
                vChildrenPkgOwners.insert( pFileChild->GetOwnerPkgFilename() );
            }
        }
    }

    return vChildrenPkgOwners;
}
