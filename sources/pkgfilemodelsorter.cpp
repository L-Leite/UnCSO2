#include "pkgfilemodelsorter.hpp"

#include <gsl/gsl>

#include "archivefilenode.hpp"

PkgFileModelSorter::PkgFileModelSorter( int column ) : m_iSortColumn( column )
{
}

bool PkgFileModelSorter::CompareNodes( const ArchiveBaseNode* l,
                                       const ArchiveBaseNode* r ) const
{
    Q_ASSERT( l != nullptr );
    Q_ASSERT( r != nullptr );

    // Directories go first
    bool left = l->IsDirectory();
    bool right = r->IsDirectory();

    if ( left ^ right )
    {
        return left;
    }

    switch ( this->m_iSortColumn )
    {
        case PFS_FileNameColumn:
        {
            return this->CompareByName( l, r );
        }
        case PFS_TypeColumn:
        {
            return this->CompareByType( l, r );
        }
        case PFS_SizeColumn:
        {
            return this->CompareBySize( l, r );
        }
        case PFS_OwnerPkgColumn:
        {
            return this->CompareByOwnerPkg( l, r );
        }
    }

    Q_ASSERT( false );
    return false;
}

bool PkgFileModelSorter::CompareByName( const ArchiveBaseNode* l,
                                        const ArchiveBaseNode* r ) const
{
    const fs::path& leftFilename = l->GetPath().filename();
    const fs::path& rightFilename = r->GetPath().filename();

    return leftFilename.compare( rightFilename ) < 0;
}

bool PkgFileModelSorter::CompareByType( const ArchiveBaseNode* l,
                                        const ArchiveBaseNode* r ) const
{
    const fs::path& leftType = l->GetPath().extension();
    const fs::path& rightType = r->GetPath().extension();

    int compare = leftType.compare( rightType );

    // Fall back to name ordering if the files are of the same type
    if ( compare == 0 )
    {
        return this->CompareByName( l, r );
    }

    return compare < 0;
}

bool PkgFileModelSorter::CompareBySize( const ArchiveBaseNode* l,
                                        const ArchiveBaseNode* r ) const
{
    int64_t sizeDifference =
        gsl::narrow_cast<int64_t>( l->GetDecryptedSize() ) -
        gsl::narrow_cast<int64_t>( r->GetDecryptedSize() );

    // Fall back to name ordering if the files are of the same size
    if ( sizeDifference == 0 )
    {
        return this->CompareByName( l, r );
    }

    return sizeDifference < 0;
}

bool PkgFileModelSorter::CompareByOwnerPkg( const ArchiveBaseNode* l,
                                            const ArchiveBaseNode* r ) const
{
    // Fall back to name ordering if one of the nodes is a directory
    if ( l->IsDirectory() == true || r->IsDirectory() == true )
    {
        return this->CompareByName( l, r );
    }

    auto pLeftFile = static_cast<const ArchiveFileNode*>( l );
    auto pRightFile = static_cast<const ArchiveFileNode*>( r );

    int compare = pLeftFile->GetOwnerPkgFilename().compare(
        pRightFile->GetOwnerPkgFilename() );

    // Fall back to name ordering if the files are of the same owner
    if ( compare == 0 )
    {
        return this->CompareByName( l, r );
    }

    return compare < 0;
}
