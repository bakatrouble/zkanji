/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QBoxLayout>
#include <QMenuBar>
#include <QPainter>
#include <QSplitter>
#include <QContextMenuEvent>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QSystemTrayIcon>
#include <QDesktopWidget>
#include <QScreen>
#include <QStringBuilder>
#include <QTimer>
#include <QWindow>

#include "zkanjiform.h"
#include "ui_zkanjiform.h"

#include "zevents.h"
#include "dialogs.h"
#include "globalui.h"
#include "zkanjiwidget.h"
#include "dictionarywidget.h"
#include "generalsettings.h"
#include "words.h"
#include "zui.h"

#ifdef Q_OS_WIN
#include <windows.h>
#undef max
#undef min
#endif


//-------------------------------------------------------------


int makeCommand(Commands command, CommandCategories categ)
{
    return (int)command | (int)categ;
}


//-------------------------------------------------------------

namespace
{
    // Whether the main window should be maximized right after load when restoring from the taskbar.
    bool loadrestoremaximized = false;
}

ZDockOverlay::ZDockOverlay(QWidget *parent) : base(parent, Qt::Widget | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
}

ZDockOverlay::~ZDockOverlay()
{

}

void ZDockOverlay::paintEvent(QPaintEvent * /*e*/)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(64, 130, 225, 128));
}


//-------------------------------------------------------------


extern char ZKANJI_PROGRAM_VERSION[];


ZKanjiForm::ZKanjiForm(bool mainform, QWidget *parent) : base(parent, parent != nullptr ? Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint
    : Qt::WindowFlags()), ui(new Ui::ZKanjiForm), mainform(mainform), activewidget(nullptr), //activepage(-1), activedict(nullptr),
    docking(false), menupdatepending(false), menusearchwidget(nullptr), menugroupwidget(nullptr), overlay(nullptr),
    restoremaximized(false), skipchange(false), skipmenu(false), dictmenu(nullptr), dictmap(nullptr), commandmap(nullptr), searchgroup(nullptr)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("zkanji %1").arg(ZKANJI_PROGRAM_VERSION));

    //setMouseTracking(true);

    if (!mainform)
        setAttribute(Qt::WA_DeleteOnClose);

    // Add new widget to the central widget by creating a new layout there.
    QWidget *cw = centralWidget();

    QHBoxLayout *l = new QHBoxLayout(cw);
    l->setContentsMargins(Settings::scaled(2), Settings::scaled(2), Settings::scaled(2), Settings::scaled(2));
    ZKanjiWidget *w = new ZKanjiWidget(cw);
    l->addWidget(w);

    if (!mainform)
        activewidget = w;
    fillMainMenu();

    w->setMode(ViewModes::WordSearch);

    if (mainform)
        connect(qApp, &QApplication::focusChanged, this, &ZKanjiForm::appFocusChanged);

    connect(gUI, &GlobalUI::dictionaryAdded, this, &ZKanjiForm::dictionaryAdded);
    connect(gUI, &GlobalUI::dictionaryRemoved, this, &ZKanjiForm::dictionaryRemoved);
    connect(gUI, &GlobalUI::dictionaryMoved, this, &ZKanjiForm::dictionaryMoved);
    connect(gUI, &GlobalUI::dictionaryRenamed, this, &ZKanjiForm::dictionaryRenamed);
    connect(gUI, &GlobalUI::dictionaryFlagChanged, this, &ZKanjiForm::dictionaryFlagChanged);

    updateMainMenu();

    //// Allowing the ZKanjiWidget to calculate correct sizes.
    //setAttribute(Qt::WA_DontShowOnScreen);
    //show();

    //QRect fr = frameGeometry();
    //int scrnum = -1;
    //QRect g = qApp->desktop()->screenGeometry(mainform ? this : (QWidget*)gUI->mainForm());
    //move(g.left() + (g.width() - fr.width()) / 2, g.top() + (g.height() - fr.height()) / 2);

    //hide();
    //setAttribute(Qt::WA_DontShowOnScreen, false);

    updateWindowGeometry(this);
}

ZKanjiForm::ZKanjiForm(ZKanjiWidget *w, QWidget *parent)
    : base(parent, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
    ui(new Ui::ZKanjiForm), mainform(false), activewidget(nullptr), //activepage(-1), activedict(nullptr),
    docking(false), menupdatepending(false), menusearchwidget(nullptr), menugroupwidget(nullptr), overlay(nullptr),
    restoremaximized(false), skipchange(false), skipmenu(false), dictmenu(nullptr), dictmap(nullptr), commandmap(nullptr), searchgroup(nullptr)
{
    ui->setupUi(this);

    //setMouseTracking(true);

    setAttribute(Qt::WA_DeleteOnClose);

    // Add the passed widget to the central widget by creating a new layout there.
    QWidget *cw = centralWidget();
    w->setParent(cw);

    QHBoxLayout *l = new QHBoxLayout(cw);
    l->setContentsMargins(Settings::scaled(2), Settings::scaled(2), Settings::scaled(2), Settings::scaled(2));
    l->addWidget(w);

    activewidget = w;
    fillMainMenu();

    connect(gUI, &GlobalUI::dictionaryAdded, this, &ZKanjiForm::dictionaryAdded);
    connect(gUI, &GlobalUI::dictionaryRemoved, this, &ZKanjiForm::dictionaryRemoved);
    connect(gUI, &GlobalUI::dictionaryMoved, this, &ZKanjiForm::dictionaryMoved);
    connect(gUI, &GlobalUI::dictionaryRenamed, this, &ZKanjiForm::dictionaryRenamed);

    updateMainMenu();

    // Allowing the ZKanjiWidget to calculate correct sizes.
    //setAttribute(Qt::WA_DontShowOnScreen);
    //show();

    //QRect fr = frameGeometry();
    //QRect g = qApp->desktop()->screenGeometry((QWidget*)gUI->mainForm());
    //move(g.left() + (g.width() - fr.width()) / 2, g.top() + (g.height() - fr.height()) / 2);

    //hide();
    //setAttribute(Qt::WA_DontShowOnScreen, false);

    updateWindowGeometry(this);
}

ZKanjiForm::~ZKanjiForm()
{
    delete ui;
}

void ZKanjiForm::saveXMLSettings(QXmlStreamWriter &writer) const
{
    if (settingsrect.isValid())
    {
        int snum = screenNumber(settingsrect);
        QRect sg = qApp->screens().at(snum)->geometry();

        writer.writeAttribute("x", QString::number(settingsrect.left()));
        writer.writeAttribute("y", QString::number(settingsrect.top()));
        writer.writeAttribute("width", QString::number(settingsrect.width()));
        writer.writeAttribute("height", QString::number(settingsrect.height()));
        writer.writeAttribute("screenx", QString::number(sg.left()));
        writer.writeAttribute("screeny", QString::number(sg.top()));
        writer.writeAttribute("screenw", QString::number(sg.width()));
        writer.writeAttribute("screenh", QString::number(sg.height()));

        if (mainform)
        {
            if ((Settings::general.minimizebehavior == GeneralSettings::DefaultMinimize && windowState().testFlag(Qt::WindowMinimized)) || gUI->isInTray())
                writer.writeAttribute("state", "minimized");
            else if (windowState().testFlag(Qt::WindowMaximized))
                writer.writeAttribute("state", "maximized");

            writer.writeAttribute("restoremaximized", restoremaximized ? "1" : "0");
        }
    }

    QWidget *cw = centralWidget();

    QWidget *w = cw->findChild<QWidget*>();
    if (w->window() != this)
        w = nullptr;

    QSplitter *s = dynamic_cast<QSplitter*>(w);
    if (s == nullptr)
    {
        writer.writeStartElement("State");
        ((ZKanjiWidget*)w)->saveXMLSettings(writer);
        writer.writeEndElement(); /* State */
        return;
    }

    std::list<int> stack;

    writer.writeStartElement("Splitter");
    writer.writeAttribute("orientation", s->orientation() == Qt::Horizontal ? "horz" : "vert");

    int ix = 0;
    int siz = s->count();
    while (s != nullptr)
    {
        w = s->widget(ix);
        int splitsiz = s->sizes().at(ix);
        if (dynamic_cast<QSplitter*>(w) != nullptr)
        {
            stack.push_front(ix);
            s = (QSplitter*)w;
            siz = s->count();

            // TODO: fix this to save the rest of the settings and quit the program.
            if (siz == 0)
                throw "Empty splitter must have been caused by other bug before.";

            ix = 0;
            writer.writeStartElement("Splitter");
            writer.writeAttribute("orientation", s->orientation() == Qt::Horizontal ? "horz" : "vert");
            writer.writeAttribute("splitsize", QString::number(splitsiz));
            continue;
        }

        writer.writeStartElement("State");
        writer.writeAttribute("splitsize", QString::number(splitsiz));
        ((ZKanjiWidget*)w)->saveXMLSettings(writer);
        writer.writeEndElement(); /* State */

        ++ix;
        while (ix == siz)
        {
            writer.writeEndElement(); /* Splitter */
            s = dynamic_cast<QSplitter*>(s->parentWidget());
            if (s == nullptr)
                break;
            siz = s->count();
            ix = stack.front() + 1;
            stack.pop_front();
        }
    }

    //writer.writeEndElement(); /* Splitter */
}

void ZKanjiForm::loadXMLSettings(QXmlStreamReader &reader)
{
    // Qt incorrectly(?) sends size and move events for a previous size and position after
    // startup, and never posts events sent right afterwards. We need to process events here,
    // or the window size and position won't be loaded.

    //qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

    FlagGuard<bool> menuguard(&skipmenu, true, false);

    bool ok = false;

    QRect geom;
    geom.setLeft(reader.attributes().value("x").toInt(&ok));
    geom.setTop((ok ? reader.attributes().value("y").toInt(&ok) : -1));
    geom.setWidth((ok ? reader.attributes().value("width").toInt(&ok) : -1));
    geom.setHeight((ok ? reader.attributes().value("height").toInt(&ok) : -1));

    QRect screen;
    screen.setLeft(ok ? reader.attributes().value("screenx").toInt(&ok) : -1);
    screen.setTop(ok ? reader.attributes().value("screeny").toInt(&ok) : -1);
    screen.setWidth(ok ? reader.attributes().value("screenw").toInt(&ok) : -1);
    screen.setHeight(ok ? reader.attributes().value("screenh").toInt(&ok) : -1);
    ok = ok && screen.width() > 0 && screen.height() > 0 && screen.width() < 999999 && screen.height() < 999999 && screen.left() > -999999 && screen.left() < 999999 && screen.top() > -999999 && screen.top() < 999999;

    QRect sg = ok ? qApp->screens().at(screenNumber(geom))->geometry() : QRect();

    if (!ok)
        geom = QRect();
    else if (!geom.isEmpty())
    {
        if (geom.width() > sg.width() - 2)
            geom.setWidth(sg.width() - 2);
        if (geom.height() > sg.height() - 16)
            geom.setHeight(sg.height() - 16);

        resize(geom.size());
        
        if (Settings::general.savewinpos)
        {
            if (geom.left() < sg.left())
                geom.setLeft(sg.left());
            else if (geom.left() + geom.width() >= sg.left() + sg.width())
                geom.setLeft(sg.left() + sg.width() - geom.width());
            if (geom.top() < sg.top())
                geom.setTop(sg.top());
            else if (geom.top() + geom.height() >= sg.top() + sg.height())
                geom.setTop(sg.top() + sg.height() - geom.height());
            move(geom.topLeft());
        }
        settingsrect = geom;
    }

    //if (!geom.isEmpty())
    //{
    //    resize(geom.size());
    //    move(geom.topLeft());
    //    settingsrect = geom;
    //}

    restoremaximized = false;

    if (Settings::general.savewinpos && mainform)
    {
        Qt::WindowState state;

        switch (Settings::general.startstate)
        {
        case GeneralSettings::AlwaysMinimize:
            state = Qt::WindowMinimized;
            break;
        case GeneralSettings::AlwaysMaximize:
            state = Qt::WindowMaximized;
            break;
        case GeneralSettings::ForgetState:
            state = Qt::WindowNoState;
            break;
        default:
        {
            QStringRef statestr = reader.attributes().value("state");
            state = statestr == "minimized" ? Qt::WindowMinimized : statestr == "maximized" ? Qt::WindowMaximized : Qt::WindowNoState;
        }
        }

        if (!reader.hasError())
        {
            FlagGuard<bool> guard(&skipchange, true, false);
            if (state != Qt::WindowNoState)
            {
                loadrestoremaximized = restoremaximized = reader.attributes().value("restoremaximized") == "1";
                restoremaximized = false;
                //                if (restoremaximized && state != Qt::WindowMaximized)
                //                {
                //    if (!isVisible())
                //    {
                //setAttribute(Qt::WA_DontShowOnScreen);
                //                        showMaximized();
                //        setWindowState(Qt::WindowMaximized);
                //                        qApp->processEvents();
                //                        hide();
                //setAttribute(Qt::WA_DontShowOnScreen, false);
                //    }
                //    else
                //        setWindowState(Qt::WindowMaximized);
                //                }
            }
            setWindowState(state);
        }
    }

    QWidget *cw = centralWidget();
    if (!mainform)
    {
        ZKanjiWidget *w = cw->findChild<ZKanjiWidget*>();
#ifdef _DEBUG
        if (w == nullptr || w->window() != this)
            throw "No widget";
#endif

        bool loaded = false;
        while (reader.readNextStartElement())
        {
            if (!loaded && reader.name() == "State")
            {
                w->loadXMLSettings(reader);
                loaded = true;
            }
            else
                reader.skipCurrentElement();
        }

        if (!reader.hasError())
        {
            skipchange = false;
            updateMainMenu();
        }

        //if (!geom.isEmpty())
        //{
        //    qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

        //    updateGeometry();
        //    layout()->invalidate();
        //    layout()->activate();

        //    resize(geom.size());
        //    move(geom.topLeft());
        //    settingsrect = geom;
        //}
        return;
    }

    QSplitter *splitter = nullptr;
    std::list<QList<int>> sizestack;

    bool foundelement;
    while ((foundelement = reader.readNextStartElement()) == true || splitter != nullptr)
    {
        if (!foundelement)
        {
            splitter->setSizes(sizestack.back());

            QWidget *w = nullptr;
            int wsiz = 0;
            if (splitter->count() < 2)
            {
                // Empty splitter encountered in a bad state file. The splitter is silently
                // removed, hoping the file is not entirely corrupted.

                if (splitter->count() == 1)
                {
                    w = splitter->widget(0);
                    w->setParent(nullptr);
                    wsiz = sizestack.back().back();
                }

                sizestack.pop_back();
                sizestack.back().pop_back();
                QSplitter *oldsplitter = splitter;
                splitter = dynamic_cast<QSplitter*>(splitter->parentWidget());
                delete oldsplitter;
            }
            else
            {
                sizestack.pop_back();
                splitter = dynamic_cast<QSplitter*>(splitter->parentWidget());
            }

            if (w != nullptr)
            {
                if (splitter != nullptr)
                {
                    sizestack.back() << wsiz;
                    splitter->addWidget(w);
                }
                else
                    cw->layout()->addWidget(w);
            }

            if (!splitter)
            {
                // It'd be possible to continue here instead of skipping if there was no
                // "State" element coming after the top splitter, which cannot be guaranteed.
                reader.skipCurrentElement();

                if (!reader.hasError())
                {
                    skipchange = false;
                    updateMainMenu();
                }

                //if (!geom.isEmpty())
                //{
                //    qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

                //    updateGeometry();
                //    layout()->invalidate();
                //    layout()->activate();

                //    resize(geom.size());
                //    move(geom.topLeft());
                //    settingsrect = geom;
                //}
                return;
            }
            continue;
        }

        if (splitter == nullptr && reader.name() == "State")
        {
            // No "Splitter" was found, only a "State" which corresponds to a main window
            // without splitters.
            ZKanjiWidget *w = cw->findChild<ZKanjiWidget*>();
#ifdef _DEBUG
            if (w == nullptr || w->window() != this)
                throw "No widget";
#endif

            w->loadXMLSettings(reader);
            reader.skipCurrentElement();

            //if (!geom.isEmpty())
            //{
            //    qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

            //    updateGeometry();
            //    layout()->invalidate();
            //    layout()->activate();

            //    resize(geom.size());
            //    move(geom.topLeft());
            //    settingsrect = geom;
            //}
            return;
        }

        if (reader.name() == "Splitter")
        {
            // The first time a splitter is encountered, the default created widget is not
            // needed.
            if (splitter == nullptr)
                delete cw->findChild<ZKanjiWidget*>();

            QSplitter *s = new QSplitter;
            if (splitter == nullptr)
                cw->layout()->addWidget(s);

            QStringRef ori = reader.attributes().value("orientation");
            bool vert = ori == "vert";

            int siz = reader.attributes().value("splitsize").toInt(&ok);
            if (!ok || siz < 0)
                siz = 0;
            if (splitter != nullptr)
                sizestack.back() << siz;
            sizestack.push_back(QList<int>());

            //s->setHandleWidth(1);
            s->setHandleWidth(Settings::scaled(s->handleWidth() * 0.8));

            s->setChildrenCollapsible(false);
            if (vert)
                s->setOrientation(Qt::Vertical);
            QSizePolicy pol = s->sizePolicy();
            pol.setHorizontalStretch(1);
            s->setSizePolicy(pol);

            if (splitter != nullptr)
                splitter->addWidget(s);

            splitter = s;
        }
        else if (reader.name() == "State")
        {
            ZKanjiWidget *w = new ZKanjiWidget(this);
            w->setMode(ViewModes::WordSearch);

            w->layout()->setContentsMargins(Settings::scaled(2), Settings::scaled(2), Settings::scaled(2), Settings::scaled(0));
            //w->layout()->setMargin(Settings::scaled(2));

            splitter->addWidget(w);

            int siz = reader.attributes().value("splitsize").toInt(&ok);
            if (!ok || siz < 0)
                siz = 0;
            sizestack.back() << siz;

            w->loadXMLSettings(reader);
        }
        else
            reader.skipCurrentElement();
    }

    if (cw->findChild<ZKanjiWidget*>() == nullptr)
        reader.raiseError("EmptyMainWindowState");

    if (!reader.hasError())
    {
        skipchange = false;
        updateMainMenu();
    }

    //if (!geom.isEmpty())
    //{
    //    qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

    //    updateGeometry();
    //    layout()->invalidate();
    //    layout()->activate();

    //    resize(geom.size());
    //    move(geom.topLeft());
    //    settingsrect = geom;
    //}
}

bool ZKanjiForm::mainForm() const
{
    return mainform;
}

bool ZKanjiForm::hasDockChild() const
{
    return mainform && dynamic_cast<ZKanjiWidget*>(centralWidget()->layout()->itemAt(0)->widget()) == nullptr;
}

void ZKanjiForm::floatWidget(ZKanjiWidget* w)
{
#ifdef _DEBUG
    if (!mainform || w == nullptr || w->window() != this || dynamic_cast<QSplitter*>(w->parent()) == nullptr)
        throw "Invalid widget to float.";
#endif

    QWidget *cw = centralWidget();
    QSplitter *splitter = (QSplitter*)w->parent();

    // This creates a new floating window with the widget.
    ZKanjiForm *f = new ZKanjiForm(w, this);
    gUI->addCreatedWindow(f);
    f->show();

    if (splitter->count() == 1)
    {
        // The splitter will be removed after the widget has ben reparented, and the remaining
        // child widget moved to the splitter's parent.

        if (splitter->parent() == cw)
        {
            // When the splitter is right on top of the main form, its children will simply be
            // moved there.

            QHBoxLayout *cl = (QHBoxLayout*)cw->layout();
            cl->replaceWidget(splitter, splitter->widget(0), Qt::FindDirectChildrenOnly);

            //((ZKanjiWidget*)cl->itemAt(0)->widget())->setContextMenuPolicy(Qt::NoContextMenu);
        }
        else
        {
            QSplitter *s = (QSplitter*)splitter->parent();
            int wix = s->indexOf(splitter);

            if (dynamic_cast<QSplitter*>(splitter->widget(0)) != nullptr)
            {
                // If the child is a splitter as well, its children must be parented to the
                // original splitter.
                QSplitter *cs = (QSplitter*)splitter->widget(0);
                for (int ix = cs->count() - 1; ix != -1; --ix)
                    s->insertWidget(wix, cs->widget(ix));

                cs->hide();
                cs->deleteLater();
            }
            else
            {
                // Move the remaining widget to the parent splitter at the old splitter's index.
                s->insertWidget(wix, splitter->widget(0));
            }
        }
        splitter->hide();
        splitter->deleteLater();
    }

    updateMainMenu();
}

void ZKanjiForm::moveToScreen(int screennum)
{
    QRect sr = settingsrect;

    int snum = screenNumber(sr);
    if (snum == screennum)
        return;

    bool maxed = false;
    Qt::WindowStates state = windowState();
    if (windowState().testFlag(Qt::WindowMaximized))
    {
        maxed = true;
        setWindowState(windowState() & ~Qt::WindowMaximized);
    }
    QPoint sp = settingsrect.topLeft() - qApp->screens().at(snum)->geometry().topLeft();

    QRect sg = qApp->screens().at(screennum)->geometry();
    sp += sg.topLeft();

    if (settingsrect.width() > sg.width() || settingsrect.height() > sg.height())
        resize(std::min(settingsrect.width(), sg.width()), std::min(settingsrect.height(), sg.height()));

    move(QPoint(std::min(std::max(sg.left(), sp.x()), sg.left() + sg.width() - sr.width()), std::min(std::max(sg.top(), sp.y()), sg.top() + sg.height() - sr.height())));

    windowHandle()->setScreen(qApp->screens().at(screennum));

    if (maxed)
        setWindowState(state);
}

void ZKanjiForm::setVisible(bool vis)
{
    if (vis && restoremaximized && !windowState().testFlag(Qt::WindowMinimized))
    {
        restoremaximized = false;

        show();
        setWindowState(Qt::WindowMaximized);

        //QTimer::singleShot(0, [this]() { showMaximized(); });

        return;
    }

    // We must prevent state change events to maximize the window on startup, so the value of
    // restoremaximized must be set false. The state change event is sent on setVisible().
    //bool oldrestore = restoremaximized;
    //restoremaximized = false;

    base::setVisible(vis);

    //restoremaximized = oldrestore;
}

ZKanjiWidget* ZKanjiForm::activeWidget() const
{
    //QWidget *w = focusWidget();
    //while (w != nullptr)
    //{
    //    ZKanjiWidget *zw = dynamic_cast<ZKanjiWidget*>(w);
    //    if (zw != nullptr)
    //    {
    //        activewidget = zw;
    //        return zw;
    //    }
    //    w = w->parentWidget();
    //}
    return activewidget;
}

void ZKanjiForm::showCommand(int command, bool show)
{
    if (commandmap == nullptr)
        return;

    QAction *a = dynamic_cast<QAction*>(commandmap->mapping(command));
    if (a == nullptr)
        return;

    a->setVisible(show);
}

void ZKanjiForm::enableCommand(int command, bool enable)
{
    if (commandmap == nullptr)
        return;

    QAction *a = dynamic_cast<QAction*>(commandmap->mapping(command));
    if (a == nullptr)
        return;

    a->setEnabled(enable);
}

void ZKanjiForm::checkCommand(int command, bool check)
{
    if (commandmap == nullptr)
        return;

    QAction *a = dynamic_cast<QAction*>(commandmap->mapping(command));
    if (a == nullptr)
        return;

    a->setChecked(check);
}

void ZKanjiForm::updateMainMenu()
{
    if (skipmenu)
        return;

    // Each ZKanjiWidget will be filled with the widget with the corresponding mode. If
    // multiple widgets are in the same mode, the widget for the mode will be left empty,
    // unless it's the active widget.
    ZKanjiWidget *wordsrc = nullptr;
    bool foundwordsrc = false;
    ZKanjiWidget *wordgrp = nullptr;
    bool foundwordgrp = false;
    ZKanjiWidget *kanjisrc = nullptr;
    bool foundkanjisrc = false;
    ZKanjiWidget *kanjigrp = nullptr;
    bool foundkanjigrp = false;

    QList<ZKanjiWidget*> wlist = findChildren<ZKanjiWidget*>();
    for (ZKanjiWidget *w : wlist)
    {
        if (w->window() != this)
            continue;

        bool *found = nullptr;
        ZKanjiWidget **widget = nullptr;
        switch (w->mode())
        {
        case ViewModes::WordSearch:
            widget = &wordsrc;
            found = &foundwordsrc;
            break;
        case ViewModes::WordGroup:
            widget = &wordgrp;
            found = &foundwordgrp;
            break;
        case ViewModes::KanjiSearch:
            widget = &kanjisrc;
            found = &foundkanjisrc;
            break;
        case ViewModes::KanjiGroup:
            widget = &kanjigrp;
            found = &foundkanjigrp;
            break;
        default:
            throw "Case not handled.";
        }
        if (!*found || w == activewidget)
            *widget = w;
        else if (*widget != activewidget)
            *widget = nullptr;
        *found = true;
    }

    viewgroup->actions().at(0)->setChecked(activewidget != nullptr && activewidget->mode() == ViewModes::WordSearch);
    viewgroup->actions().at(1)->setChecked(activewidget != nullptr && activewidget->mode() == ViewModes::WordGroup);
    viewgroup->actions().at(2)->setChecked(activewidget != nullptr && activewidget->mode() == ViewModes::KanjiGroup);
    viewgroup->actions().at(3)->setChecked(activewidget != nullptr && activewidget->mode() == ViewModes::KanjiSearch);

    updateSubMenu(wordsrc, wordsearchmenu, (int)CommandLimits::WordsBegin, (int)CommandLimits::WordsEnd, CommandCategories::SearchCateg);
    updateSubMenu(wordgrp, wordgroupmenu, (int)CommandLimits::WordsBegin, (int)CommandLimits::WordsEnd, CommandCategories::GroupCateg);
    updateSubMenu(kanjisrc, kanjisearchmenu, (int)CommandLimits::KanjiBegin, (int)CommandLimits::KanjiEnd, CommandCategories::SearchCateg);
    updateSubMenu(kanjigrp, kanjigroupmenu, (int)CommandLimits::KanjiGroupBegin, (int)CommandLimits::KanjiGroupEnd, CommandCategories::GroupCateg);

    if (wordsrc || wordgrp)
        updateDictionaryMenu(wordsrc, wordgrp);
    else if (activewidget)
        ((QAction*)dictmap->mapping(activewidget->dictionaryIndex()))->setChecked(true);
}

void ZKanjiForm::updateSubMenu(ZKanjiWidget *w, QMenu *menu, int from, int to, CommandCategories categ)
{
    if (w == nullptr)
        menu->menuAction()->setVisible(false);
    else
    {
        for (int ix = from; ix != to; ++ix)
        {
            QObject *tmp = commandmap->mapping(makeCommand((Commands)ix, categ));
            QAction *a = dynamic_cast<QAction*>(tmp);
            if (a == nullptr)
                continue;

            bool enabled = true;
            bool checked = false;
            bool visible = true;
            w->commandState(ix, enabled, checked, visible);

            if (a->isCheckable())
                a->setChecked(checked);
            a->setEnabled(enabled);
            a->setVisible(visible);
        }
        menu->menuAction()->setVisible(true);
    }
}

ZEVENT(UpdateDictionaryMenuEvent)
void ZKanjiForm::updateDictionaryMenu(ZKanjiWidget *srcw, ZKanjiWidget *grpw)
{
    if (menusearchwidget != nullptr && srcw != menusearchwidget)
        disconnect(menusearchwidget, &QObject::destroyed, this, &ZKanjiForm::menuwidgetDestroyed);
    if (srcw != nullptr && srcw != menusearchwidget)
        connect(srcw, &QObject::destroyed, this, &ZKanjiForm::menuwidgetDestroyed);

    if (menugroupwidget != nullptr && grpw != menugroupwidget)
        disconnect(menugroupwidget, &QObject::destroyed, this, &ZKanjiForm::menuwidgetDestroyed);
    if (grpw != nullptr && grpw != menugroupwidget)
        disconnect(grpw, &QObject::destroyed, this, &ZKanjiForm::menuwidgetDestroyed);

    menusearchwidget = srcw;
    menugroupwidget = grpw;
    if (menupdatepending)
        return;

    menupdatepending = true;
    qApp->postEvent(this, new UpdateDictionaryMenuEvent());
}

void ZKanjiForm::executeCommand(int command)
{
    CommandCategories categ = CommandCategories(command & (int)CommandCategories::CategMask);
    command = (command & (int)CommandCategories::CommandMask);

    ViewModes mode;
    if (command >= (int)CommandLimits::WordsBegin && command < (int)CommandLimits::WordsEnd)
        mode = categ == CommandCategories::SearchCateg ? ViewModes::WordSearch : ViewModes::WordGroup;
    else
        mode = categ == CommandCategories::SearchCateg ? ViewModes::KanjiSearch : ViewModes::KanjiGroup;

    if (activewidget != nullptr && activewidget->mode() == mode)
    {
        activewidget->executeCommand(command);
        return;
    }

    QList<ZKanjiWidget*> wlist = findChildren<ZKanjiWidget*>();

    ZKanjiWidget *widget = nullptr;
    for (ZKanjiWidget *w : wlist)
    {
        if (w->mode() != mode || w->window() != this)
            continue;

        if (widget != nullptr)
            return;
        widget = w;
    }

    if (widget == nullptr)
        return;

    widget->executeCommand(command);
}

void ZKanjiForm::setViewMode(int mode)
{
    if (activewidget == nullptr)
        return;

    activewidget->setMode((ViewModes)mode);
}

void ZKanjiForm::switchToDictionary(int index)
{
    qApp->postEvent(this, new SetDictEvent(index));
}

void ZKanjiForm::showDictionaryInfo()
{
    int dictindex = 0;
    if (activewidget != nullptr)
        dictindex = activewidget->dictionaryIndex();
    showDictionaryStats(dictindex);
}

bool ZKanjiForm::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        retranslateMainMenu();
    }
    else if (e->type() == SetDictEvent::Type())
    {
        QList<ZKanjiWidget*> widgets = findChildren<ZKanjiWidget*>();
        int index = ((SetDictEvent*)e)->index();
        for (ZKanjiWidget *w : widgets)
        {
            if (w->window() != this)
                continue;
            w->setDictionary(index);
        }

        //if (activewidget != nullptr)
        //    dictionaryActivated(activewidget);
        return true;
    }
    else if (e->type() == GetDictEvent::Type())
    {
        QList<ZKanjiWidget*> widgets = findChildren<ZKanjiWidget*>();
        GetDictEvent *ge = (GetDictEvent*)e;
        for (ZKanjiWidget *w : widgets)
        {
            if (w->window() == this)
            {
                ge->setResult(w->dictionaryIndex());
                break;
            }
        }
        return true;
    }
    else if (e->type() == StartEvent::Type())
    {
        // Sent from GlobalUI::createWindow() after the window is shown.
        if (isVisible())
            settingsrect = QRect(pos(), size()); //geometry();
        return true;
    }
    else if (e->type() == SaveSizeEvent::Type())
    {
        // Sent from move and resize events.
        if (isVisible() && !windowState().testFlag(Qt::WindowMaximized) && !windowState().testFlag(Qt::WindowMinimized))
            settingsrect = QRect(pos(), size()); //geometry();
    }
    else if (e->type() == UpdateDictionaryMenuEvent::Type())
    {
        menupdatepending = false;
        ZKanjiWidget *srcw = menusearchwidget;
        ZKanjiWidget *grpw = menugroupwidget;
        if (srcw == nullptr && grpw == nullptr)
        {
            bool foundwordsrc = false;
            bool foundwordgrp = false;

            QList<ZKanjiWidget*> wlist = findChildren<ZKanjiWidget*>();
            for (ZKanjiWidget *w : wlist)
            {
                if (w->window() != this)
                    continue;

                bool *found;
                ZKanjiWidget **widget;
                switch (w->mode())
                {
                case ViewModes::WordSearch:
                    widget = &srcw;
                    found = &foundwordsrc;
                    break;
                case ViewModes::WordGroup:
                    widget = &grpw;
                    found = &foundwordgrp;
                    break;
                default:
                    continue;
                }
                if (!*found || w == activewidget)
                    *widget = w;
                else if (*widget != activewidget)
                    *widget = nullptr;
                *found = true;
            }
        }

        if (srcw && srcw->dictionary())
        {
            searchgroup->actions().at(0)->setText(tr("&Japanese to %1").arg(srcw->dictionary()->name()));
            searchgroup->actions().at(1)->setText(tr("%1 &to Japanese").arg(srcw->dictionary()->name()));
        }
        if (grpw && grpw->dictionary())
        {
            wordsgroup->actions().at(0)->setText(tr("&Japanese to %1").arg(grpw->dictionary()->name()));
            wordsgroup->actions().at(1)->setText(tr("%1 &to Japanese").arg(grpw->dictionary()->name()));
        }

        if (activewidget)
            ((QAction*)dictmap->mapping(activewidget->dictionaryIndex()))->setChecked(true);
    }

    return base::event(e);
}

void ZKanjiForm::mouseMoveEvent(QMouseEvent *e)
{
    if (gUI->dockForm() == nullptr)
    {
        base::mouseMoveEvent(e);
        return;
    }

    QPoint p;

    QRect g = gUI->dockForm()->frameGeometry();
    QRect g2 = gUI->dockForm()->geometry();
    p.setX(e->globalPos().x() - g.width() / 2);
    p.setY(e->globalPos().y() - (g2.top() - g.top()) / 2);
    gUI->dockForm()->move(p);

    p = e->globalPos();
    if (validDockPos(p))
        dockAt(centralWidget()->mapFromGlobal(p));
    else
        hideDockOverlay();

    base::mouseMoveEvent(e);
}

void ZKanjiForm::mousePressEvent(QMouseEvent *e)
{
    if (gUI->dockForm() != nullptr)
    {
        if (e->button() == Qt::LeftButton)
        {
            QPoint p = e->globalPos();
            docking = true;
            if (validDockPos(p))
                dockAt(centralWidget()->mapFromGlobal(p), gUI->dockForm());
            docking = false;
        }
        hideDockOverlay();
        gUI->endDockDrag();
    }

    base::mousePressEvent(e);
}

void ZKanjiForm::keyPressEvent(QKeyEvent *e)
{
    if (gUI->dockForm() != nullptr)
    {
        gUI->endDockDrag();
        hideDockOverlay();
        e->accept();
        return;
    }

    base::keyPressEvent(e);
}

void ZKanjiForm::moveEvent(QMoveEvent * /*e*/)
{
    qApp->postEvent(this, new SaveSizeEvent());
    //    //if (!windowState().testFlag(Qt::WindowMaximized) && !windowState().testFlag(Qt::WindowMinimized))
    //    //    settingsrect = normalGeometry();
    //
    //    // Qt sends move events during resize not just during move (tested on Windows.) In case
    //    // the window was merely resized, docking behavior should be avoided.
    //    QRect r;
    //    QPoint p;
    //    if (downnonclient)
    //    {
    //        r = frameGeometry();
    //        if (!qApp->mouseButtons().testFlag(Qt::LeftButton) || framerect == r || framerect.width() != r.width() || framerect.height() != r.height())
    //        {
    //            ((ZKanjiForm*)gUI->mainForm())->hideDockOverlay();
    //            downnonclient = false;
    //        }
    //    }
    //    if (downnonclient)
    //    {
    //        p = QCursor::pos();
    //        if (dockmoving = validDockPos(p))
    //            ((ZKanjiForm*)gUI->mainForm())->dockAt(gUI->mainForm()->centralWidget()->mapFromGlobal(p));
    //        else
    //            ((ZKanjiForm*)gUI->mainForm())->hideDockOverlay();
    //        framerect = frameGeometry();
    //    }
    //
    //    base::moveEvent(e);
}

void ZKanjiForm::resizeEvent(QResizeEvent *e)
{
    //if (!windowState().testFlag(Qt::WindowMaximized) && !windowState().testFlag(Qt::WindowMinimized))
    //    settingsrect = normalGeometry();
    qApp->postEvent(this, new SaveSizeEvent());

    base::resizeEvent(e);
}

void ZKanjiForm::changeEvent(QEvent *e)
{
    if (mainform && e->type() == QEvent::WindowStateChange && !skipchange)
    {
        if (gUI->dockForm() != nullptr)
        {
            gUI->endDockDrag();
            hideDockOverlay();
        }

        bool totray = false;
        QWindowStateChangeEvent *se = (QWindowStateChangeEvent*)e;
        if (windowState().testFlag(Qt::WindowMinimized) && !se->oldState().testFlag(Qt::WindowMinimized))
        {
            totray = Settings::general.minimizebehavior == GeneralSettings::TrayOnMinimize && QSystemTrayIcon::isSystemTrayAvailable();
            restoremaximized = se->oldState().testFlag(Qt::WindowMaximized);


            emit stateChanged(totray || Settings::general.minimizebehavior == GeneralSettings::DefaultMinimize);
        }
        else if (!windowState().testFlag(Qt::WindowMinimized) && !windowState().testFlag(Qt::WindowMaximized))
        {
            if (se->oldState().testFlag(Qt::WindowMinimized))
            {
                emit stateChanged(false);
                if (restoremaximized)
                    setWindowState(Qt::WindowMaximized);
#ifdef Q_OS_WIN
                else
                {
                    move(settingsrect.topLeft());
                    resize(settingsrect.size());
                }
#endif
            }
        }

        if (totray)
        {
            e->ignore();
            qApp->processEvents();
            QTimer::singleShot(0, gUI, &GlobalUI::minimizeToTray);
        }
        else
            base::changeEvent(e);

        if (loadrestoremaximized)
        {
            loadrestoremaximized = false;
            restoremaximized = true;
        }

        return;
    }
    if (e->type() == QEvent::ActivationChange)
    {
        emit activated(this, isActiveWindow());
        //#ifdef Q_OS_MAC
        //        if (isActiveWindow() && activewidget != nullptr)
        //            activateMenu(activewidget);
        //#endif
    }

    base::changeEvent(e);
}

void ZKanjiForm::closeEvent(QCloseEvent *e)
{
    // It's necessary to save settings before the main form closes, because the child windows
    // owned by this form would get destroyed before their settings could be saved.
    if (mainform)
    {
        if (gUI->dockForm() != nullptr)
        {
            gUI->endDockDrag();
            hideDockOverlay();
        }

#ifdef Q_OS_OSX
        if (!e->spontaneous() || !isVisible())
        {
            base::closeEvent(e);
            return;
        }
#endif
        if (!windowState().testFlag(Qt::WindowMinimized) && isVisible() && Settings::general.minimizebehavior == GeneralSettings::TrayOnClose && QSystemTrayIcon::isSystemTrayAvailable())
        {
            emit stateChanged(true);
            e->ignore();
            //qApp->processEvents();
            gUI->minimizeToTray();
            return;
        }

        gUI->saveBeforeQuit();
    }

    base::closeEvent(e);
}

#ifdef Q_OS_WIN
bool ZKanjiForm::nativeEvent(const QByteArray &etype, void *msg, long *result)
{
    if (etype != "windows_generic_MSG")
        return base::nativeEvent(etype, msg, result);

    MSG *m = (MSG*)msg;
    if (mainform && m->message == WM_USER + 999)
    {
        gUI->raiseAndActivate();
        return true;
    }

    return base::nativeEvent(etype, msg, result);
}
#endif

void ZKanjiForm::appFocusChanged(QWidget * /*prev*/, QWidget *current)
{
    if (current == nullptr || current->window() != this)
    {
        if (gUI->dockForm() != nullptr && !docking)
        {
            gUI->endDockDrag();
            hideDockOverlay();
        }

        //if (activewidget != nullptr)
        //    activewidget->showActivated(false);
        return;
    }

    //while (prev && dynamic_cast<ZKanjiWidget*>(prev) == nullptr)
    //    prev = prev->parentWidget();
    if (!current->hasFocus())
        return; //current = nullptr;
    while (current && dynamic_cast<ZKanjiWidget*>(current) == nullptr)
        current = current->parentWidget();

    if (current == nullptr || activewidget == current)
        return;

    if (activewidget != nullptr)
        activewidget->showActivated(false);

    activewidget = (ZKanjiWidget*)current;

    if (activewidget->parentWidget() != centralWidget())
        activewidget->showActivated(true);

    updateMainMenu();
}

void ZKanjiForm::startDockDrag()
{
    gUI->startDockDrag(this);
}

void ZKanjiForm::dictionaryAdded()
{
    if (dictmap == nullptr)
        return;

    int ix = ZKanji::dictionaryCount() - 1;
    gUI->insertCommandAction(dictmap, dictmenu, ZKanji::dictionaryCount() - 1, ZKanji::dictionary(ix)->name(), ix < 9 ? QKeySequence(Qt::ALT + (Qt::Key_1 + ix)) : QKeySequence(), ix, true, dictgroup)->setIcon(ZKanji::dictionaryMenuFlag(ZKanji::dictionary(ix)->name()));
}

void ZKanjiForm::dictionaryRemoved(int /*index*/, int order, void * /*oldaddress*/)
{
    if (dictmap == nullptr)
        return;

    QAction *a = dictmenu->actions().at(order);
    dictmenu->removeAction(a);
    dictmap->removeMappings(a);
    dictgroup->removeAction(a);
    a->deleteLater();

    // addCommandAction(dictmap, dictmenu, ZKanji::dictionary(dix)->name(), ix < 9 ? QKeySequence(Qt::ALT + (Qt::Key_1 + ix)) : QKeySequence(), dix, true, dictgroup);

    for (int ix = order, siz = ZKanji::dictionaryCount(); ix != siz; ++ix)
    {
        int dix = ZKanji::dictionaryPosition(ix);
        a = dictmenu->actions().at(ix);
        dictmap->removeMappings(a);
        dictmap->setMapping(a, dix);
        if (ix < 9)
            a->setShortcut(QKeySequence(Qt::ALT + (Qt::Key_1 + ix)));
        else
            a->setShortcut(QKeySequence());
    }
}

void ZKanjiForm::dictionaryMoved(int from, int to)
{
    if (dictmap == nullptr)
        return;

    QAction *a = dictmenu->actions().at(from);
    dictmenu->removeAction(a);

    if (to > from)
        --to;

    dictmenu->insertAction(dictmenu->actions().at(to), a);

    for (int ix = 0, siz = ZKanji::dictionaryCount(); ix != siz; ++ix)
    {
        int dix = ZKanji::dictionaryPosition(ix);
        a = dictmenu->actions().at(ix);
        dictmap->removeMappings(a);
        dictmap->setMapping(a, dix);
        if (ix < 9)
            a->setShortcut(QKeySequence(Qt::ALT + (Qt::Key_1 + ix)));
        else
            a->setShortcut(QKeySequence());
    }
}

void ZKanjiForm::dictionaryRenamed(const QString &/*oldname*/, int index, int order)
{
    QAction *a = dictmenu->actions().at(order);
    a->setText(ZKanji::dictionary(index)->name());
}

void ZKanjiForm::dictionaryFlagChanged(int index, int order)
{
    QAction *a = dictmenu->actions().at(order);
    a->setIcon(ZKanji::dictionaryMenuFlag(ZKanji::dictionary(index)->name()));
}

void ZKanjiForm::menuwidgetDestroyed(QObject *o)
{
    if (menusearchwidget == o)
        menusearchwidget = nullptr;
    if (menugroupwidget == o)
        menugroupwidget = nullptr;
}

//void ZKanjiForm::installCommands()
//{
//    if (mainform)
//        return;
//
//    ZKanjiWidget *w = findChild<ZKanjiWidget*>();
//
//    QSignalMapper *map = new QSignalMapper(this);
//    connect(map, &QSignalMapper::mappedInt, w, &ZKanjiWidget::executeCommand);
//    addCommandShortcut(map, Qt::Key_F2, UICommands::FromJapanese);
//    addCommandShortcut(map, Qt::Key_F3, UICommands::ToJapanese);
//    addCommandShortcut(map, Qt::Key_F4, UICommands::BrowseJapanese);
//    addCommandShortcut(map, Qt::Key_F5, UICommands::ToggleExamples);
//    addCommandShortcut(map, Qt::Key_F6, UICommands::ToggleAnyStart);
//    addCommandShortcut(map, Qt::Key_F7, UICommands::ToggleAnyEnd);
//    addCommandShortcut(map, Qt::Key_F8, UICommands::ToggleDeinflect);
//    addCommandShortcut(map, Qt::Key_F9, UICommands::ToggleStrict);
//    addCommandShortcut(map, QKeySequence(tr("Ctrl+F")), UICommands::ToggleFilter);
//    addCommandShortcut(map, QKeySequence(tr("Ctrl+Shift+F")), UICommands::EditFilters);
//    addCommandShortcut(map, Qt::Key_F10, UICommands::ToggleMultiline);
//
//    map = new QSignalMapper(this);
//    connect(map, &QSignalMapper::mappedInt, w, &ZKanjiWidget::setMode);
//    for (int ix = 0; ix != 4; ++ix)
//        addCommandShortcut(map, Qt::CTRL + (Qt::Key_1 + ix), ix);
//
//}
//
//void ZKanjiForm::addCommandShortcut(QSignalMapper *map, const QKeySequence &keyseq, int command)
//{
//    QAction *a = new QAction(this);
//    a->setShortcut(keyseq);
//    //a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
//    connect(a, &QAction::triggered, map, (void (QSignalMapper::*)())&QSignalMapper::map);
//    map->setMapping(a, command);
//    addAction(a);
//}

void ZKanjiForm::fillMainMenu()
{
    QMenuBar *menu = menuBar();

    QFont tmp = menu->font();
    tmp.setKerning(false);
    menu->setFont(tmp);

    QAction *a;

    // Warning: update retranslateMainMenu() when changing menu item text or item order, or
    // when adding and removing items. Keyboard shortcuts should be translated as well.
    if (mainform)
    {
        QMenu *filemenu = menu->addMenu(tr("&File"));

        QMenu *datamenu = filemenu->addMenu(tr("&Database update"));

        a = datamenu->addAction(tr("Import &base dictionary..."));
        connect(a, &QAction::triggered, gUI, &GlobalUI::importBaseDict);
        a = datamenu->addAction(tr("Import &example sentences..."));
        connect(a, &QAction::triggered, gUI, &GlobalUI::importExamples);

        QMenu *inmenu = filemenu->addMenu(tr("&Export/Import"));

        a = inmenu->addAction(tr("Export &user data..."));
        connect(a, &QAction::triggered, gUI, &GlobalUI::userExportAction);

        a = inmenu->addAction(tr("Export &dictionary..."));
        connect(a, &QAction::triggered, gUI, &GlobalUI::dictExportAction);

        inmenu->addSeparator();

        a = inmenu->addAction(tr("Import u&ser data..."));
        connect(a, &QAction::triggered, gUI, &GlobalUI::userImportAction);

        a = inmenu->addAction(tr("Import dic&tionary..."));
        connect(a, &QAction::triggered, gUI, &GlobalUI::dictImportAction);

        inmenu->addSeparator();

        a = inmenu->addAction(tr("Import &legacy user data..."));
        connect(a, &QAction::triggered, gUI, &GlobalUI::importOldUserData);

        filemenu->addSeparator();

        a = filemenu->addAction(tr("&Settings..."));
        a->setShortcut(QKeySequence::Preferences);
        connect(a, &QAction::triggered, gUI, &GlobalUI::showSettingsWindow);

        filemenu->addSeparator();

        a = filemenu->addAction(tr("E&xit"));
        a->setShortcut(QKeySequence::Quit);
        connect(a, &QAction::triggered, gUI, &GlobalUI::quit);

        QMenu *studymenu = menu->addMenu(tr("S&tudy"));

        a = studymenu->addAction(tr("Hiragana/Katakana &practice..."));
        //a->setShortcut(Qt::CTRL + Qt::Key_S);
        connect(a, &QAction::triggered, gUI, &GlobalUI::practiceKana);

        a = studymenu->addAction(tr("Long-term &study lists..."));
        //: Long-term study list menu shortcut
        a->setShortcut(QKeySequence(tr("Ctrl+L")) /*Qt::CTRL + Qt::Key_S*/);
        connect(a, &QAction::triggered, gUI, &GlobalUI::showDecks);
    }

    // Dictionary main menu item.

    dictmenu = menu->addMenu(tr("&Dictionary"));
    dictgroup = new QActionGroup(this);

    dictmap = new QSignalMapper(this);
    connect(dictmap, &QSignalMapper::mappedInt, this, &ZKanjiForm::switchToDictionary);
    for (int ix = 0, siz = ZKanji::dictionaryCount(); ix != siz; ++ix)
    {
        int dix = ZKanji::dictionaryPosition(ix);
        gUI->addCommandAction(dictmap, dictmenu, ZKanji::dictionary(dix)->name(), ix < 9 ? QKeySequence(Qt::ALT + (Qt::Key_1 + ix)) : QKeySequence(), dix, true, dictgroup)->setIcon(ZKanji::dictionaryMenuFlag(ZKanji::dictionary(dix)->name()));
    }

    dictmenu->addSeparator();

    a = dictmenu->addAction(tr("&Manage dictionaries..."));
    connect(a, &QAction::triggered, &editDictionaries);

    a = dictmenu->addAction(tr("Dictionary &information..."));
    connect(a, &QAction::triggered, this, &ZKanjiForm::showDictionaryInfo);

    //dictmenu->addSeparator();

    //a = dictmenu->addAction(tr("New word to dictionary..."));
    //connect(a, &QAction::triggered, [this]() { editNewWord(activewidget->dictionary()); });


    commandmap = new QSignalMapper(this);
    connect(commandmap, &QSignalMapper::mappedInt, this, &ZKanjiForm::executeCommand);

    // View main menu item.

    viewmenu = menu->addMenu(tr("&View"));

    QSignalMapper *viewmap = new QSignalMapper(this);
    connect(viewmap, &QSignalMapper::mappedInt, this, &ZKanjiForm::setViewMode);
    viewgroup = new QActionGroup(this);
    gUI->addCommandAction(viewmap, viewmenu, tr("Dictionary &search"), QKeySequence(tr("Ctrl+1")) /*Qt::CTRL + Qt::Key_1*/, 0, true, viewgroup)->setIcon(QIcon(":/magnisearch.svg"));
    gUI->addCommandAction(viewmap, viewmenu, tr("&Word groups"), QKeySequence(tr("Ctrl+2")), 1, true, viewgroup)->setIcon(QIcon(":/wordgroups.svg"));
    gUI->addCommandAction(viewmap, viewmenu, tr("&Kanji groups"), QKeySequence(tr("Ctrl+3")), 2, true, viewgroup)->setIcon(QIcon(":/kanjigroups.svg"));
    gUI->addCommandAction(viewmap, viewmenu, tr("Kan&ji search"), QKeySequence(tr("Ctrl+4")), 3, true, viewgroup)->setIcon(QIcon(":/kanjisearch.svg"));

    viewmenu->addSeparator();

    a = viewmenu->addAction(tr("&New window"));
    a->setShortcut(QKeySequence::New);
    connect(a, &QAction::triggered, gUI, &GlobalUI::createWindow);

    viewmenu->addSeparator();

    // Word search main menu item.

    wordsearchmenu = menu->addMenu(tr("Dictionary &search"));
    searchgroup = new QActionGroup(this);
    fillSearchMenu(commandmap, searchgroup, wordsearchmenu, CommandCategories::SearchCateg);

    wordgroupmenu = menu->addMenu(tr("&Word group"));
    wordsgroup = new QActionGroup(this);
    fillSearchMenu(commandmap, wordsgroup, wordgroupmenu, CommandCategories::GroupCateg);

    // Kanji search main menu item.

    kanjisearchmenu = menu->addMenu(tr("Kan&ji search"));
    QMenu *kanjisrcsubmenu = kanjisearchmenu->addMenu(tr("&Filters"));

    gUI->addCommandAction(commandmap, kanjisrcsubmenu, tr("&Reset"), QKeySequence(/*tr("Ctrl+R")*/), makeCommand(Commands::ResetKanjiSearch, CommandCategories::SearchCateg), false);
    kanjisrcsubmenu->addSeparator();
    gUI->addCommandAction(commandmap, kanjisrcsubmenu, tr("Stroke &count filter"), QKeySequence(tr("1"))/*Qt::Key_1*/, makeCommand(Commands::StrokeFilter, CommandCategories::SearchCateg), true);
    gUI->addCommandAction(commandmap, kanjisrcsubmenu, tr("&JLPT filter"), QKeySequence(tr("2")), makeCommand(Commands::JLPTFilter, CommandCategories::SearchCateg), true);
    gUI->addCommandAction(commandmap, kanjisrcsubmenu, tr("&Meaning filter"), QKeySequence(tr("3")), makeCommand(Commands::MeaningFilter, CommandCategories::SearchCateg), true);
    gUI->addCommandAction(commandmap, kanjisrcsubmenu, tr("Rea&ding filter"), QKeySequence(tr("4")), makeCommand(Commands::ReadingFilter, CommandCategories::SearchCateg), true);
    gUI->addCommandAction(commandmap, kanjisrcsubmenu, tr("Jouyou &grade filter"), QKeySequence(tr("5")), makeCommand(Commands::JouyouFilter, CommandCategories::SearchCateg), true);
    kanjisrcsubmenu->addSeparator();
    gUI->addCommandAction(commandmap, kanjisrcsubmenu, tr("R&adicals filter"), QKeySequence(tr("6")), makeCommand(Commands::RadicalsFilter, CommandCategories::SearchCateg), true);
    kanjisrcsubmenu->addSeparator();
    gUI->addCommandAction(commandmap, kanjisrcsubmenu, tr("&Index filter"), QKeySequence(tr("7")), makeCommand(Commands::IndexFilter, CommandCategories::SearchCateg), true);
    gUI->addCommandAction(commandmap, kanjisrcsubmenu, tr("&SKIP filter"), QKeySequence(tr("8")), makeCommand(Commands::SKIPFilter, CommandCategories::SearchCateg), true);
    kanjisearchmenu->addSeparator();

    fillKanjiMenu(commandmap, kanjisearchmenu, CommandCategories::SearchCateg);

    kanjigroupmenu = menu->addMenu(tr("&Kanji group"));
    fillKanjiMenu(commandmap, kanjigroupmenu, CommandCategories::GroupCateg);

    helpmenu = menu->addMenu(tr("&Help"));

    a = helpmenu->addAction(tr("&About"));
    connect(a, &QAction::triggered, gUI, &GlobalUI::showAbout);

    a = helpmenu->addAction(tr("About &Qt"));
    connect(a, &QAction::triggered, qApp, &QApplication::aboutQt);

    if (!mainform)
    {
        a = menu->addAction(tr("Dock"));
        a->setShortcut(QKeySequence(tr("F9")));
        a->setIcon(QIcon(":/dockwindows.svg"));

        connect(a, &QAction::triggered, this, &ZKanjiForm::startDockDrag);
    }
}

void ZKanjiForm::retranslateMainMenu()
{
    QMenuBar *menu = menuBar();

    if (mainform)
    {
        QAction *filemenu = menu->actions().at(0);
        filemenu->setText(tr("&File"));

        QAction *datamenu = filemenu->menu()->actions().at(0);
        datamenu->setText(tr("&Database update"));

        QAction *a = datamenu->menu()->actions().at(0);
        a->setText(tr("Import &base dictionary..."));
        a = datamenu->menu()->actions().at(1);
        a->setText(tr("Import &example sentences..."));

        QAction *inmenu = filemenu->menu()->actions().at(1);
        inmenu->setText(tr("&Export/Import"));

        a = inmenu->menu()->actions().at(0);
        a->setText(tr("Export &user data..."));
        a = inmenu->menu()->actions().at(1);
        a->setText(tr("Export &dictionary..."));

        // inmenu action 2 is separator

        a = inmenu->menu()->actions().at(3);
        a->setText(tr("Import u&ser data..."));
        a = inmenu->menu()->actions().at(4);
        a->setText(tr("Import dic&tionary..."));

        // inmenu action 5 is separator

        a = inmenu->menu()->actions().at(6);
        a->setText(tr("Import &legacy user data..."));

        // filemenu action 2 is separator

        a = filemenu->menu()->actions().at(3);
        a->setText(tr("&Settings..."));

        // filemenu action 4 is separator

        a = filemenu->menu()->actions().at(5);
        a->setText(tr("E&xit"));

        QAction *studymenu = menu->actions().at(1);
        studymenu->setText(tr("S&tudy"));

        a = studymenu->menu()->actions().at(0);
        a->setText(tr("Hiragana/Katakana &practice..."));

        a = studymenu->menu()->actions().at(1);
        a->setText(tr("Long-term &study lists..."));
        a->setShortcut(QKeySequence(tr("Ctrl+L")));
    }

    dictmenu->menuAction()->setText(tr("&Dictionary"));

    // dictmenu action X is separator (X is number of dictionaries)

    QAction *a = dictmenu->actions().at(ZKanji::dictionaryCount() + 1);
    a->setText(tr("&Manage dictionaries..."));

    a = dictmenu->actions().at(ZKanji::dictionaryCount() + 2);
    a->setText(tr("Dictionary &information..."));

    viewmenu->menuAction()->setText(tr("&View"));

    a = viewmenu->actions().at(0);
    a->setText(tr("Dictionary &search"));
    a->setShortcut(QKeySequence(tr("Ctrl+1")));

    a = viewmenu->actions().at(1);
    a->setText(tr("&Word groups"));
    a->setShortcut(QKeySequence(tr("Ctrl+2")));

    a = viewmenu->actions().at(2);
    a->setText(tr("&Kanji groups"));
    a->setShortcut(QKeySequence(tr("Ctrl+3")));

    a = viewmenu->actions().at(3);
    a->setText(tr("Kan&ji search"));
    a->setShortcut(QKeySequence(tr("Ctrl+4")));

    // viewmenu action 4 is separator

    a = viewmenu->actions().at(5);
    a->setText(tr("&New window"));
    a->setShortcut(QKeySequence::New);

    // viewmenu action 6 is separator ? not needed as last item?

    wordsearchmenu->menuAction()->setText(tr("Dictionary &search"));
    retranslateSearchMenu(wordsearchmenu, CommandCategories::SearchCateg);

    wordgroupmenu->menuAction()->setText(tr("&Word group"));
    retranslateSearchMenu(wordgroupmenu, CommandCategories::GroupCateg);

    kanjisearchmenu->menuAction()->setText(tr("Kan&ji search"));

    QAction *kanjisrcsubmenu = kanjisearchmenu->actions().at(0);
    kanjisrcsubmenu->setText(tr("&Filters"));

    a = kanjisrcsubmenu->menu()->actions().at(0);
    a->setText(tr("&Reset"));

    // kanjisrcsubmenu action 1 is separator

    a = kanjisrcsubmenu->menu()->actions().at(2);
    a->setText(tr("Stroke &count filter"));

    a = kanjisrcsubmenu->menu()->actions().at(3);
    a->setText(tr("&JLPT filter"));

    a = kanjisrcsubmenu->menu()->actions().at(4);
    a->setText(tr("&Meaning filter"));

    a = kanjisrcsubmenu->menu()->actions().at(5);
    a->setText(tr("Rea&ding filter"));

    a = kanjisrcsubmenu->menu()->actions().at(6);
    a->setText(tr("Jouyou &grade filter"));

    // kanjisrcsubmenu action 7 is separator

    a = kanjisrcsubmenu->menu()->actions().at(8);
    a->setText(tr("R&adicals filter"));

    // kanjisrcsubmenu action 9 is separator

    a = kanjisrcsubmenu->menu()->actions().at(10);
    a->setText(tr("&Index filter"));

    a = kanjisrcsubmenu->menu()->actions().at(11);
    a->setText(tr("&SKIP filter"));

    // kanjisearchmenu action 1 is separator

    retranslateKanjiMenu(kanjisearchmenu, CommandCategories::SearchCateg, 2);

    kanjigroupmenu->menuAction()->setText(tr("&Kanji group"));
    retranslateKanjiMenu(kanjigroupmenu, CommandCategories::GroupCateg);

    helpmenu->menuAction()->setText(tr("&Help"));
    a = helpmenu->actions().at(0);
    a->setText(tr("&About"));

    a = helpmenu->actions().at(1);
    a->setText(tr("About &Qt"));


    if (!mainform)
    {
        a = menu->actions().constLast();
        a->setText(tr("Dock"));
        a->setShortcut(QKeySequence(tr("F9")));
    }

    qApp->postEvent(this, new UpdateDictionaryMenuEvent);
}

void ZKanjiForm::fillSearchMenu(QSignalMapper *signalmap, QActionGroup *group, QMenu *menu, CommandCategories categ)
{
    QMenu *srcsubmenu = menu->addMenu(tr("&Mode"));

    gUI->addCommandAction(signalmap, srcsubmenu, tr("&Japanese to %1"), categ == CommandCategories::SearchCateg ? Qt::Key_F2 : QKeySequence(), makeCommand(Commands::FromJapanese, categ), true, group);
    gUI->addCommandAction(signalmap, srcsubmenu, tr("%1 &to Japanese"), categ == CommandCategories::SearchCateg ? Qt::Key_F3 : QKeySequence(), makeCommand(Commands::ToJapanese, categ), true, group);
    if (categ == CommandCategories::SearchCateg)
        gUI->addCommandAction(signalmap, srcsubmenu, tr("&Browse Japanese"), Qt::Key_F4, makeCommand(Commands::BrowseJapanese, categ), true, group);

    //menu->addSeparator();

    srcsubmenu = menu->addMenu(tr("&Options"));

    if (categ == CommandCategories::SearchCateg)
    {
        gUI->addCommandAction(signalmap, srcsubmenu, tr("E&xample sentences"), Qt::Key_F5, makeCommand(Commands::ToggleExamples, categ), true);

        srcsubmenu->addSeparator();
    }

    gUI->addCommandAction(signalmap, srcsubmenu, tr("Any &start of word"), categ == CommandCategories::SearchCateg ? Qt::Key_F6 : QKeySequence(), makeCommand(Commands::ToggleAnyStart, categ), true);
    gUI->addCommandAction(signalmap, srcsubmenu, tr("Any &end of word"), categ == CommandCategories::SearchCateg ? Qt::Key_F7 : QKeySequence(), makeCommand(Commands::ToggleAnyEnd, categ), true);

    srcsubmenu->addSeparator();

    gUI->addCommandAction(signalmap, srcsubmenu, tr("&Deinflect search"), categ == CommandCategories::SearchCateg ? Qt::Key_F8 : QKeySequence(), makeCommand(Commands::ToggleDeinflect, categ), true);
    gUI->addCommandAction(signalmap, srcsubmenu, tr("Strict &match"), categ == CommandCategories::SearchCateg ? Qt::Key_F9 : QKeySequence(), makeCommand(Commands::ToggleStrict, categ), true);

    srcsubmenu->addSeparator();

    gUI->addCommandAction(signalmap, srcsubmenu, tr("&Filtering"), categ == CommandCategories::SearchCateg ? QKeySequence(tr("Ctrl+F")) : QKeySequence(), makeCommand(Commands::ToggleFilter, categ), true);
    gUI->addCommandAction(signalmap, srcsubmenu, tr("Edi&t filters"), categ == CommandCategories::SearchCateg ? QKeySequence(tr("Ctrl+Shift+F")) : QKeySequence(), makeCommand(Commands::EditFilters, categ));

    srcsubmenu->addSeparator();

    gUI->addCommandAction(signalmap, srcsubmenu, tr("Se&parate meanings"), categ == CommandCategories::SearchCateg ? Qt::Key_F10 : QKeySequence(), makeCommand(Commands::ToggleMultiline, categ), true);

    menu->addSeparator();

    //srcsubmenu = menu->addMenu(tr("Words"));

    gUI->addCommandAction(signalmap, menu, tr("Add word to &group..."), categ == CommandCategories::SearchCateg ? QKeySequence(tr("Ctrl+Enter")) : QKeySequence(), makeCommand(Commands::WordsToGroup, categ));
    gUI->addCommandAction(signalmap, menu, tr("Add word to &study deck..."), QKeySequence(), makeCommand(Commands::StudyWord, categ));
    gUI->addCommandAction(signalmap, menu, tr("Add word to &dictionary..."), QKeySequence(), makeCommand(Commands::WordToDict, categ));
    menu->addSeparator();
    gUI->addCommandAction(signalmap, menu, tr("&Edit word..."), QKeySequence(), makeCommand(Commands::EditWord, categ));
    gUI->addCommandAction(signalmap, menu, tr("De&lete word"), QKeySequence(), makeCommand(Commands::DeleteWord, categ));
    gUI->addCommandAction(signalmap, menu, tr("&Revert to original"), QKeySequence(), makeCommand(Commands::RevertWord, categ));

    menu->addSeparator();
    if (menu == wordsearchmenu)
    {
        gUI->addCommandAction(signalmap, menu, tr("&New word to dictionary..."), QKeySequence(), makeCommand(Commands::CreateNewWord, categ));
        menu->addSeparator();
    }

    srcsubmenu = menu->addMenu(tr("&Clipboard"));

    gUI->addCommandAction(signalmap, srcsubmenu, tr("&Copy word"), QKeySequence(), makeCommand(Commands::CopyWord, categ));
    srcsubmenu->addSeparator();
    gUI->addCommandAction(signalmap, srcsubmenu, tr("Copy &word written form"), QKeySequence(), makeCommand(Commands::CopyWordKanji, categ));
    gUI->addCommandAction(signalmap, srcsubmenu, tr("&Append word written form"), QKeySequence(), makeCommand(Commands::AppendWordKanji, categ));
    srcsubmenu->addSeparator();
    gUI->addCommandAction(signalmap, srcsubmenu, tr("C&opy word kana form"), QKeySequence(), makeCommand(Commands::CopyWordKana, categ));
    gUI->addCommandAction(signalmap, srcsubmenu, tr("A&ppend word kana"), QKeySequence(), makeCommand(Commands::AppendWordKana, categ));
    srcsubmenu->addSeparator();
    gUI->addCommandAction(signalmap, srcsubmenu, tr("Cop&y word definition"), QKeySequence(), makeCommand(Commands::CopyWordDef, categ));
}

void ZKanjiForm::retranslateSearchMenu(QMenu *menu, CommandCategories categ, int from)
{
    QAction *srcsubmenu = menu->actions().at(from + 0);
    srcsubmenu->setText(tr("&Mode"));

    QAction *a = srcsubmenu->menu()->actions().at(0);
    a->setText(tr("&Japanese to %1"));
    a->setShortcut(categ == CommandCategories::SearchCateg ? Qt::Key_F2 : QKeySequence());

    a = srcsubmenu->menu()->actions().at(1);
    a->setText(tr("%1 &to Japanese"));
    a->setShortcut(categ == CommandCategories::SearchCateg ? Qt::Key_F3 : QKeySequence());
    if (categ == CommandCategories::SearchCateg)
    {
        a = srcsubmenu->menu()->actions().at(2);
        a->setText(tr("&Browse Japanese"));
        a->setShortcut(Qt::Key_F4);
    }

    srcsubmenu = menu->actions().at(from + 1);
    srcsubmenu->setText(tr("&Options"));


    int skip = 0;
    if (categ == CommandCategories::SearchCateg)
    {
        a = srcsubmenu->menu()->actions().at(0);
        a->setText(tr("E&xample sentences"));

        skip = 2;
    }

    a = srcsubmenu->menu()->actions().at(skip + 0);
    a->setText(tr("Any &start of word"));
    a = srcsubmenu->menu()->actions().at(skip + 1);
    a->setText(tr("Any &end of word"));

    // srcsubmenu action skip + 2 is separator

    a = srcsubmenu->menu()->actions().at(skip + 3);
    a->setText(tr("&Deinflect search"));
    a = srcsubmenu->menu()->actions().at(skip + 4);
    a->setText(tr("Strict &match"));

    // srcsubmenu action skip + 5 is separator

    a = srcsubmenu->menu()->actions().at(skip + 6);
    a->setText(tr("&Filtering"));
    a = srcsubmenu->menu()->actions().at(skip + 7);
    a->setText(tr("Edi&t filters"));

    // srcsubmenu action skip + 8 is separator

    a = srcsubmenu->menu()->actions().at(skip + 9);
    a->setText(tr("Se&parate meanings"));

    // menu action from + 2 is separator

    a = menu->actions().at(from + 3);
    a->setText(tr("Add word to &group..."));
    a = menu->actions().at(from + 4);
    a->setText(tr("Add word to &study deck..."));
    a = menu->actions().at(from + 5);
    a->setText(tr("Add word to &dictionary..."));

    // menu action from + 6 is separator

    a = menu->actions().at(from + 7);
    a->setText(tr("&Edit word..."));
    a = menu->actions().at(from + 8);
    a->setText(tr("De&lete word"));
    a = menu->actions().at(from + 9);
    a->setText(tr("&Revert to original"));

    // menu action from + 10 is separator

    if (menu == wordsearchmenu)
    {
        a = menu->actions().at(from + 11);
        a->setText(tr("&New word to dictionary..."));
        from += 2;
    }

    srcsubmenu = menu->actions().at(from + 11);
    srcsubmenu->setText(tr("&Clipboard"));

    a = srcsubmenu->menu()->actions().at(0);
    a->setText(tr("&Copy word"));

    // srcsubmenu action 1 is separator

    a = srcsubmenu->menu()->actions().at(2);
    a->setText(tr("Copy &word written form"));
    a = srcsubmenu->menu()->actions().at(3);
    a->setText(tr("&Append word written form"));

    // srcsubmenu action 4 is separator

    a = srcsubmenu->menu()->actions().at(5);
    a->setText(tr("C&opy word kana form"));
    a = srcsubmenu->menu()->actions().at(6);
    a->setText(tr("A&ppend word kana"));

    // srcsubmenu action 7 is separator

    a = srcsubmenu->menu()->actions().at(8);
    a->setText(tr("Cop&y word definition"));
}

void ZKanjiForm::fillKanjiMenu(QSignalMapper *signalmap, QMenu *menu, CommandCategories categ)
{
    gUI->addCommandAction(signalmap, menu, tr("Kanji &information"), QKeySequence(), makeCommand(Commands::ShowKanjiInfo, categ));
    menu->addSeparator();
    gUI->addCommandAction(signalmap, menu, tr("Add kanji to &group..."), QKeySequence(), makeCommand(Commands::KanjiToGroup, categ));
    gUI->addCommandAction(signalmap, menu, tr("Collect kanji &words..."), QKeySequence(), makeCommand(Commands::CollectKanjiWords, categ));
    menu->addSeparator();
    gUI->addCommandAction(signalmap, menu, tr("&Edit kanji definition..."), QKeySequence(), makeCommand(Commands::EditKanjiDef, categ));
    menu->addSeparator();
    gUI->addCommandAction(signalmap, menu, tr("&Copy to clipboard"), QKeySequence(), makeCommand(Commands::CopyKanji, categ));
    gUI->addCommandAction(signalmap, menu, tr("A&ppend to clipboard"), QKeySequence(), makeCommand(Commands::AppendKanji, categ));
}


void ZKanjiForm::retranslateKanjiMenu(QMenu *menu, CommandCategories /*categ*/, int from)
{
    QAction *a = menu->actions().at(from + 0);
    a->setText(tr("Kanji &information"));

    // menu action from + 1 is separator

    a = menu->actions().at(from + 2);
    a->setText(tr("Add kanji to &group..."));
    a = menu->actions().at(from + 3);
    a->setText(tr("Collect kanji &words..."));

    // menu action from + 4 is separator

    a = menu->actions().at(from + 5);
    a->setText(tr("&Edit kanji definition..."));

    // menu action from + 6 is separator

    a = menu->actions().at(from + 7);
    a->setText(tr("&Copy to clipboard"));
    a = menu->actions().at(from + 8);
    a->setText(tr("A&ppend to clipboard"));

}


bool ZKanjiForm::validDockPos(QPoint pos)
{
#ifdef _DEBUG
    if (!mainform)
        throw "Only main forms have dock positions.";
#endif

    return centralWidget()->rect().contains(centralWidget()->mapFromGlobal(pos));
}

void ZKanjiForm::dockAt(QPoint lpos, ZKanjiForm *what)
{
#ifdef _DEBUG
    if (!mainform)
        throw "Only main forms can be dock targets.";
#endif

    QWidget *cw = centralWidget();
    QWidget *w = getDockPosOwnerAt(lpos);
    QPoint wpt = w->mapFrom(cw, lpos);
    QRect wr = w->rect();

    if (wpt.x() < 0)
        wpt.setX(0);
    if (wpt.x() > wr.width())
        wpt.setX(wr.width());
    if (wpt.y() < 0)
        wpt.setY(0);
    if (wpt.y() > wr.height())
        wpt.setY(wr.height());

    bool toleft = wpt.x() < wr.width() / 2.0;
    bool totop = wpt.y() < wr.height() / 2.0;
    double horzrate = double(toleft ? wpt.x() : std::max(0, wr.width() - wpt.x())) / (wr.width() / 2.0);
    double vertrate = double(totop ? wpt.y() : std::max(0, wr.height() - wpt.y())) / (wr.height() / 2.0);

    bool nearhorz = false;
    bool nearvert = false;

    // Number of pixels distance to an edge. When dragging a dock window closer than that, it
    // should be docked at the edge of the parent splitter.
    // TODO: change the distance of near edge detection for docking depending on interface
    // scaling.
    const int neardist = 32;

    if (horzrate < vertrate)
        nearhorz = ((toleft ? wpt.x() : wr.width() - wpt.x()) < neardist);
    else
        nearvert = ((totop ? wpt.y() : wr.height() - wpt.y()) < neardist);

    QSplitter *splitter = nullptr;
    if (dynamic_cast<QSplitter*>(w) != nullptr)
        splitter = (QSplitter*)w;
    else
        splitter = dynamic_cast<QSplitter*>(w->parent());

    ZKanjiWidget *kanjiwidget = nullptr;
    if (what != nullptr)
    {
#ifdef _DEBUG
        kanjiwidget = dynamic_cast<ZKanjiWidget*>(what->centralWidget()->layout()->itemAt(0)->widget());
        if (kanjiwidget == nullptr)
            throw "Passed form must directly hold a ZKanjiWidget object.";
#else
        kanjiwidget = (ZKanjiWidget*)what->centralWidget()->layout()->itemAt(0)->widget();
        kanjiwidget->layout()->setContentsMargins(Settings::scaled(2), Settings::scaled(2), Settings::scaled(2), Settings::scaled(0));
#endif
    }


    // Splitter orientation is horizontal?
    bool horz = splitter != nullptr && splitter->orientation() == Qt::Horizontal;

    // The dock window can be inserted into the splitter if the cursor is over an area of a
    // ZKanjiWidget matching the splitter orientation, when the cursor is on a splitter
    // handle, or the cursor is on an outside edge on one end of the splitter.
    // It's easier to check the opposite. When the cursor is not on a "bad" side of a widget
    // or the splitter, it automatically means insertion.
    if (splitter != nullptr && ((splitter != w && ((horz && (horzrate < vertrate)) || (!horz && (horzrate >= vertrate)))) || (horz && nearhorz) || (!horz && nearvert)))
    {
        // The dock position is at either end of the splitter.
        if ((w == splitter && (nearvert || nearhorz)) || (w == splitter->widget(0) && ((horz && toleft) || (!horz && totop))) || (w == splitter->widget(splitter->count() - 1) && ((horz && !toleft) || (!horz && !totop))))
        {
            bool front = (horz && toleft) || (!horz && totop);
            if (what == nullptr)
            {
                if (overlay == nullptr)
                    overlay = new ZDockOverlay(cw);
                if (front)
                    w = splitter->widget(0);
                else
                    w = splitter->widget(splitter->count() - 1);

                wr = w->rect();
                QRect lr = QRect(w->mapTo(cw, wr.topLeft()), w->mapTo(cw, wr.bottomRight()));

                if (horz)
                {
                    if (!toleft)
                        lr.setLeft(lr.left() + 2 * wr.width() / 3.0);
                    else
                        lr.setWidth(wr.width() / 3.0);
                }
                else
                {
                    if (!totop)
                        lr.setTop(lr.top() + 2 * wr.height() / 3.0);
                    else
                        lr.setHeight(wr.height() / 3.0);
                }
                overlay->setGeometry(lr);
                overlay->show();
            }
            else
            {
                hideDockOverlay();

                if (front)
                    splitter->insertWidget(0, kanjiwidget);
                else
                    splitter->addWidget(kanjiwidget);
            }
        }
        else
        {
            // Find the insert position for the kanji widget.
            int insix = 0;

            if (w != splitter)
            {
                insix = splitter->indexOf(w);
                if ((horz && !toleft) || (!horz && !totop))
                    ++insix;
            }
            else
            {
                // The cursor is on a splitter handle. The only way to find the index of the
                // handle is by walking through every handle and checking their rectangles.

                for (int ix = 1; ix != splitter->count(); ++ix)
                {
                    QSplitterHandle *h = splitter->handle(ix);
                    wr = h->rect();
                    QRect lr = QRect(h->mapTo(cw, wr.topLeft()), h->mapTo(cw, wr.bottomRight()));
                    if (lr.contains(lpos))
                    {
                        insix = ix;
                        break;
                    }
                }
            }
#ifdef _DEBUG
            if (insix == 0)
                throw "Cannot find the insert position.";
#else
            if (insix == 0)
            {
                hideDockOverlay();
                return;
            }
#endif
            if (what == nullptr)
            {
                if (overlay == nullptr)
                    overlay = new ZDockOverlay(cw);

                w = splitter->widget(insix - 1);
                wr = w->rect();

                QRect lr = QRect(w->mapTo(cw, wr.topLeft()), w->mapTo(cw, wr.bottomRight()));

                if (horz)
                    lr.setLeft(lr.left() + 2 * lr.width() / 3.0);
                else
                    lr.setTop(lr.top() + 2 * lr.height() / 3.0);

                w = splitter->widget(insix);
                wr = w->rect();
                wr = QRect(w->mapTo(cw, wr.topLeft()), w->mapTo(cw, wr.bottomRight()));

                if (horz)
                    lr.setRight(wr.left() + wr.width() / 3.0);
                else
                    lr.setBottom(wr.top() + wr.height() / 3.0);

                overlay->setGeometry(lr);
                overlay->show();
            }
            else
            {
                hideDockOverlay();
                splitter->insertWidget(insix, kanjiwidget);
            }
        }

        if (what != nullptr)
            updateMainMenu();

        return;
    }

    // Even if the cursor was over a ZKanjiWidget, when it is near the long edge of the
    // splitter parent, we want to dock next to the splitter instead of the widget.
    if (splitter != nullptr && w != splitter && ((horz && nearvert) || (!horz && nearhorz)))
    {
        w = splitter;
        wr = w->rect();
    }

    // If ZKanjiWidget has a splitter parent we can dock another widget beside it, when we
    // neared to the opposite orientation than what the parent has.
    if (splitter != nullptr && w != splitter && ((!horz && horzrate < vertrate) || (horz && horzrate >= vertrate)))
    {
        QRect lr = QRect(w->mapTo(cw, wr.topLeft()), w->mapTo(cw, wr.bottomRight()));

        if (what == nullptr)
        {
            if (overlay == nullptr)
                overlay = new ZDockOverlay(cw);

            overlay->show();
            if (!horz)
            {
                lr.setWidth(lr.width() / 2.0);
                if (!toleft)
                    lr.adjust(lr.width(), 0, lr.width(), 0);
            }
            else
            {
                lr.setHeight(lr.height() / 2.0);
                if (!totop)
                    lr.adjust(0, lr.height(), 0, lr.height());
            }

            overlay->setGeometry(lr);
            overlay->show();
        }
        else
        {
            hideDockOverlay();

            int wix = splitter->indexOf(w);
            QSplitter *s = new QSplitter;
            //s->setHandleWidth(1);
            s->setHandleWidth(Settings::scaled(s->handleWidth() * 0.8));

            s->setChildrenCollapsible(false);
            if (horz)
                s->setOrientation(Qt::Vertical);
            QSizePolicy pol = s->sizePolicy();
            pol.setHorizontalStretch(1);
            s->setSizePolicy(pol);

            if ((!horz && toleft) || (horz && totop))
                s->addWidget(kanjiwidget);
            s->addWidget(w);
            if ((!horz && !toleft) || (horz && !totop))
                s->addWidget(kanjiwidget);

            splitter->insertWidget(wix, s);
        }

        if (what != nullptr)
            updateMainMenu();

        return;
    }


    // No splitter is only possible if a ZKanjiWidget is directly on the centralWidget(). Any
    // side of the widget is a possible dock target. For a splitter, if it's directly on the
    // main form and something was dragged near its edge on one of the long sides, the dock
    // target is that side.
    if (splitter == nullptr || (w->parent() == cw && ((horz && nearvert) || (!horz && nearhorz))))
    {
        QRect lr = QRect(w->mapTo(cw, wr.topLeft()), w->mapTo(cw, wr.bottomRight()));

        if (what == nullptr)
        {
            if (overlay == nullptr)
                overlay = new ZDockOverlay(cw);
            if (horzrate < vertrate)
                overlay->setGeometry(toleft ? lr.left() : lr.left() + lr.width() / 2.0, lr.top(), lr.width() / 2.0, lr.height());
            else
                overlay->setGeometry(lr.left(), totop ? lr.top() : lr.top() + lr.height() / 2.0, lr.width(), lr.height() / 2.0);

            overlay->show();
        }
        else
        {
            hideDockOverlay();

            if (splitter != nullptr)
                w = splitter;
            else
                w->layout()->setContentsMargins(Settings::scaled(2), Settings::scaled(2), Settings::scaled(2), Settings::scaled(0));

            QHBoxLayout *l = (QHBoxLayout*)cw->layout();
            splitter = new QSplitter(cw);

            splitter->setHandleWidth(Settings::scaled(splitter->handleWidth() * 0.8));

            splitter->setChildrenCollapsible(false);
            QSizePolicy pol = splitter->sizePolicy();
            pol.setHorizontalStretch(1);
            splitter->setSizePolicy(pol);

            QLayoutItem *li = l->replaceWidget(w, splitter, Qt::FindDirectChildrenOnly);
            delete li;

            if (horzrate < vertrate)
            {
                if (toleft)
                    splitter->addWidget(kanjiwidget);
                splitter->addWidget(w);
                if (!toleft)
                    splitter->addWidget(kanjiwidget);
            }
            else
            {
                splitter->setOrientation(Qt::Vertical);
                if (totop)
                    splitter->addWidget(kanjiwidget);
                splitter->addWidget(w);
                if (!totop)
                    splitter->addWidget(kanjiwidget);
            }

        }

        if (what != nullptr)
            updateMainMenu();

        return;
    }

    if (what != nullptr)
        updateMainMenu();
}

void ZKanjiForm::hideDockOverlay()
{
#ifdef _DEBUG
    if (!mainform)
        throw "Only main forms can be dock targets.";
#endif

    if (overlay == nullptr)
        return;
    overlay->hide();
    overlay->deleteLater();
    overlay = nullptr;
}

QWidget* ZKanjiForm::getDockPosOwnerAt(QPoint lpos)
{
#ifdef _DEBUG
    if (!mainform)
        throw "Only main forms can be dock targets.";
#endif

    QLayout *cl = centralWidget()->layout();
    if (dynamic_cast<ZKanjiWidget*>(cl->itemAt(0)->widget()) != nullptr)
        return cl->itemAt(0)->widget();

    // The child of the centralWidget can only be a QSplitter if it's not a ZKanjiWidget.
    QSplitter *s = (QSplitter*)cl->itemAt(0)->widget();

    for (int ix = 0; ix != s->count(); ++ix)
    {
        QRect r = s->widget(ix)->geometry();

        if (QRect(s->mapTo(centralWidget(), r.topLeft()), s->mapTo(centralWidget(), r.bottomRight())).contains(lpos))
        {
            // If the rectangle of the widget at ix contains lpos, and it's a ZKanjiWidget, it
            // can be returned. Otherwise it's a QSplitter.
            if (dynamic_cast<ZKanjiWidget*>(s->widget(ix)) != nullptr)
                return s->widget(ix);

            s = (QSplitter*)s->widget(ix);
            ix = -1;
        }
    }

    // The coordinates were on a grab handle between widgets in the splitter.
    return s;
}


//-------------------------------------------------------------

