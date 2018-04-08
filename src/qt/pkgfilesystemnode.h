#ifndef PKGFILESYSTEMNODE_H
#define PKGFILESYSTEMNODE_H

#pragma once

#include <QIcon>
#include <QList>
#include <QVariant>

#include "pkgfilesystem.h"

class CPkgFileSystemNode
{
public:
	explicit CPkgFileSystemNode( const std::filesystem::path& filePath = std::filesystem::path(), const std::filesystem::path& filenamePath = std::filesystem::path(),
		CCSO2PkgEntry* pPkgEntry = Q_NULLPTR, CPkgFileSystemNode *pParentNode = Q_NULLPTR );
	~CPkgFileSystemNode();

	QVector<CPkgFileSystemNode*>& GetChildren();	  
	CPkgFileSystemNode* GetChildContaining( const std::filesystem::path& path );			

	QVariant GetData( int column ) const;
	int GetRow() const;
	CPkgFileSystemNode *GetParentNode();

	QString GetFormattedSize( bool bUncompressedSize = false ) const;
	static QString GetFormattedSize( qint64 bytes );

	inline int GetLocationOf( CPkgFileSystemNode* pChild ) { return m_childNodes.indexOf( pChild ); }
		  	
	inline CCSO2PkgEntry* GetPkgEntry() { return m_pPkgEntry; }

	inline const std::filesystem::path& GetFilePath() const { return m_filePath; }

	inline const QString GetFileName() const { return m_szFileName; }
	inline size_t GetFileNameHash() const { return m_iFileNameHash; }
	inline const QString GetFileType() const { if ( m_bIsDirectory ) return "Directory"; return m_szFileExtension; }

	inline qint64 GetFilePackedSize() const { if ( m_bIsDirectory ) return NULL; return m_pPkgEntry->GetPackedSize(); }
	inline qint64 GetFileUnpackedSize() const { if ( m_bIsDirectory ) return NULL; return m_pPkgEntry->GetUnpackedSize(); }

	inline bool IsDirectory() const { return m_bIsDirectory; }	

private:
	QVector<CPkgFileSystemNode*> m_childNodes;
	CPkgFileSystemNode *m_pParentNode;

	std::filesystem::path m_filePath;
	QString m_szFileName;
	size_t m_iFileNameHash;
	QString m_szFileExtension;	   

	CCSO2PkgEntry* m_pPkgEntry;

	bool m_bIsDirectory;			
};

#endif // PKGFILESYSTEMNODE_H
