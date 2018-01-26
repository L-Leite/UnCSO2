#include "stdafx.h"
#include "uncso2app.h"

CUnCSO2App::CUnCSO2App( int& argc, char** argv ) : QApplication( argc, argv )
{
	m_pMainWindow = new CMainWindow();
	m_pMainWindow->show();
}
