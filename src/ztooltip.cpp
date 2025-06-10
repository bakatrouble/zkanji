/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QToolTip>
#include <QtEvents>
#include <QApplication>
#include <QStyle>
#include <QLayout>
#include <QStylePainter>
#include <QStyleOption>
#include <QDesktopWidget>
#include <QScreen>
#include <qwindow.h>
#include <qdatetime.h>
#include "ztooltip.h"
#include "zkanjimain.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif
#undef max

//#ifdef _DEBUG
//#include <QDebug>
//#endif
//-------------------------------------------------------------

// Number of milliseconds it should take for the tooltip to show.
static const int tooltip_show_length = 300;
// Number of milliseconds it should take for the tooltip to hide.
static const int tooltip_hide_length = 200;

ZToolTip *ZToolTip::instance = nullptr;


ZToolTip::ZToolTip(const QPoint &screenpos, QWidget *content, QWidget *owner, const QRect &ownerrect, int mshideafter, int msshowafter, QWidget *parent) :
        base(parent, Qt::ToolTip | Qt::BypassGraphicsProxyWidget| Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus /*| Qt::WindowStaysOnTopHint*/),
        owner(owner), ownerrect(ownerrect), showpos(screenpos), mshideafter(mshideafter), fadestart(-1), fadein(true)
{
    //setObjectName(QLatin1String("qtooltip_label"));
    //setProperty("_q_windowsDropShadow", QVariant(true));

    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    setPalette(QToolTip::palette());
    
    content->setParent(this);
    QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    layout->setMargin(/*4 +*/ qApp->style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, 0, this));
    setLayout(layout);
    layout->addWidget(content);

    ensurePolished();

    qApp->installEventFilter(this);

    animate = qApp->isEffectEnabled(Qt::UI_FadeTooltip) || qApp->isEffectEnabled(Qt::UI_AnimateTooltip);

    if (animate && msshowafter != 0)
        setWindowOpacity(0);

    resize(sizeHint());

    if (msshowafter == 0)
    {
        updatePosition();
        base::show();
    }
    else
        waittimer.start(msshowafter == -1 ? 200 : msshowafter, this);
}

ZToolTip::~ZToolTip()
{

}

/* static */ void ZToolTip::show(const QPoint &screenpos, QWidget *content, QWidget *displayer, const QRect &disprect, int mshideafter, int msshowafter)
{
    if (instance && msshowafter == 0 && isShown())
    {
        instance->update(screenpos, content, displayer, disprect, mshideafter);
        return;
    }

    if (instance)
        instance->hideNow();
#ifdef Q_OS_WIN
    instance = new ZToolTip(screenpos, content, displayer, disprect, mshideafter, msshowafter, qApp->desktop()->screen(qApp->desktop()->isVirtualDesktop() ? qApp->desktop()->screenNumber(screenpos) : qApp->desktop()->screenNumber(displayer)));
#else
    instance = new ZToolTip(screenpos, content, displayer, disprect, mshideafter, msshowafter, displayer);
#endif
}

/* static */ void ZToolTip::show(QWidget *content, QWidget *displayer, const QRect &disprect, int mshideafter, int msshowafter)
{
    if (instance && msshowafter == 0 && isShown())
    {
        instance->update(QPoint(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()), content, displayer, disprect, mshideafter);
        return;
    }

    if (instance)
        instance->hideNow();
#ifdef Q_OS_WIN
    instance = new ZToolTip(QPoint(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()), content, displayer, disprect, mshideafter, msshowafter);
#else
    instance = new ZToolTip(QPoint(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()), content, displayer, disprect, mshideafter, msshowafter, displayer);
#endif
}

/* static */ void ZToolTip::startHide()
{
    if (instance == nullptr)
        return;
    instance->startFade(false);
}

void ZToolTip::paintEvent(QPaintEvent *e)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.init(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();

    base::paintEvent(e);
}

void ZToolTip::resizeEvent(QResizeEvent *e)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.init(this);
    if (qApp->style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);

    base::resizeEvent(e);
}

bool ZToolTip::eventFilter(QObject *o, QEvent *e)
{
    switch (e->type()) {
//#ifdef Q_DEAD_CODE_FROM_QT4_MAC
//    case QEvent::KeyPress:
//    case QEvent::KeyRelease: {
//        int key = static_cast<QKeyEvent *>(e)->key();
//        Qt::KeyboardModifiers mody = static_cast<QKeyEvent *>(e)->modifiers();
//        if (!(mody & Qt::KeyboardModifierMask)
//            && key != Qt::Key_Shift && key != Qt::Key_Control
//            && key != Qt::Key_Alt && key != Qt::Key_Meta)
//            hideTip();
//        break;
//    }
//#endif
    case QEvent::Leave:
        if (o == owner)
            startFade(false);
        break;


#if defined (Q_OS_QNX) // On QNX the window activate and focus events are delayed and will appear
        // after the window is shown.
    case QEvent::WindowActivate:
    case QEvent::FocusIn:
        return false;
    case QEvent::WindowDeactivate:
        if (o != this)
            return false;
        hideNow();
        break;
    case QEvent::FocusOut:
        if (reinterpret_cast<QWindow*>(o) != windowHandle())
            return false;
        hideNow();
        break;
#else
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
#endif
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
    {
        hideNow();
        break;
    }

    case QEvent::MouseMove:
    {
        QPoint p = static_cast<QMouseEvent*>(e)->pos();
        if (o == owner && !ownerrect.isNull() && !ownerrect.contains(p))
        {
            startFade(false);
            //qDebug() << ownerrect << " + " << p;
        }
    }
    default:
        break;
    }
    return false;
}

//bool ZToolTip::event(QEvent *e)
//{
//#ifdef Q_OS_WIN
//    // Hack to add drop shadow to the tooltip under windows. 
//    if (e->type() == QEvent::WinIdChange)
//    {
//        HWND hwnd = reinterpret_cast<HWND>(winId());
//        DWORD class_style = ::GetClassLong(hwnd, GCL_STYLE);
//        class_style |= CS_DROPSHADOW;
//        ::SetClassLong(hwnd, GCL_STYLE, class_style);
//    }
//#endif
//
//    return base::event(e);
//}

void ZToolTip::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == fadetimer.timerId() && fadestart != -1)
    {
        int len = fadein ? tooltip_show_length : tooltip_hide_length;

        qreal pos = clamp(qreal(time.elapsed() - fadestart) / len, 0.0, 1.0);

        bool done = false;
        if (std::abs(1.0 - pos) < 0.001)
        {
            pos = 1.0;
            done = true;
        }

        if (fadein)
            setWindowOpacity(pos);
        else
            setWindowOpacity(1.0 - pos);

        if (done)
        {
            fadestart = -1;
            fadetimer.stop();
            time.invalidate();

            if (!fadein)
                hideNow();
            else
                waittimer.start(mshideafter, this);
        }

        return;
    }
    if (e->timerId() == waittimer.timerId() && fadestart == -1)
    {
        waittimer.stop();
        if (isVisible())
            startFade(false);
        else
        {
            updatePosition();

            base::show();
            startFade(true);
        }
        return;
    }

    base::timerEvent(e);
}

/*static*/ bool ZToolTip::isShown()
{
    return instance != nullptr && instance->isVisible() && !instance->fadetimer.isActive() && instance->windowOpacity() == 1.0;
}

void ZToolTip::hideNow()
{
    if (instance == nullptr)
        return;
    instance->fadetimer.stop();
    instance->waittimer.stop();
    instance->time.invalidate();
    instance->fadestart = -1;

    instance->hide();
    instance->deleteLater();
    instance = nullptr;
}

void ZToolTip::startFade(bool show)
{
    if (!animate)
    {
        if (!show)
            hideNow();
        else
            waittimer.start(mshideafter, this);
        return;
    }
        
    if (!time.isValid())
        time.start();

    // Already hiding.
    if (!show && !fadein && fadestart != -1)
        return;

    fadetimer.stop();
    waittimer.stop();

//#ifdef _DEBUG
//    if (!show)
//        qDebug() << "Fading out";
//#endif


    qreal normalop = qApp->style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, 0, this) / 255.0;

    // If we are in the middle of a fade-in and must fade out instead, the control should
    // continue from its current opacity

    qreal opacity = windowOpacity();

    if (!show && fadestart != -1 && std::abs(opacity - normalop) > 0.001)
    {
        // Fake the start of a fadeout so we continue with the current opacity.

        fadestart = time.elapsed() - tooltip_hide_length * (opacity / normalop);
    }
    else
        fadestart = time.elapsed();
    fadein = show;

    fadetimer.start(20, this);
    //setWindowOpacity();
}

void ZToolTip::updatePosition()
{
    if (showpos == QPoint(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()))
        showpos = QCursor::pos();

//#ifdef Q_OS_WIN
//    QWidget *screen = qApp->desktop()->screen(qApp->desktop()->isVirtualDesktop() ? qApp->desktop()->screenNumber(showpos) : qApp->desktop()->screenNumber(owner));
//    setParent(screen, windowFlags());
//#endif
    QScreen *screen = qApp->primaryScreen()->virtualSiblings().contains(qApp->primaryScreen()) ? qApp->screenAt(showpos) : qApp->screens().at(qApp->desktop()->screenNumber(owner));
    QRect rect = screen->geometry();

    QPoint p = showpos + QPoint(2, 24);
    if (p.x() + width() > rect.x() + rect.width())
        p.rx() -= 4 + width();
    if (p.y() + height() > rect.y() + rect.height())
        p.ry() -= 24 + height();
    if (p.y() < rect.y())
        p.setY(rect.y());
    if (p.x() + width() > rect.x() + rect.width())
        p.setX(rect.x() + rect.width() - width());
    if (p.x() < rect.x())
        p.setX(rect.x());
    if (p.y() + height() > rect.y() + rect.height())
        p.setY(rect.y() + rect.height() - height());
    move(p);
}

void ZToolTip::update(const QPoint &screenpos, QWidget *content, QWidget * /*displayer*/, const QRect &disprect, int hideafter)
{
    waittimer.stop();
    fadetimer.stop();

    showpos = screenpos;
    ownerrect = disprect;
    mshideafter = hideafter;
    fadestart = -1;
    fadein = false;

    qApp->removeEventFilter(this);
    ((QBoxLayout*)layout())->itemAt(0)->widget()->deleteLater();
    content->setParent(this);
    ((QBoxLayout*)layout())->addWidget(content);
    ensurePolished();
    qApp->installEventFilter(this);
    resize(sizeHint());
    updatePosition();

    waittimer.start(mshideafter, this);
}


//-------------------------------------------------------------
