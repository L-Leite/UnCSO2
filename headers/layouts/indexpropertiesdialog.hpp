#pragma once

#include <QDialog>
#include "ui_indexpropertiesdialog.h"

#include <filesystem>
namespace fs = std::filesystem;

class FileProperties;

class CIndexPropertiesDialog : public QDialog, public Ui::IndexPropertiesDialog
{
    Q_OBJECT

public:
    CIndexPropertiesDialog( const FileProperties& props,
                            std::string_view indexFilename,
                            QWidget* pParent = nullptr );
};
