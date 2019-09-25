#include "widgets/statuswidget.hpp"

#include <QLabel>
#include <QProgressBar>

StatusWidget::StatusWidget( gsl::not_null<QLabel*> pLabel,
                            gsl::not_null<QProgressBar*> pProgressBar )
    : m_pLabel( pLabel ), m_pProgressBar( pProgressBar )
{
}

void StatusWidget::SetLabel( const QString& szNewLabel )
{
    this->m_pLabel->setText( szNewLabel );
}

void StatusWidget::SetMargins( int iMinProgress, int iMaxProgress )
{
    this->m_pProgressBar->setMinimum( iMinProgress );
    this->m_pProgressBar->setMaximum( iMaxProgress );
}

void StatusWidget::SetProgressNum( int iNewProgress )
{
    this->m_pProgressBar->setValue( iNewProgress );
}

void StatusWidget::SetVisible( bool bVisible )
{
    this->m_pLabel->setVisible( bVisible );
    this->m_pProgressBar->setVisible( bVisible );
}