#include "stdafx.h"
#include "uncso2app.h"

std::filesystem::path g_PkgDataPath;
std::filesystem::path g_OutPath;

bool CreateDebugConsole()
{
	BOOL result = AllocConsole();

	if ( !result )
		return false;

	freopen( "CONIN$", "r", stdin );
	freopen( "CONOUT$", "w", stdout );
	freopen( "CONOUT$", "w", stderr );

	SetConsoleTitleW( L"Debug Console" );

	return true;
}

int main( int argc, char *argv[] )
{
#ifndef _DEBUG
	bool bCreateConsole = false;

	for ( int i = 0; i < argc; i++ )
	{ 
		if ( !strcmp( argv[i], "-dbgconsole" ) )
			bCreateConsole = true;
	}
	
	if ( bCreateConsole )
#endif
		CreateDebugConsole();

	DBG_PRINTF( "Console created\n" );

	CUnCSO2App app( argc, argv );
	return app.exec();
}
