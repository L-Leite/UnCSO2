#ifndef PKGFILESYSTEMMODEL_H
#define PKGFILESYSTEMMODEL_H

#pragma once

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QSet>
#include <QVariant>

class CCSO2PkgEntry;
class CMainWindow;
class CPkgFileSystemNode;

class CPkgFileSystemModel : public QAbstractItemModel
{
	Q_OBJECT

public:	   
	explicit CPkgFileSystemModel( CMainWindow* pParent = Q_NULLPTR );
	~CPkgFileSystemModel();

	QVariant data( const QModelIndex &index, int role ) const Q_DECL_OVERRIDE;
	bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole ) Q_DECL_OVERRIDE;

	QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;

	QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;
	QModelIndex index( const CPkgFileSystemNode *node, int column = 0) const;

	QModelIndex parent( const QModelIndex &index ) const Q_DECL_OVERRIDE;

	int rowCount( const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;
	int columnCount( const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;

	Qt::ItemFlags flags( const QModelIndex &index ) const Q_DECL_OVERRIDE;

	void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) Q_DECL_OVERRIDE;
	void sortChildren( int column, const QModelIndex &parent );

	CPkgFileSystemNode* GetNode( const QModelIndex &index ) const; 	

	bool GenerateFileSystem();
	void CleanFileSystem();
	void CreateChild( CCSO2PkgEntry* pPkgEntry );
	bool ExtractCheckedNodes();		

	inline bool WasGenerated() { return m_bGenerated; }

	enum
	{
		FileNameColumn = 0,
		TypeColumn,
		PackedSizeColumn,
		UnpackedSizeColumn,
		NumColumns
	};

private:	
	void UpdateNodeChildren( const QModelIndex &index, const QVariant &value );

	static QVector<CPkgFileSystemNode*> m_DirectoryNodes;

	CMainWindow* m_pParentWindow;

	CPkgFileSystemNode* m_pRoot;
	QSet<QPersistentModelIndex> m_CheckedIndexes;		
	
	int m_iSortColumn;
	Qt::SortOrder m_SortOrder;	 	
	bool m_bForceSort;

	bool m_bGenerated;
};

#endif // PKGFILESYSTEMMODEL_H
