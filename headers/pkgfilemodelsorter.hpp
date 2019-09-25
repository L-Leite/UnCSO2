#pragma once

#include <QtCore>

#include "pkgfilesystemshared.hpp"

class ArchiveBaseNode;

class PkgFileModelSorter
{
public:
    PkgFileModelSorter( int column );

    bool CompareNodes( const ArchiveBaseNode* l,
                       const ArchiveBaseNode* r ) const;

    inline bool operator()( const ArchiveBaseNode* l,
                            const ArchiveBaseNode* r ) const
    {
        return this->CompareNodes( l, r );
    }

protected:
    bool CompareByName( const ArchiveBaseNode* l,
                        const ArchiveBaseNode* r ) const;
    bool CompareByType( const ArchiveBaseNode* l,
                        const ArchiveBaseNode* r ) const;
    bool CompareBySize( const ArchiveBaseNode* l,
                        const ArchiveBaseNode* r ) const;
    bool CompareByFlags( const ArchiveBaseNode* l,
                         const ArchiveBaseNode* r ) const;
    bool CompareByOwnerPkg( const ArchiveBaseNode* l,
                            const ArchiveBaseNode* r ) const;

private:
    int m_iSortColumn;
};