#include "stdafx.h"
#include "aboutdialog.h"

CAboutDialog::CAboutDialog( QWidget* pParent /*= Q_NULLPTR*/ ) : QDialog( pParent )
{
	ui.setupUi( this );
	ui.closeButton->setDefault( true );

	connect( ui.closeButton, SIGNAL( released() ), this, SLOT( close() ) );
}