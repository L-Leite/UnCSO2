#include "archivebasenode.hpp"

ArchiveBaseNode::ArchiveBaseNode(
    const fs::path& itemPath, ArchiveDirectoryNode* pParentNode /*= nullptr*/ )
    : m_pParentNode( pParentNode ), m_NodePath( itemPath )
{
    this->m_iPathHash =
        std::hash<std::string>{}( this->m_NodePath.generic_string() );
}

ArchiveBaseNode::~ArchiveBaseNode() {}

ArchiveDirectoryNode* ArchiveBaseNode::GetParentNode() const
{
    return this->m_pParentNode;
}

void ArchiveBaseNode::SetParentNode( ArchiveDirectoryNode* pNewParent )
{
    this->m_pParentNode = pNewParent;
}
