#pragma once

#pragma once

#include <gsl/gsl>

#include <QObject>

class QFrame;
class QTextEdit;
class QToolButton;

class ErrorBoxWidget : public QObject
{
    Q_OBJECT
public:
    ErrorBoxWidget( gsl::not_null<QFrame*> frame,
                    gsl::not_null<QTextEdit*> msgBox,
                    gsl::not_null<QToolButton*> closeBtn );

public:
    void SetMessage( const QString& message,
                     const QString& errorDetails = QString() );
    void SetVisible( bool bVisible );

private slots:
    void OnCloseMsg();

private:
    gsl::not_null<QFrame*> m_pFrame;
    gsl::not_null<QTextEdit*> m_pMsgBox;
    gsl::not_null<QToolButton*> m_pCloseBtn;

    int m_iFrameMaxHeight;
};