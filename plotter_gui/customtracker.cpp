#include "customtracker.h"
#include <qwt_picker_machine.h>
#include <qwt_series_data.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include "qwt_picker_machine.h"
#include "qwt_event_pattern.h"
#include <qevent.h>

struct compareX
{
    inline bool operator()( const double x, const QPointF &pos ) const
    {
        return ( x < pos.x() );
    }
};


//! Constructor
PickerTrackerMachine::PickerTrackerMachine():
    QwtPickerMachine( NoSelection )
{
    keypressed = false;
}

//! Transition
QList<QwtPickerMachine::Command> PickerTrackerMachine::transition(
        const QwtEventPattern & eventPattern, const QEvent *event )
{
    QList<QwtPickerMachine::Command> cmdList;

    switch ( event->type() )
    {
        case  QEvent::MouseButtonPress:
        {
            if( eventPattern.mouseMatch( QwtEventPattern::MouseSelect1, static_cast<const QMouseEvent *>( event ) ) )
            {
                qDebug() << "keypressed = true";
                keypressed = true;
                cmdList += Move;
            }
            break;
        }

        case  QEvent::MouseButtonRelease:
        {
            qDebug() << "keypressed = false";
            keypressed = false;
            cmdList += Move;
            break;
        }

        case  QEvent::MouseMove:
        {
            qDebug() << "move " << keypressed;
            if( keypressed ){
                cmdList += Move;
            }
            break;
        }
        case QEvent::Leave:
        {
            keypressed = false;
            break;
        }
        default:
            break;
    }
    return cmdList;
}


CurveTracker::CurveTracker( QWidget *canvas ):
    QwtPlotPicker( canvas )
{
    setTrackerMode( QwtPlotPicker::AlwaysOn );
    setRubberBand( VLineRubberBand );

    PickerTrackerMachine* sm =  new PickerTrackerMachine();
    sm->setState(1);
    this->setStateMachine( sm );

    this->begin();
    this->append( QPoint(0,0));
}

QPointF CurveTracker::actualPosition() const
{
    return _prev_trackerpoint;
}

void CurveTracker::manualMove(const QPointF& plot_pos)
{
    // the difference with move is that it will not emit a signal
    _prev_trackerpoint = plot_pos;
    QwtPicker::move( transform (plot_pos) );
}

void CurveTracker::onExternalRescale()
{
    QwtPlotPicker::move( transform (_prev_trackerpoint) );
}

void CurveTracker::onExternalZoom(const QRectF &)
{
    QwtPlotPicker::move( transform (_prev_trackerpoint) );
}

QwtText CurveTracker::trackerText( const QPoint &pos ) const
{
    return trackerTextF(_prev_trackerpoint);
}

QwtText CurveTracker::trackerTextF( QPointF pos ) const
{
    //qDebug() << "trackerTextF " << pos;

    QwtText trackerText;

    trackerText.setColor( Qt::black );

    QColor c( "#FFFFFF" );
    trackerText.setBorderPen( QPen( c, 2 ) );
    c.setAlpha( 200 );
    trackerText.setBackgroundBrush( c );

    QString info;

    const QwtPlotItemList curves =
            plot()->itemList( QwtPlotItem::Rtti_PlotCurve );

    for ( int i = 0; i < curves.size(); i++ )
    {
        const QString curveInfo = curveInfoAt(
                    static_cast<const QwtPlotCurve *>( curves[i] ), pos );

        if ( !curveInfo.isEmpty() )
        {
            if ( !info.isEmpty() )
                info += "<br>";

            info += curveInfo;
        }
    }

    trackerText.setText( info );
    return trackerText;
}

void CurveTracker::move(const QPoint &pos)
{
    qDebug() << "SM move " << pos;
    _prev_trackerpoint = invTransform(pos);
    QwtPlotPicker::move( pos );
}

QString CurveTracker::curveInfoAt( const QwtPlotCurve *curve, QPointF pos ) const
{
    // qDebug() << "curveInfoAt " << pos;

    const QLineF line = curveLineAt( curve, pos.x() );
    if ( line.isNull() )
        return QString::null;

    double y;

    if( fabs( pos.x() - line.p1().x()) <   fabs( pos.x() - line.p2().x()) )
    {
        y = line.p1().y();
    }
    else{
        y = line.p2().y();
    }

    QString info( "<font color=""%1"">%2</font>" );
    return info.arg( curve->pen().color().name() ).arg( y );
}

QLineF CurveTracker::curveLineAt(
        const QwtPlotCurve *curve, double x ) const
{
    QLineF line;

    if ( curve->dataSize() >= 2 )
    {
        const QRectF br = curve->boundingRect();
        if ( ( br.width() > 0 ) && ( x >= br.left() ) && ( x <= br.right() ) )
        {
            int index = qwtUpperSampleIndex<QPointF>(
                        *curve->data(), x, compareX() );

            if ( index == -1 &&
                 x == curve->sample( curve->dataSize() - 1 ).x() )
            {
                // the last sample is excluded from qwtUpperSampleIndex
                index = curve->dataSize() - 1;
            }

            if ( index > 0 )
            {
                line.setP1( curve->sample( index - 1 ) );
                line.setP2( curve->sample( index ) );
            }
        }
    }

    return line;
}

QRect CurveTracker::trackerRect( const QFont &font ) const
{
    if ( !isActive() )
    {
        return QRect();
    }

    QPoint pos = trackerPosition();
    if ( pos.x() < 0 || pos.y() < 0 )
    {
        pos = transform(_prev_trackerpoint);
    }

    QwtText text = trackerText( pos );
    if ( text.isEmpty() )
        return QRect();

    const QSizeF textSize = text.textSize( font );
    QRect textRect( 0, 0, qCeil( textSize.width() ), qCeil( textSize.height() ) );

    int alignment = Qt::AlignTop | Qt::AlignRight;

    const int margin = 5;

    int x = pos.x();
    if ( alignment & Qt::AlignLeft )
        x -= textRect.width() + margin;
    else if ( alignment & Qt::AlignRight )
        x += margin;

    int y = pos.y();
    if ( alignment & Qt::AlignBottom )
        y += margin;
    else if ( alignment & Qt::AlignTop )
        y -= textRect.height() + margin;

    textRect.moveTopLeft( QPoint( x, y ) );

    const QRect pickRect = pickArea().boundingRect().toRect();

    int right = qMin( textRect.right(), pickRect.right() - margin );
    int bottom = qMin( textRect.bottom(), pickRect.bottom() - margin );
    textRect.moveBottomRight( QPoint( right, bottom ) );

    int left = qMax( textRect.left(), pickRect.left() + margin );
    int top = qMax( textRect.top(), pickRect.top() + margin );
    textRect.moveTopLeft( QPoint( left, top ) );

    return textRect;
}
