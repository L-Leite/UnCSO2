#pragma once

#include "gitversion.h"
#include <QString>

#define APP_VERSION "1.4"

QString GetCurrentCommit();
QString GetAppVersion();
