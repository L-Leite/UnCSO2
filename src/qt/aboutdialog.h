#pragma once

#include <QDialog>
#include "ui_aboutdialog.h"

class CAboutDialog : public QDialog
{
	Q_OBJECT

public:
	CAboutDialog( QWidget* pParent = Q_NULLPTR );

private:
	Ui::AboutDialog ui;
};
