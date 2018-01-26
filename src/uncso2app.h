#pragma once

#include <QApplication>
#include "qt/mainwindow.h"

class CUnCSO2App : public QApplication
{
	Q_OBJECT

public:
	CUnCSO2App( int& argc, char** argv );

private:
	CMainWindow* m_pMainWindow;
};
