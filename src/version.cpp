#include "stdafx.h"

QString GetCurrentCommit()
{	
	return GIT_BRANCH "_" GIT_VERSION;
}

QString GetAppVersion()
{				 
	return APP_VERSION;
}
