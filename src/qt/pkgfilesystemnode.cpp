#include "stdafx.h"
#include "pkgfilesystemnode.h"

#include <QLocale>
#include <QStringList>


CPkgFileSystemNode::CPkgFileSystemNode( const std::filesystem::path& filePath /*= std::filesystem::path()*/, const std::filesystem::path& filenamePath /*= std::filesystem::path()*/,
	CCSO2PkgEntry* pPkgEntry /*= Q_NULLPTR*/, CPkgFileSystemNode *pParentNode /*= Q_NULLPTR*/ )
{
	m_filePath = filePath;
	m_szFileName = filenamePath.string().c_str();
	m_iFileNameHash = std::hash<std::string>{}(filenamePath.string());
	m_szFileExtension = filenamePath.extension().string().c_str();
	m_pPkgEntry = pPkgEntry;
	m_bIsDirectory = !filenamePath.has_extension();
	m_pParentNode = pParentNode;
}

CPkgFileSystemNode::~CPkgFileSystemNode()
{
	qDeleteAll( m_childNodes );
	delete m_pPkgEntry;
}

QVector<CPkgFileSystemNode*>& CPkgFileSystemNode::GetChildren()
{
	return m_childNodes;
}

CPkgFileSystemNode* CPkgFileSystemNode::GetChildContaining( const std::filesystem::path& path )
{ 				
	size_t iPathHash = std::hash<std::string>{}(path.string());

	for ( const auto& child : m_childNodes )
	{												
		if ( child->m_iFileNameHash == iPathHash )
			return child;
	}

	return Q_NULLPTR;
}

QVariant CPkgFileSystemNode::GetData( int column ) const
{
	switch ( column )
	{
		case 0:
			return GetFileName();
		case 1:
			return m_bIsDirectory ? "Directory" : QString( "%1 file" ).arg( m_szFileExtension );
		case 2:
			return GetFormattedSize();
		case 3:				  
			return GetFormattedSize( true );
	}

	return QVariant();
}

CPkgFileSystemNode *CPkgFileSystemNode::GetParentNode()
{
	return m_pParentNode;
}

QString CPkgFileSystemNode::GetFormattedSize( bool bUncompressedSize /*= false*/ ) const
{										   
	if ( IsDirectory() )
		return QString( "" );

	return GetFormattedSize( bUncompressedSize ? GetFileUnpackedSize() : GetFilePackedSize() );
}

QString CPkgFileSystemNode::GetFormattedSize( qint64 bytes )
{
	return QLocale::system().formattedDataSize( bytes );
}

int CPkgFileSystemNode::GetRow() const
{
	if ( m_pParentNode )
		return m_pParentNode->m_childNodes.indexOf( const_cast<CPkgFileSystemNode*>(this) );

	return 0;
}
