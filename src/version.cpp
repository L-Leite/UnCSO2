#include "stdafx.h"

QString GetCurrentCommit()
{	
	return GIT_BRANCH "-" GIT_VERSION;
}
