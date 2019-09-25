#pragma once

#include <gsl/gsl>

class QLabel;
class QProgressBar;
class QString;

class StatusWidget
{
public:
    StatusWidget( gsl::not_null<QLabel*> pLabel,
                  gsl::not_null<QProgressBar*> pProgressBar );

public:
    void SetLabel( const QString& szNewLabel );
    void SetMargins( int iMinProgress, int iMaxProgress );
    void SetProgressNum( int iNewProgress );

    void SetVisible( bool bVisible );

private:
    gsl::not_null<QLabel*> m_pLabel;
    gsl::not_null<QProgressBar*> m_pProgressBar;
};