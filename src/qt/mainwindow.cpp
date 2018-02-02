#include "stdafx.h"
#include "mainwindow.h"

#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>

#include "aboutdialog.h"
#include "pkgfilesystemmodel.h"

#define STATUS_BAR_PRINT( message, ...) GetStatusBar()->showMessage( QString( "%1: " message ).arg( __func__, ##__VA_ARGS__ ) )

CMainWindow::CMainWindow( QWidget* pParent )
	: QMainWindow( pParent )
{
	setWindowTitle( "UnCSO2 Commit " + GetCurrentCommit() );

	ui.setupUi( this );
	ui.unpackButton->setDisabled( true );

	m_pPkgItemModel = new CPkgFileSystemModel( this );			

	connect( ui.actionOpen_folder, SIGNAL( triggered() ), this, SLOT( OnFileOpen() ) );
	connect( ui.actionExit, SIGNAL( triggered() ), this, SLOT( OnExit() ) );	
	connect( ui.actionDecrypt_encrypted_files, SIGNAL( toggled( bool ) ), this, SLOT( OnDecryptToggle( bool ) ) );
	connect( ui.actionRename_encrypted_files, SIGNAL( toggled( bool ) ), this, SLOT( OnRenameToggle( bool ) ) );
	connect( ui.unpackButton, SIGNAL( released() ), this, SLOT( OnUnpackClick() ) );
	connect( ui.actionAbout, SIGNAL( triggered() ), this, SLOT( OnAbout() ) );
	connect( ui.actionAboutQt, &QAction::triggered, this, &QApplication::aboutQt );

	STATUS_BAR_PRINT( "initialized" );
}

CMainWindow::~CMainWindow()
{
	delete m_pPkgItemModel;
}
  
void CMainWindow::OnFileOpen()
{ 			 
	DBG_PRINTF( "triggered\n" );

	if ( m_pPkgItemModel->WasGenerated() )
	{
		DBG_PRINTF( "Cleaning up PkgItemModel...\n" );
		m_pPkgItemModel->CleanFileSystem();
		ui.pkgFileSystemView->setModel( nullptr );	  		
	}

	STATUS_BAR_PRINT( "Selecting data directory..." );
	g_PkgDataPath = QFileDialog::getExistingDirectory( this, "Select the data directory", QDir::currentPath() ).toStdString();

	if ( !g_PkgDataPath.empty() )
	{
		std::string szDataPath = g_PkgDataPath.string();

		if ( szDataPath[szDataPath.length() - 1] != '\\' )
		{
			szDataPath.append( 1, '\\' );
			g_PkgDataPath = szDataPath;
		}

		DBG_WPRINTF( L"New g_PkgDataPath: %s\n", g_PkgDataPath.c_str() );
		STATUS_BAR_PRINT( "New g_PkgDataPath: %2", g_PkgDataPath.string().c_str() );

		if ( m_pPkgItemModel->GenerateFileSystem() )
		{									 
			DBG_PRINTF( "Generated items successfully\n" );	
			STATUS_BAR_PRINT( "Generated items successfully" );
			ui.pkgFileSystemView->setModel( m_pPkgItemModel );
			ui.pkgFileSystemView->setColumnWidth( CPkgFileSystemModel::FileNameColumn, QLabel( "This is the very length of the file name." ).sizeHint().width() );
			ui.unpackButton->setDisabled( false );
			ui.dataPathLabel->setText( QString( "Data path: %1" ).arg( g_PkgDataPath.string().c_str() ) );	  
		}
		else
		{
			DBG_PRINTF( "Failed to generate file system items\n" );
			STATUS_BAR_PRINT( "Failed to generate file system items" );
			ui.unpackButton->setDisabled( true );
			ui.dataPathLabel->setText( QString( "No data path selected" ) );
		}		
	}
	else
	{
		STATUS_BAR_PRINT( "g_PkgDataPath is null!" );
		DBG_PRINTF( "g_PkgDataPath is null!\n" );		
		QMessageBox msgBox( "Error", "Please choose a valid path.", QMessageBox::Warning, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
		msgBox.exec();
	}
}

void CMainWindow::OnExit()
{ 
	DBG_PRINTF( "triggered\n" );
	std::exit( 0 );
}

void CMainWindow::OnDecryptToggle( bool bChecked )
{
	DBG_PRINTF( "toggled\n" );
	m_pPkgItemModel->SetDecryptEncFiles( bChecked );
}

void CMainWindow::OnRenameToggle( bool bChecked )
{
	DBG_PRINTF( "toggled\n" );
	m_pPkgItemModel->SetRenameEncFiles( bChecked );
}

void CMainWindow::OnUnpackClick()
{
	DBG_PRINTF( "triggered\n" );
	g_OutPath = QFileDialog::getExistingDirectory( this, "Select the output directory", g_PkgDataPath.string().c_str() ).toStdString();

	if ( !g_OutPath.empty() )
	{
		std::string szDataPath = g_OutPath.string();

		if ( szDataPath[szDataPath.length() - 1] != '\\' )
		{
			szDataPath.append( 1, '\\' );
			g_OutPath = szDataPath;
		}

		DBG_WPRINTF( L"New g_OutPath: %s\n", g_OutPath.c_str() );
		STATUS_BAR_PRINT( "New g_OutPath: %2", g_OutPath.string().c_str() );

		m_pPkgItemModel->ExtractCheckedNodes();
	}
	else
	{
		STATUS_BAR_PRINT( "g_OutPath: is null!" );
		DBG_PRINTF( "g_OutPath: is null!\n" );
		QMessageBox msgBox( "Error", "Please choose a valid path.", QMessageBox::Warning, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
		msgBox.exec();
	}	  	
}

void CMainWindow::OnAbout()
{
	CAboutDialog( this ).exec();
}
