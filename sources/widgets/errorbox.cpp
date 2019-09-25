#include "widgets/errorbox.hpp"

#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTextBrowser>
#include <QToolButton>

ErrorBoxWidget::ErrorBoxWidget( gsl::not_null<QFrame*> frame,
                                gsl::not_null<QTextEdit*> msgBox,
                                gsl::not_null<QToolButton*> closeBtn )
    : m_pFrame( frame ), m_pMsgBox( msgBox ), m_pCloseBtn( closeBtn )
{
    this->m_iFrameMaxHeight = this->m_pFrame->maximumHeight();
    this->m_pFrame->setMaximumHeight( 0 );

    this->connect( this->m_pCloseBtn, &QToolButton::clicked, this,
                   &ErrorBoxWidget::OnCloseMsg );
}

void ErrorBoxWidget::SetMessage( const QString& message,
                                 const QString& errorDetails /*= QString() */ )
{
    auto formattedHtml = tr( "<p>%1." ).arg( message );

    if ( errorDetails.isEmpty() == false )
    {
        formattedHtml.append(
            tr( "<br>Error: <i>%1</i>" ).arg( errorDetails ) );
    }

    formattedHtml.append( tr( "</p>" ) );

    this->m_pMsgBox->setText( formattedHtml );
}

void ErrorBoxWidget::SetVisible( bool bVisible )
{
    auto eff = new QGraphicsOpacityEffect( this->m_pFrame->parent() );
    this->m_pFrame->setGraphicsEffect( eff );

    auto heightAnim = new QPropertyAnimation( this->m_pFrame, "maximumHeight" );
    auto opacityAnim = new QPropertyAnimation( eff, "opacity" );
    heightAnim->setDuration( 250 );
    opacityAnim->setDuration( 125 );

    if ( bVisible == true )
    {
        heightAnim->setStartValue( 0 );
        heightAnim->setEndValue( m_iFrameMaxHeight );
        heightAnim->setEasingCurve( QEasingCurve::InBack );

        opacityAnim->setStartValue( 0 );
        opacityAnim->setEndValue( 1 );
        opacityAnim->setEasingCurve( QEasingCurve::InBack );
    }
    else
    {
        heightAnim->setStartValue( m_iFrameMaxHeight );
        heightAnim->setEndValue( 0 );
        heightAnim->setEasingCurve( QEasingCurve::OutBack );

        opacityAnim->setStartValue( 1 );
        opacityAnim->setEndValue( 0 );
        opacityAnim->setEasingCurve( QEasingCurve::OutBack );
    }

    heightAnim->start( QPropertyAnimation::DeleteWhenStopped );
    opacityAnim->start( QPropertyAnimation::DeleteWhenStopped );
}

void ErrorBoxWidget::OnCloseMsg()
{
    this->SetVisible( false );
}
