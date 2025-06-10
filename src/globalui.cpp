/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QFileDialog>
#include <QWindow>
#include <QClipboard>
#include <QStringBuilder>
#include <QFormLayout>
#include <QSplitter>
#include <QDesktopWidget>
#include <QScreen>

#include "globalui.h"
#include "zui.h"
#include "zkanalineedit.h"
#include "zkanjimain.h"
#include "zevents.h"
#include "zkanjiform.h"
#include "worddeckform.h"
#include "groupexportform.h"
#include "groupimportform.h"
#include "dictionaryexportform.h"
#include "dictionaryimportform.h"
#include "settingsform.h"
#include "words.h"
#include "worddeck.h"
#include "groups.h"
#include "popupdict.h"
#include "popupkanjisearch.h"
#include "settings.h"
#include "generalsettings.h"
#include "shortcutsettings.h"
#include "datasettings.h"
#include "kanjiinfoform.h"
#include "zkanjiwidget.h"
#include "fontsettings.h"
#include "import.h"
#include "sentences.h"
#include "importreplaceform.h"
#include "dialogs.h"
#include "grouppickerform.h"
#include "colorsettings.h"
#include "wordtogroupform.h"
#include "wordtodictionaryform.h"
#include "languages.h"
#include "languagesettings.h"

//// Mode button icon image width.
//static const int _iconW = 16;
//// Mode button icon image height.
//static const int _iconH = 16;


//TODO: put the global shortcut code into settings loading. Make sure the correct files are added to the project in their OS.
// Documentation for QxtGlobalShortcut: http://libqxt.bitbucket.org/doc/tip/qxtglobalshortcut.html

#include "Qxt/qxtglobalshortcut.h"
namespace
{
    QxtGlobalShortcut *fromPopupDictShortcut = nullptr;
    QxtGlobalShortcut *toPopupDictShortcut = nullptr;
    QxtGlobalShortcut *kanjiPopupShortcut = nullptr;
}

extern char ZKANJI_PROGRAM_VERSION[];

//-------------------------------------------------------------


HideAppWindowsGuard::HideAppWindowsGuard() : released(false)
{
    gUI->hideAppWindows();
}
HideAppWindowsGuard::~HideAppWindowsGuard()
{
    release();
}

void HideAppWindowsGuard::release()
{
    if (released)
        return;
    released = true;
    gUI->showAppWindows();
}


//-------------------------------------------------------------
AutoSaveGuard::AutoSaveGuard() : released(false)
{
    gUI->disableAutoSave();
}

AutoSaveGuard::~AutoSaveGuard()
{
    release();
}

void AutoSaveGuard::release()
{
    if (released)
        return;
    released = true;
    gUI->enableAutoSave();
}


//-------------------------------------------------------------


GlobalUI* GlobalUI::i = nullptr;
//QSignalMapper* GlobalUI::dictmap = nullptr;
//std::vector<ZKanjiForm*> GlobalUI::mainforms;

GlobalUI* GlobalUI::instance()
{
    if (i == nullptr)
    {
        i = new GlobalUI(qApp);
        //initDictionaryActions();
    }
    return i;
}

GlobalUI::GlobalUI(QObject *parent) : base(parent), kanjiinfo(nullptr), infoblock(0), dockform(nullptr), hiddencounter(0), autosavecounter(0), lastworddict(nullptr), lastworddiag(LastWordDialog::Group)
{
    if (i != nullptr)
        throw "Code should only contain a single instance of this.";
    i = this;

    //connect(&ZKanji::wordfilters(), &WordAttributeFilterList::filterErased, this, &GlobalUI::wordFilterErased);

    //connect(qApp, &QApplication::applicationStateChanged, this, &GlobalUI::appStateChanged);
    connect(qApp, &QApplication::paletteChanged, this, &GlobalUI::applySettings);
    connect(qApp, &QApplication::focusChanged, this, &GlobalUI::appFocusChanged);
    connect(qApp, &QApplication::aboutToQuit, this, &GlobalUI::saveBeforeQuit);
    qApp->installEventFilter(this);

    uiTimer.start(2000, this);

    connect(&popupmap, &QSignalMapper::mappedInt, this, &GlobalUI::showPopup);
}

GlobalUI::~GlobalUI()
{
    i = nullptr;
}

void GlobalUI::loadXMLWindow(bool ismain, QXmlStreamReader &reader)
{
    if (ismain && !mainforms.empty())
        reader.raiseError("MultipleMainWindowStates");
    if (!ismain && mainforms.empty())
        reader.raiseError("MainWindowStateMissing");

    if (reader.hasError())
        return;

    ZKanjiForm *f = new ZKanjiForm(ismain, mainforms.empty() ? nullptr : mainforms.front());
    if (ismain)
        connect(f, &ZKanjiForm::stateChanged, this, &GlobalUI::mainStateChanged);
    f->loadXMLSettings(reader);

    if (reader.hasError())
    {
        f->deleteLater();
        return;
    }

    mainforms.push_back(f);
    i->connect(f, &QMainWindow::destroyed, i, &GlobalUI::formDestroyed);
    i->connect(f, &ZKanjiForm::activated, i, &GlobalUI::formActivated);
}

void GlobalUI::saveXMLKanjiInfo(QXmlStreamWriter &writer)
{
    QWidgetList wlist = qApp->topLevelWidgets();
    for (QWidget *w : wlist)
    {
        if (dynamic_cast<KanjiInfoForm*>(w) == nullptr || w->parentWidget() != mainForm())
            continue;

        KanjiInfoForm *f = (KanjiInfoForm*)w;

        writer.writeStartElement("KanjiInfo");
        f->saveXMLSettings(writer);
        writer.writeEndElement(); /* KanjiInfo */
    }
}

void GlobalUI::loadXMLKanjiInfo(QXmlStreamReader &reader)
{
    if (kanjiinfo)
    {
        disconnect(kanjiinfo, &KanjiInfoForm::destroyed, this, &GlobalUI::kanjiInfoDestroyed);
        disconnect(kanjiinfo, &KanjiInfoForm::formLock, this, &GlobalUI::kanjiInfoLock);
        kanjiinfo->deleteLater();
        kanjiinfo = nullptr;
    }

    while (reader.readNextStartElement())
    {
        if (reader.name() == "KanjiInfo")
        {
            KanjiInfoForm *f = new KanjiInfoForm((QWidget*)mainForm());
            if (f->loadXMLSettings(reader))
            {
                if (!f->locked())
                {
                    kanjiinfo = f;
                    connect(kanjiinfo, &KanjiInfoForm::destroyed, this, &GlobalUI::kanjiInfoDestroyed);
                    connect(kanjiinfo, &KanjiInfoForm::formLock, this, &GlobalUI::kanjiInfoLock);
                }
            }
            else
                delete f;
        }
        else
            reader.skipCurrentElement();
    }
}

void GlobalUI::saveXMLLastDecks(QXmlStreamWriter &writer)
{
    for (int ix = 0, siz = ZKanji::dictionaryCount(); ix != siz; ++ix)
    {
        Dictionary *d = ZKanji::dictionary(ix);
        writer.writeEmptyElement("Item");
        WordDeck *wd = d->wordDecks()->lastSelected();
        writer.writeAttribute("dictionary", d->name());
        writer.writeAttribute("deck", wd == nullptr ? "" : wd->getName());
    }
}

void GlobalUI::loadXMLLastDecks(QXmlStreamReader &reader)
{
    while (reader.readNextStartElement())
    {
        if (reader.name() == "Item")
        {
            int ix = ZKanji::dictionaryIndex(reader.attributes().value("dictionary").toString());
            if (ix == -1)
                continue;
            ZKanji::dictionary(ix)->wordDecks()->setLastSelected(reader.attributes().value("deck").toString());
        }

        reader.skipCurrentElement();
    }
}

void GlobalUI::saveXMLLastGroups(QXmlStreamWriter &writer)
{
    for (int ix = 0, siz = ZKanji::dictionaryCount(); ix != siz; ++ix)
    {
        Dictionary *d = ZKanji::dictionary(ix);
        writer.writeEmptyElement("Item");
        WordGroup *wg = d->wordGroups().lastSelected();
        KanjiGroup *kg = d->kanjiGroups().lastSelected();
        writer.writeAttribute("dictionary", d->name());
        writer.writeAttribute("wordgroup", wg == nullptr ? "" : wg->fullEncodedName());
        writer.writeAttribute("kanjigroup", kg == nullptr ? "" : kg->fullEncodedName());
    }
}

void GlobalUI::loadXMLLastGroups(QXmlStreamReader &reader)
{
    while (reader.readNextStartElement())
    {
        if (reader.name() == "Item")
        {
            int ix = ZKanji::dictionaryIndex(reader.attributes().value("dictionary").toString());
            if (ix == -1)
                continue;
            ZKanji::dictionary(ix)->wordGroups().setLastSelected(reader.attributes().value("wordgroup").toString());
            ZKanji::dictionary(ix)->kanjiGroups().setLastSelected(reader.attributes().value("kanjigroup").toString());
        }

        reader.skipCurrentElement();
    }
}

void GlobalUI::saveXMLLastSettings(QXmlStreamWriter &writer)
{
    writer.writeAttribute("worddestdict", lastworddict == nullptr ? "" : lastworddict->name());
    writer.writeAttribute("lastworddiag", lastworddiag == LastWordDialog::Group ? "group" : "dictionary");
}

void GlobalUI::loadXMLLastSettings(QXmlStreamReader &reader)
{
    if (reader.attributes().hasAttribute("worddestdict"))
    {
        QString val = reader.attributes().value("worddestdict").toString();
        int ix = ZKanji::dictionaryIndex(val);
        if (ix != -1)
            setLastWordDestination(ZKanji::dictionary(ix));
        else
            lastworddict = nullptr;
    }
    if (reader.attributes().hasAttribute("lastworddiag"))
    {
        QString val = reader.attributes().value("lastworddiag").toString().toLower();
        if (val == "dictionary")
            lastworddiag = LastWordDialog::Dictionary;
        else
            lastworddiag = LastWordDialog::Group;
    }

    reader.skipCurrentElement();
}

void GlobalUI::createWindow(bool ismain)
{
    if (ismain && !lastsave.isValid())
        lastsave = QDateTime::currentDateTimeUtc();

    if (ismain && !mainforms.empty())
    {
        // Main form already loaded from settings file. Show every hidden window.

        bool minimized = mainforms.at(0)->windowState().testFlag(Qt::WindowMinimized);

        // If main form is minimized at this point, it means it might have to be put to the
        // tray instead of showing.
        if (Settings::general.minimizebehavior != GeneralSettings::DefaultMinimize && QSystemTrayIcon::isSystemTrayAvailable() && minimized)
        {
            if (!mainforms.at(0)->isVisible())
            {
                // Because minimizeToTray() doesn't store hidden windows in appwin, it must
                // be done manually here.
                for (ZKanjiForm *w : mainforms)
                {
                    connect(w, &QWidget::destroyed, this, &GlobalUI::hiddenWindowDestroyed);
                    appwin.push_back(w);
                }
            }
            minimizeToTray();
        }
        else
        {
            for (ZKanjiForm *f : mainforms)
                f->show();
        }

        mainStateChanged(minimized);

        return;
    }

    ZKanjiForm *f = new ZKanjiForm(mainforms.empty(), mainforms.empty() ? nullptr : mainforms.front());
    if (mainforms.empty())
        connect(f, &ZKanjiForm::stateChanged, this, &GlobalUI::mainStateChanged);
    mainforms.push_back(f);
    i->connect(f, &QMainWindow::destroyed, i, &GlobalUI::formDestroyed);
    i->connect(f, &ZKanjiForm::activated, i, &GlobalUI::formActivated);

    f->show();

    // Notifying window that it should update its saved geometry, so it won't be moved to
    // the top left corner of the screen on next startup.
    qApp->postEvent(f, new StartEvent());
}

void GlobalUI::addCreatedWindow(ZKanjiForm *f)
{
    mainforms.push_back(f);
    i->connect(f, &QMainWindow::destroyed, i, &GlobalUI::formDestroyed);
    i->connect(f, &ZKanjiForm::activated, i, &GlobalUI::formActivated);

    // Notifying window that it should update its saved geometry, so it won't be moved to
    // the top left corner of the screen on next startup.
    qApp->postEvent(f, new StartEvent());
}

ZKanjiForm* GlobalUI::mainForm() const
{
    if (mainforms.empty())
        return nullptr;
    return mainforms.front();
}

QWidget* GlobalUI::activeMainForm() const
{
    // Program not initialized?
    if (mainforms.empty())
        return nullptr;

    if (qApp->activeModalWidget() != nullptr)
        return qApp->activeModalWidget();

    if (mainForm()->isVisible() && !mainForm()->windowState().testFlag(Qt::WindowMinimized))
        return mainForm();

    if (PopupDictionary::getInstance() != nullptr && PopupDictionary::getInstance()->isActiveWindow())
        return PopupDictionary::getInstance();

    if (PopupKanjiSearch::getInstance() != nullptr && PopupKanjiSearch::getInstance()->isActiveWindow())
        return PopupKanjiSearch::getInstance();

    return nullptr;
}

void GlobalUI::raiseAndActivate()
{
    restoreFromTray();
    //qApp->processEvents();

    QWidget *f = activeMainForm();

    if (f == nullptr)
    {
        QWindow *w = qApp->modalWindow();
        if (w != nullptr)
        {
            for (QWidget *ww : qApp->topLevelWidgets())
            {
                if (ww->windowHandle() == w)
                {
                    f = ww;
                    break;
                }
            }
        }


        if (f == nullptr && mainForm()->isVisible() && mainForm()->isEnabled())
            f = mainForm();

        if (f == nullptr)
        {
            for (QWidget *ww : qApp->topLevelWidgets())
            {
                if (!ww->isWindow() || !ww->isVisible() || f != nullptr)
                    continue;

                if (ww->parent() == nullptr)
                    f = ww;
            }
        }
    }

    if (f == nullptr)
        return;

    f->setWindowState((f->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    f->show();
    f->raise();
    f->activateWindow();
}

int GlobalUI::formCount() const
{
    return tosigned(mainforms.size());
}

ZKanjiForm* GlobalUI::forms(int ix) const
{
    return mainforms[ix];
}

bool GlobalUI::hasMainAccess() const
{
    return qApp->activeModalWidget() == nullptr && (mainForm()->isVisible() || (PopupDictionary::getInstance() != nullptr && PopupDictionary::getInstance()->isVisible()) || (PopupKanjiSearch::getInstance() != nullptr && PopupKanjiSearch::getInstance()->isVisible()));
}

ZKanjiForm* GlobalUI::dockForm() const
{
    return dockform;
}

Dictionary* GlobalUI::lastWordDestination() const
{
    return lastworddict;
}

void GlobalUI::setLastWordDestination(Dictionary *d)
{
    lastworddict = d;
}

void GlobalUI::wordToDestSelect(Dictionary *d, int windex, bool showmodal)
{
    if (lastworddiag == LastWordDialog::Group || ZKanji::dictionaryCount() == 1)
        wordToGroupSelect(d, windex, showmodal, true);
    else
        wordToDictionarySelect(d, windex, showmodal, true);
}

void GlobalUI::setLastWordToDialog(LastWordDialog last)
{
    lastworddiag = last;
}

void GlobalUI::startDockDrag(ZKanjiForm *form)
{
    dockform = form;
    dockstartpos = form->pos();
    dockform->setWindowOpacity(0.65);

    QWidgetList list = qApp->topLevelWidgets();
    for (QWidget *w : list)
    {
        if (!w->isVisible() || w == form || w == mainforms[0])
            continue;

        connect(w, &QWidget::destroyed, this, &GlobalUI::hiddenWindowDestroyed);
        appwin.push_back(w);
        w->hide();
    }

    mainforms[0]->setMouseTracking(true);
    mainforms[0]->grabMouse();
    mainforms[0]->grabKeyboard();
}

void GlobalUI::endDockDrag()
{
    mainforms[0]->releaseKeyboard();
    mainforms[0]->releaseMouse();
    mainforms[0]->setMouseTracking(false);

    if (dockform != nullptr)
    {
        if (dockform->findChild<ZKanjiWidget*>() == nullptr)
        {
            delete dockform;
            dockform = nullptr;
        }
        else
        {
            dockform->move(dockstartpos);
            dockform->setWindowOpacity(1.0);
        }
    }

    for (QWidget *w : appwin)
    {
        w->show();
        disconnect(w, &QWidget::destroyed, this, &GlobalUI::hiddenWindowDestroyed);
    }

    if (dockform)
    {
        dockform->raise();
        if (qApp->activeWindow() != nullptr)
            dockform->activateWindow();
    }
    appwin.clear();
    dockform = nullptr;
}

//QPixmap GlobalUI::flags(int index)
//{
//    if (flagimg.empty())
//    {
//        int cnt = ZKanji::dictionaryCount();
//        flagimg.resize(cnt);
//        for (int ix = 0; ix < cnt; ++ix)
//            flagimg[ix] = ZKanji::dictionaryFlag(QSize(_iconW, _iconH), ZKanji::dictionary(ix)->name(), Flags::Flag);
//    }
//
//    return flagimg[index];
//}

//WordDeckForm* GlobalUI::wordDeckForm(bool createshow, int dict) const
//{
//    if (worddeckform == nullptr && createshow)
//    {
//        worddeckform = new WordDeckForm(mainForm());
//        i->connect(worddeckform, &QMainWindow::destroyed, i, &GlobalUI::formDestroyed);
//        //i->connect(w, &WordDeckForm::activated, i, &GlobalUI::formActivated);
//
//        //gUI->addActionsToWidget(w);
//        if (dict != -1)
//            worddeckform->setDictionary(dict);
//        worddeckform->show();
//    }
//
//
//    return worddeckform;
//}

// TODO: implement GetDictEvent and optionally SetDictEvent for every top level widget which
// has a dictionary.
int GlobalUI::activeDictionaryIndex() const
{
    QWidget *w = qApp->focusWidget();

    while (w != nullptr)
    {
        GetDictEvent e;
        qApp->sendEvent(w, &e);

        if (e.result() != -1)
            return e.result();

        w = w->parentWidget();
    }

    return -1;
}

void GlobalUI::applySettings()
{
    checkColorTheme();
    //applyStyleSheet();
    emit settingsChanged();

    if (Settings::language != zLang->currentID())
        zLang->setLanguage(Settings::language);
}

void GlobalUI::applyStyleSheet()
{
    // Changing the qApp style sheet is VERY slow, so we only do it if it actually needs to change.

    static QColor oldbgcol = QColor();
    QColor bgcol = Settings::colors.lighttheme ? qApp->palette().color(QPalette::Active, QPalette::Base).darker(115) : qApp->palette().color(QPalette::Active, QPalette::Base).lighter(115);

    if (bgcol == oldbgcol)
        return;

    oldbgcol = bgcol;

    int scrollw = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    int scrollbtnw = scrollw;
    int scaledscrollw = Settings::scaled(scrollw);
    int scaledscrollbtnw = Settings::scaled(scrollbtnw);

    qApp->setStyleSheet(QString("QScrollBar:vertical { width: %1px; } QScrollBar:horizontal { height: %1px; } ").arg(scaledscrollw) %
        QString("QScrollBar::add-line:vertical { height: %1px; } QScrollBar::sub-line:vertical { height: %1px; } QScrollBar::add-line:horizontal { width: %1px; } QScrollBar::sub-line:horizontal { width: %1px; } ").arg(scaledscrollbtnw) %
        QString("QSplitter::handle { background-color: %1; } ").arg(bgcol.name()));
}

void GlobalUI::scaleWidget(QWidget *w)
{
    if (w == nullptr)
        return;

    _scaleWidget(w);

    // Recursion of child widgets is done in _scaleWidget()
}

void GlobalUI::preventWidgetScale(QWidget *w)
{
    if (w == nullptr)
        return;

    _registerWidgetScale(w);
}

void GlobalUI::clipCopy(const QString &str) const
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(str);
}

void GlobalUI::clipAppend(const QString &str) const
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(clipboard->text() + str);
}

void GlobalUI::showKanjiInfo(Dictionary *d, int index)
{
    if (infoblock != 0)
        return;

    if (kanjiinfo == nullptr)
    {
        kanjiinfo = new KanjiInfoForm((QWidget*)activeMainForm());
        kanjiinfo->popup(d, index);
        connect(kanjiinfo, &KanjiInfoForm::destroyed, this, &GlobalUI::kanjiInfoDestroyed);
        connect(kanjiinfo, &KanjiInfoForm::formLock, this, &GlobalUI::kanjiInfoLock);
        emit kanjiInfoShown();
    }
    else
        kanjiinfo->setKanji(d, index);
}

void GlobalUI::setInfoKanji(Dictionary *d, int index)
{
    if (kanjiinfo != nullptr)
        kanjiinfo->setKanji(d, index);
}

void GlobalUI::toggleKanjiInfo(bool toggleon)
{
    if (toggleon && infoblock == 0)
        return;

    infoblock += toggleon ? -1 : 1;
    if (infoblock > 1)
        return;

    kanjiinfo = nullptr;

    auto wlist = qApp->topLevelWidgets();
    for (QWidget *w : wlist)
    {
        if (!dynamic_cast<KanjiInfoForm*>(w) || w->parentWidget() != mainForm())
            continue;

        KanjiInfoForm *f = (KanjiInfoForm*)w;
        if (infoblock)
            f->hide();
        else
        {
            f->show();
            if (!f->locked())
                kanjiinfo = f;
        }
    }
}

bool GlobalUI::kanjiInfoAllowed() const
{
    return infoblock == 0;
}

QAction* GlobalUI::addCommandAction(QSignalMapper *map, QMenu *parentmenu, const QString &str, const QKeySequence &keyseq, int command, bool checkable, QActionGroup *group)
{
    return insertCommandAction(map, parentmenu, -1, str, keyseq, command, checkable, group);
}

QAction* GlobalUI::insertCommandAction(QSignalMapper *map, QMenu *parentmenu, int pos, const QString &str, const QKeySequence &keyseq, int command, bool checkable, QActionGroup *group)
{
    QAction *a = new QAction(str, i);
    if (!keyseq.isEmpty())
        a->setShortcut(keyseq);
    if (checkable)
        a->setCheckable(true);
    if (group != nullptr)
        a->setActionGroup(group);
    connect(a, &QAction::triggered, map, (void (QSignalMapper::*)())&QSignalMapper::map);
    map->setMapping(a, command);
    if (pos != -1 && pos < parentmenu->actions().size())
        parentmenu->insertAction(parentmenu->actions().at(pos), a);
    else
        parentmenu->addAction(a);
    return a;
}

void GlobalUI::minimizeToTray()
{
    if (trayicon || hiddencounter != 0 || !QSystemTrayIcon::isSystemTrayAvailable())
        return;

    trayicon.reset(new QSystemTrayIcon);
    traymenu.reset(new QMenu);

    //emit appWindowsVisibilityChanged(false);

    qApp->setQuitOnLastWindowClosed(false);
    for (QWidget *w : qApp->topLevelWidgets())
    {
        if (!w->isVisible())
            continue;
        connect(w, &QWidget::destroyed, this, &GlobalUI::hiddenWindowDestroyed);
        appwin.push_back(w);
        w->hide();
    }

    connect(trayicon.get(), &QSystemTrayIcon::activated, this, &GlobalUI::trayActivate);

    QAction *a = traymenu->addAction(tr("Japanese to %1").arg(ZKanji::dictionary(PopupDictionary::dictionaryIndex())->name()));
    connect(a, &QAction::triggered, &popupmap, (void (QSignalMapper::*)())&QSignalMapper::map);
    popupmap.setMapping(a, 0);

    a = traymenu->addAction(tr("%1 to Japanese").arg(ZKanji::dictionary(PopupDictionary::dictionaryIndex())->name()));
    connect(a, &QAction::triggered, &popupmap, (void (QSignalMapper::*)())&QSignalMapper::map);
    popupmap.setMapping(a, 1);

    traymenu->addSeparator();

    a = traymenu->addAction(tr("Kanji search"));
    connect(a, &QAction::triggered, &popupmap, (void (QSignalMapper::*)())&QSignalMapper::map);
    popupmap.setMapping(a, 2);

    traymenu->addSeparator();

    a = traymenu->addAction(tr("Exit"));
    connect(a, &QAction::triggered, this, &GlobalUI::quit);

    trayicon->setContextMenu(traymenu.get());
    //#ifdef Q_OS_WIN
    //    trayicon->setIcon(QIcon(":/program.ico"));
    //#else
    trayicon->setIcon(QIcon(":/programico2.svg"));
    //#endif
    trayicon->setToolTip("zkanji");
    trayicon->show();
}

void GlobalUI::restoreFromTray()
{
    if (!trayicon)
        return;

    trayicon.reset();
    traymenu.reset();

    //for (QWidget *w : appwin)
    //    w->show();

    //qApp->processEvents();

    for (QWidget *w : appwin)
    {
        w->setWindowState((w->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        w->show();
        w->raise();
        w->activateWindow();
        disconnect(w, &QWidget::destroyed, this, &GlobalUI::hiddenWindowDestroyed);
    }

    qApp->setQuitOnLastWindowClosed(true);
    appwin.clear();

    mainforms.front()->moveToScreen(qApp->screens().indexOf(qApp->screenAt(QCursor::pos())));

    //emit appWindowsVisibilityChanged(true);
}

bool GlobalUI::isInTray() const
{
    return trayicon != nullptr;
}

bool GlobalUI::eventFilter(QObject *obj, QEvent *e)
{
    static bool closeeventing = false;

    if (e->type() == QEvent::Close && !closeeventing)
    {
        closeeventing = true;
        FlagGuard<bool> fg(&closeeventing, false);

        QCloseEvent *ce = (QCloseEvent*)e;
        ce->accept();

        if (dynamic_cast<QWidget*>(obj) != nullptr)
        {
            QWidget *w = (QWidget*)obj;
            QList<QWidget*> closed;
            closed.push_back(w);

            int first = 0;
            int last = 0;

            do
            {
                for (int ix = first; ix != last + 1; ++ix)
                {
                    w = closed[ix];
                    QList<QWidget*> cl = w->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
                    for (QWidget *ch : cl)
                    {
                        if (ch->windowFlags().testFlag(Qt::Window))
                            closed.push_back(ch);
                    }
                }
                first = last + 1;
                last = closed.size() - 1;
            } while (first <= last);

            for (int ix = closed.size() - 1; ix != 0; --ix)
            {
                qApp->sendEvent(closed[ix], ce);
                if (!ce->isAccepted())
                    return true;
            }

            // Every window below the window to be closed accepted the close event. Let's
            // close them for real to avoid the strange Qt behavior when a parent is closed
            // but the child windows are not.

            for (int ix = closed.size() - 1; ix != 0; --ix)
                closed[ix]->close();
        }
    }

    return base::eventFilter(obj, e);
}

void GlobalUI::signalDictionaryAdded()
{
    // Signalling dictionary actions can only occure after the program has started. The flag
    // images are not filled until that.
    //if (flagimg.empty())
    //    return;

    //while (flagimg.size() != ZKanji::dictionaryCount())
    //    flagimg.push_back(ZKanji::dictionaryFlag(QSize(_iconW, _iconH), ZKanji::dictionary(ZKanji::dictionaryCount() - 1)->name(), Flags::Flag));

    emit dictionaryAdded();
}

void GlobalUI::signalDictionaryToBeRemoved(int index, int orderindex, Dictionary *dict)
{
    //// Signalling dictionary actions can only occure after the program has started. The flag
    //// images are not filled until that.
    //if (flagimg.empty())
    //    return;

    if (lastworddict == dict)
        lastworddict = nullptr;

    ZKanji::eraseDictionaryFlag(dict->name());

    emit dictionaryToBeRemoved(index, orderindex, dict);
}

void GlobalUI::signalDictionaryRemoved(int index, int orderindex, void *oldaddress)
{
    //// Signalling dictionary actions can only occure after the program has started. The flag
    //// images are not filled until that.
    //if (flagimg.empty())
    //    return;

    //if (flagimg.size() != ZKanji::dictionaryCount())
    //    flagimg.erase(flagimg.begin() + index);

    emit dictionaryRemoved(index, orderindex, oldaddress);
}

void GlobalUI::signalDictionaryMoved(int from, int to)
{
    //// Signalling dictionary actions can only occure after the program has started. The flag
    //// images are not filled until that.
    //if (flagimg.empty())
    //    return;

    emit dictionaryMoved(from, to);
}

void GlobalUI::signalDictionaryRenamed(const QString &oldname, int index, int orderindex)
{
    //// Signalling dictionary actions can only occure after the program has started. The flag
    //// images are not filled until that.
    //if (flagimg.empty())
    //    return;

    ZKanji::changeDictionaryFlagName(oldname, ZKanji::dictionary(index)->name());

    emit dictionaryRenamed(oldname, index, orderindex);
}

void GlobalUI::signalDictionaryReplaced(Dictionary *olddict, Dictionary *newdict, int index)
{
    //// Signalling dictionary actions can only occure after the program has started. The flag
    //// images are not filled until that.
    //if (flagimg.empty())
    //    return;

    emit dictionaryReplaced(olddict, newdict, index);
}

void GlobalUI::signalDictionaryFlagChange(int index)
{
    emit dictionaryFlagChanged(index, ZKanji::dictionaryOrder(index));
}

void GlobalUI::timerEvent(QTimerEvent *e)
{
    base::timerEvent(e);

    if (e->timerId() != uiTimer.timerId())
        return;

    if (autosavecounter == 0 && Settings::data.autosave && lastsave.isValid())
    {
        QDateTime now = QDateTime::currentDateTimeUtc();
        if (lastsave.secsTo(now) >= Settings::data.interval * 60)
        {
            lastsave = now;
            ZKanji::saveUserData();
        }
    }
}

bool GlobalUI::event(QEvent *e)
{
    if (e->type() == StartEvent::Type())
    {
        ZKanji::backupUserData();
        return true;
    }
    return base::event(e);
}

#ifndef Q_OS_WIN
void GlobalUI::secondAppStarted()
{
    raiseAndActivate();
}
#endif

void GlobalUI::importBaseDict()
{
    NTFSPermissionGuard permissionguard;

    if ((QFileInfo(ZKanji::appFolder() + "/data/English.zkj").exists() && !QFileInfo(ZKanji::appFolder() + "/data/English.zkj").isWritable()) ||
        !QFileInfo(ZKanji::appFolder() + "/data").isWritable())
    {
        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("File permissions do not allow creating and writing data files in the program's data folder."));
        return;
    }

    if ((QFileInfo(ZKanji::userFolder() + "/data/English.zkdict").exists() && !QFileInfo(ZKanji::userFolder() + "/data/English.zkdict").isWritable()) || !QFileInfo(ZKanji::userFolder() + "/data").isWritable())
    {
        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("File permissions do not allow writing files in the user folder."));
        return;
    }

    if (QMessageBox::question(mainforms[0], "zkanji", tr("This operation will replace the current base dictionary. Changes by the user will be kept.\n\n"
        "You will be asked to select a folder with the JMdict data file. The folder must contain the file:\n"
        "JMdict or JMdict_e\n"
        "\n"
        "Do you wish to import?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
        return;
    QString path;

    do
    {
        path = QFileDialog::getExistingDirectory(activeMainForm(), tr("Select folder with dictionary file"), QString());
        if (path.isEmpty())
            return;
        if (path.at(path.size() - 1) == '/')
            path.resize(path.size() - 1);

        if ((!QFileInfo(path + "/JMdict").exists() || !QFileInfo(path + "/JMdict").isReadable()) &&
            (!QFileInfo(path + "/JMdict_e").exists() || !QFileInfo(path + "/JMdict_e").isReadable()))
        {
            path = QString();
            if (showAndReturn("zkanji", tr("The selected folder does not contain the required file or the file cannot be read."),
                tr("\"%1\" to select another folder with the dictionary file. If you \"%2\" the import will be aborted.").arg(tr("Retry")).arg(tr("Cancel")), {
                    { tr("Retry"), QMessageBox::AcceptRole },
                    { tr("Cancel"), QMessageBox::RejectRole } }) == 1)
                    return;
        }
    } while (path.isEmpty());

    HideAppWindowsGuard hideguard;
    AutoSaveGuard saveguard;

    DictImport diform;
    std::unique_ptr<Dictionary> importdict(diform.importDict(path, false));
    diform.hide();

    if (importdict.get() == nullptr)
    {
        hideguard.release();
        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The base dictionary hasn't been updated."));
        return;
    }

    Dictionary *dict = ZKanji::dictionary(0);

    ImportReplaceForm irf(dict, importdict.get());
    if (!irf.exec())
    {
        hideguard.release();
        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The base dictionary update was aborted."));
        return;
    }

    ZKanji::originals.swap(irf.originals());
    dict->swapDictionaries(importdict.get(), irf.changes());

    //// ONLY TEST: CHECKING WHETHER UNDO WORKS
    //ZKanji::originals.swap(irf.originals());
    //dict->restoreChanges(importdict.get());

    if (QFileInfo::exists(ZKanji::userFolder() + "/data/English.zkdict") && !QFile::remove(ZKanji::userFolder() + "/data/English.zkdict"))
    {
        ZKanji::originals.swap(irf.originals());
        dict->restoreChanges(importdict.get());

        saveguard.release();
        hideguard.release();
        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The base dictionary update failed because access to the user folder's data file was denied. No files were modified."));
        return;
    }

    if ((QFileInfo(ZKanji::appFolder() + "/data/English.zkj").exists() && !QFileInfo(ZKanji::appFolder() + "/data/English.zkj").isWritable()))
    {
        ZKanji::originals.swap(irf.originals());
        dict->restoreChanges(importdict.get());

        saveguard.release();
        hideguard.release();
        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The base dictionary update failed because access to the main dictionary file was denied. No files were modified."));
        return;
    }

    if (!dict->save(ZKanji::appFolder() + "/data/English.zkj"))
    {
        ZKanji::originals.swap(irf.originals());
        dict->restoreChanges(importdict.get());

        saveguard.release();
        hideguard.release();
        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The base dictionary update failed. Depending on the error, the base dictionary file might be compromised.") % QString("\n\n%1").arg(Error::last()));
        return;
    }

    if (!QFile::copy(ZKanji::appFolder() + "/data/English.zkj", ZKanji::userFolder() + "/data/English.zkdict"))
    {
        saveguard.release();
        hideguard.release();
        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("Error occurred while trying to copy base dictionary to user folder. To avoid corrupting the user data file, the program will terminate.") % QString("\n\n%1").arg(Error::last()));
        exit(0);
        return;
    }

    if (!dict->saveUserData(ZKanji::userFolder() + "/data/English.zkuser"))
    {
        saveguard.release();
        hideguard.release();
        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("Error occurred while trying to save user data for the base dictionary. Depending on the error, the user data file might be compromised.") % QString("\n\n%1").arg(Error::last()));
        return;
    }

    saveguard.release();
    hideguard.release();
    QMessageBox::information(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The base dictionary has been updated."));
}

void GlobalUI::importExamples()
{
    NTFSPermissionGuard permissionguard;

    if ((QFileInfo(ZKanji::appFolder() + "/data/examples.zkj").exists() && !QFileInfo(ZKanji::appFolder() + "/data/examples.zkj").isWritable()) ||
        !QFileInfo(ZKanji::appFolder() + "/data").isWritable())
    {
        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("File permissions do not allow creating and writing data files in the program's data folder."));
        return;
    }

    if (QMessageBox::question(mainforms[0], "zkanji", tr("This operation will replace the current example sentences data.\n\n"
        "You will be asked to select a folder with the unpacked example sentences file. The folder must contain the file:\n"
        "examples.utf\n"
        "\n"
        "Do you wish to import?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
        return;
    QString path;

    do
    {
        path = QFileDialog::getExistingDirectory(activeMainForm(), tr("Select folder with the example sentences file"), QString());
        if (path.isEmpty())
            return;
        if (path.at(path.size() - 1) == '/')
            path.resize(path.size() - 1);

        if (!QFileInfo(path + "/examples.utf").exists() || !QFileInfo(path + "/examples.utf").isReadable())
        {
            path = QString();
            if (showAndReturn("zkanji", tr("The selected folder does not contain the required file or the file cannot be read."),
                tr("\"%1\" to select another folder with the example sentences file. If you \"%2\" the import will be aborted.").arg(tr("Retry")).arg(tr("Cancel")), {
                    { tr("Retry"), QMessageBox::AcceptRole },
                    { tr("Cancel"), QMessageBox::RejectRole } }) == 1)
                    return;
        }
    } while (path.isEmpty());

    HideAppWindowsGuard hideguard;

    DictImport diform;
    if (!diform.importExamples(path, ZKanji::userFolder() + "/data/examples.zkj2", ZKanji::dictionary(0)))
    {
        if (QFileInfo().exists(ZKanji::userFolder() + "/data/examples.zkj2"))
            QFile::remove(ZKanji::userFolder() + "/data/examples.zkj2");

        diform.hide();

        ZKanji::commons.clearExamplesData();
        ZKanji::sentences.load(ZKanji::appFolder() + "/data/examples.zkj");

        hideguard.release();

        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The example sentences data wasn't updated."));
        return;
    }

    ZKanji::sentences.reset();
    if ((QFileInfo().exists(ZKanji::appFolder() + "/data/examples.zkj") && !QFile::remove(ZKanji::appFolder() + "/data/examples.zkj")) || !QFile::copy(ZKanji::userFolder() + "/data/examples.zkj2", ZKanji::appFolder() + "/data/examples.zkj"))
    {
        if (QFileInfo().exists(ZKanji::userFolder() + "/data/examples.zkj2"))
            QFile::remove(ZKanji::userFolder() + "/data/examples.zkj2");

        diform.hide();

        ZKanji::commons.clearExamplesData();
        ZKanji::sentences.load(ZKanji::appFolder() + "/data/examples.zkj");

        hideguard.release();

        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The example sentences data wasn't updated because access was denied to examples.zkj in the program data folder."));
        return;
    }

    ZKanji::sentences.load(ZKanji::appFolder() + "/data/examples.zkj");

    if (QFileInfo().exists(ZKanji::userFolder() + "/data/examples.zkj2") && !QFile::remove(ZKanji::userFolder() + "/data/examples.zkj2"))
    {
        diform.hide();

        emit sentencesReset();

        hideguard.release();

        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The example sentences data has been updated but the temporary examples.zkj2 couldn't be removed from the user data folder."));

        return;
    }

    diform.hide();

    emit sentencesReset();

    QMessageBox::information(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The example sentences data has been updated."));
}

void GlobalUI::importOldUserData()
{
    QString fname = QFileDialog::getOpenFileName(!mainforms.empty() ? mainforms[0] : nullptr, tr("Locate old user dictionary"), QString(), QString("%1 (*.zkd)").arg(tr("Old user dictionary")));
    if (fname.isEmpty())
        return;

    QFileInfo finf(fname);
    QString dictname = finf.baseName();
    QString groupname = fname.left(fname.size() - finf.suffix().size()) + "zkg";

    int dictix = -1;
    for (int ix = 0, siz = ZKanji::dictionaryCount(); ix != siz; ++ix)
    {
        if (ZKanji::dictionary(ix)->name().toLower() == dictname.toLower())
        {
            int r = showAndReturn("zkanji",
                ix == 0 ?
                tr("Warning: Importing user data for the main dictionary will reset all user changes to the dictionary and replace existing groups.") :
                tr("Warning: A dictionary with the same name already exists. The dictionary itself and all existing groups will be replaced."),
                tr("Choose \"%1\" to import the old data.").arg(qApp->translate("", "Continue")),
                {
                    { qApp->translate("", "Continue"), QMessageBox::AcceptRole },
                    { qApp->translate("", "Quit"), QMessageBox::RejectRole }
                });
            if (r == 1)
                return;
            dictix = ix;
            break;
        }
    }

    Dictionary *d = nullptr; 

    // Safe copy of dictionary to be updated, if it exists.
    std::unique_ptr<Dictionary> copy;
    // Safe copy of word originals if loading data for the main dictionary.
    OriginalWordsList oricopy;

    bool donedict = false;
    try
    {
        d = dictix == -1 ? ZKanji::addDictionary() : new Dictionary();

        if (dictix != -1)
        {
            // Create a safe copy to be restored in the event of an error or when cancelling.
            copy.reset(ZKanji::replaceDictionary(dictix, d));
            if (dictix == 0)
                oricopy.swap(ZKanji::originals);
        }

        d->loadFile(fname, dictix == 0, dictix != 0);
        donedict = true;
        d->loadUserDataFile(groupname, dictix != 0);

        if (dictix == 0)
        {
            ImportReplaceForm irf(d);
            if (irf.exec())
            {
                ZKanji::originals.swap(irf.originals());
                d->swapDictionaries(irf.dictionary(), irf.changes());
                signalDictionaryReplaced(copy.get(), d, dictix);
            }
            else
            {
                // Restore safe copy.
                d = ZKanji::replaceDictionary(dictix, copy.get());
                copy.release();
                copy.reset(d);
                if (dictix == 0)
                    oricopy.swap(ZKanji::originals);
            }
        }
        else if (dictix != -1)
            signalDictionaryReplaced(copy.get(), d, dictix);

    }
    catch (...)
    {
        if (dictix != -1)
        {
            // Restore safe copy.
            d = ZKanji::replaceDictionary(dictix, copy.get());
            copy.release();
            copy.reset(d);
            if (dictix == 0)
                oricopy.swap(ZKanji::originals);
        }

        if (!donedict)
            QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", qApp->translate("", "Error loading dictionary data: %1").arg(dictname));
        else
            QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", qApp->translate("", "Error loading user data for dictionary: %1").arg(dictname));

        if (dictix == -1)
            ZKanji::deleteDictionary(ZKanji::dictionaryCount() - 1);

        return;
    }

    d->setToModified();
    d->setToUserModified();

    if (dictix == -1)
        gUI->signalDictionaryRenamed(QString(), ZKanji::dictionaryCount() - 1, ZKanji::dictionaryOrder(ZKanji::dictionaryCount() - 1));
}

void GlobalUI::userExportAction()
{
    GetDictEvent d;

    qApp->sendEvent(qApp->activeWindow(), &d);

    if (d.result() == -1)
        return;

    GroupExportForm f(!mainforms.empty() ? mainforms[0] : nullptr);
    f.exec(ZKanji::dictionary(d.result()));
}

void GlobalUI::userImportAction()
{
    GetDictEvent d;

    qApp->sendEvent(qApp->activeWindow(), &d);

    if (d.result() == -1)
        return;

    GroupImportForm f(!mainforms.empty() ? mainforms[0] : nullptr);
    if (!f.exec(ZKanji::dictionary(d.result())))
        return;

    QString fname = QFileDialog::getOpenFileName(!mainforms.empty() ? mainforms[0] : nullptr, tr("Open export file"), QString(), QString("%1 (*.zkanji.export)").arg(tr("Export file")));
    if (fname.isEmpty())
        return;

    //f.dictionary()->importUserData(fname, f.kanjiCategory(), f.wordsCategory());


    HideAppWindowsGuard hideguard;
    AutoSaveGuard saveguard;

    if (f.dictionary()->isUserModified())
        f.dictionary()->saveUserData(ZKanji::userFolder() + "/data" + f.dictionary()->name() + ".zkuser");

    DictImport diform;
    bool result = diform.importUserData(fname, f.dictionary(), f.kanjiCategory(), f.wordsCategory(), f.importKanjiExamples(), f.importWordMeanings());
    diform.hide();

    if (!result)
        f.dictionary()->loadUserDataFile(ZKanji::userFolder() + "/data" + f.dictionary()->name() + ".zkuser", true);

    saveguard.release();
    hideguard.release();
    if (result)
        QMessageBox::information(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The user data has been updated."));
    else
        QMessageBox::information(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The user data import was aborted."));

}

void GlobalUI::dictExportAction()
{
    GetDictEvent d;

    qApp->sendEvent(qApp->activeWindow(), &d);

    if (d.result() == -1)
        return;

    DictionaryExportForm f(!mainforms.empty() ? mainforms[0] : nullptr);
    f.exec(ZKanji::dictionary(d.result()));
}

void GlobalUI::dictImportAction()
{
    GetDictEvent de;

    qApp->sendEvent(qApp->activeWindow(), &de);

    if (de.result() == -1)
        return;

    DictionaryImportForm frm(!mainforms.empty() ? mainforms[0] : nullptr);

    if (!frm.exec(ZKanji::dictionary(de.result())))
        return;

    QString dname = frm.dictionaryName();
    int dix = ZKanji::dictionaryIndex(dname);
    Dictionary *dict = nullptr;
    if (dix != -1)
        dict = ZKanji::dictionary(dix);

    bool full = frm.fullImport();

    WordGroup *wgroup = frm.selectedWordGroup();
    KanjiGroup *kgroup = frm.selectedKanjiGroup();

    QString fname = QFileDialog::getOpenFileName(!mainforms.empty() ? mainforms[0] : nullptr, tr("Open export file"), QString(), QString("%1 (*.zkanji.dictionary)").arg(tr("Dictionary export file")));
    if (fname.isEmpty())
        return;

    HideAppWindowsGuard hideguard;
    AutoSaveGuard saveguard;

    if (full)
    {
        DictImport diform;
        std::unique_ptr<Dictionary> importdict(diform.importFromExport(fname));
        diform.hide();

        if (importdict.get() == nullptr)
        {
            saveguard.release();
            hideguard.release();
            QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The dictionary import was aborted."));
            return;
        }

        if (dict == nullptr)
        {
            Dictionary *d = importdict.release();
            d->setName(dname);
            ZKanji::addDictionary(d);

            if (!d->save(ZKanji::userFolder() + QString("/data/%1.zkdict").arg(d->name())))
            {
                ZKanji::deleteDictionary(ZKanji::dictionaryIndex(d));
                saveguard.release();
                hideguard.release();
                QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The dictionary couldn't be saved. Make sure you have access to the zkanji user folder and there is enough disk space."));
                return;

            }
            saveguard.release();
            hideguard.release();
            QMessageBox::information(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The dictionary has been imported."));
            return;
        }

        ImportReplaceForm irf(dict, importdict.get());
        if (!irf.exec())
        {
            saveguard.release();
            hideguard.release();
            QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The dictionary update was aborted."));
            return;
        }

        dict->swapDictionaries(importdict.get(), irf.changes());
        dict->setName(dname);

        if (!dict->save(ZKanji::userFolder() + QString("/data/%1.zkdict").arg(dict->name())) || !dict->saveUserData(ZKanji::userFolder() + QString("/data/%1.zkuser").arg(dict->name())))
        {
            dict->restoreChanges(importdict.get());

            saveguard.release();
            hideguard.release();
            QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The dictionary or its user data file couldn't be saved. Depending on the error, the files might be compromised.") % QString("\n\n%1").arg(Error::last()));
            return;
        }

        saveguard.release();
        hideguard.release();
        QMessageBox::information(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The dictionary has been updated."));
        return;
    }

    DictImport diform;
    bool result = diform.importFromExportPartial(fname, dict, wgroup, kgroup);
    diform.hide();

    saveguard.release();
    hideguard.release();
    if (result)
    {
        if (!dict->save(ZKanji::userFolder() + QString("/data/%1.zkdict").arg(dict->name())) || !dict->saveUserData(ZKanji::userFolder() + QString("/data/%1.zkuser").arg(dict->name())))
        {
            QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The dictionary or its user data file couldn't be saved. Depending on the error, the files might be compromised.") % QString("\n\n%1").arg(Error::last()));
            return;
        }
        QMessageBox::information(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The dictionary has been updated."));
    }
    else
        QMessageBox::warning(!mainforms.empty() ? mainforms[0] : nullptr, "zkanji", tr("The dictionary update was aborted."));
}

void GlobalUI::saveUserData()
{
    ZKanji::saveUserData(true);
}

void GlobalUI::showSettingsWindow()
{
    SettingsForm *f = new SettingsForm(activeMainForm());
    f->showModal();
}

void GlobalUI::loadScalingSetting()
{
    Settings::loadScaleSettingFromFile();
}

void GlobalUI::saveSettingsAndStates()
{
    Settings::saveSettingsToFile();
}

void GlobalUI::loadSettings()
{
    checkColorTheme();
    Settings::loadSettingsFromFile();
}

void GlobalUI::loadStates()
{
    Settings::loadStatesFromFile();
}

void GlobalUI::checkColorTheme()
{
    QColor basecolor = qApp->palette().color(QPalette::Active, QPalette::Base);
    Settings::colors.lighttheme = basecolor.red() + basecolor.green() + basecolor.blue() > 128 * 3;
}

void GlobalUI::showPopup(int which)
{
    int screennum = qApp->screens().indexOf(qApp->screenAt(QCursor::pos()));
    if (which == 0)
    {
        if (PopupDictionary::getInstance() != nullptr)
            disconnect(PopupDictionary::getInstance(), &PopupDictionary::beingClosed, this, &GlobalUI::popupClosing);
        PopupDictionary::popup(screennum, true);
        connect(PopupDictionary::getInstance(), &PopupDictionary::beingClosed, this, &GlobalUI::popupClosing);
    }
    else if (which == 1)
    {
        if (PopupDictionary::getInstance() != nullptr)
            disconnect(PopupDictionary::getInstance(), &PopupDictionary::beingClosed, this, &GlobalUI::popupClosing);
        PopupDictionary::popup(screennum, false);
        connect(PopupDictionary::getInstance(), &PopupDictionary::beingClosed, this, &GlobalUI::popupClosing);
    }
    else if (which == 2)
    {
        if (PopupKanjiSearch::getInstance() != nullptr)
            disconnect(PopupKanjiSearch::getInstance(), &PopupKanjiSearch::beingClosed, this, &GlobalUI::popupClosing);
        PopupKanjiSearch::popup(screennum);
        connect(PopupKanjiSearch::getInstance(), &PopupKanjiSearch::beingClosed, this, &GlobalUI::popupClosing);
    }
}

void GlobalUI::quit()
{
    if (mainforms.empty())
        return;
    //if (!mainforms.at(0)->isVisible())
    //{
    //QCloseEvent ce;
    //qApp->sendEvent(mainforms.at(0), &ce);
    //if (ce.isAccepted())

    saveBeforeQuit();
    qApp->processEvents();
    qApp->quit();

    //return;
    //}
    //mainforms.at(0)->close();
}

void GlobalUI::practiceKana()
{
    setupKanaPractice();
}

void GlobalUI::showDecks()
{
    int dictix = activeDictionaryIndex();
    if (dictix == -1)
        dictix = 0;
    WordDeckForm *f = WordDeckForm::Instance(true, dictix);
    f->setDictionary(dictix);
}


void GlobalUI::kanjiInfoDestroyed()
{
    kanjiinfo = nullptr;
}

void GlobalUI::kanjiInfoLock(bool locked)
{
    if (locked)
    {
        disconnect(kanjiinfo, &KanjiInfoForm::destroyed, this, &GlobalUI::kanjiInfoDestroyed);
        kanjiinfo = nullptr;
    }
    else
    {
        kanjiinfo = (KanjiInfoForm*)sender();
        connect(kanjiinfo, &KanjiInfoForm::destroyed, this, &GlobalUI::kanjiInfoDestroyed);
    }
}

//void GlobalUI::appStateChanged(Qt::ApplicationState state)
//{
//    if (mainForm() == nullptr)
//        return;
//
//     Currently the kanji information windows are only StayOnTop when shown on popup windows.
//     In that case minimizing and restoring the application is not making sense.
//    if (!mainForm()->windowState().testFlag(Qt::WindowMinimized))
//    {
//        // Change the stay on top state of the visible info windows.
//        const QWidgetList wlist = qApp->topLevelWidgets();
//        for (QWidget *w : wlist)
//        {
//            if (w->isVisible() && dynamic_cast<KanjiInfoForm*>(w) != nullptr)
//                ((KanjiInfoForm*)w)->setStayOnTop(state == Qt::ApplicationActive);
//        }
//    }
//}

void GlobalUI::popupClosing()
{
    QWidget *snd = (QWidget*)sender();
    const QWidgetList wlist = qApp->topLevelWidgets();

    QWidget *pdict = PopupDictionary::getInstance();
    QWidget *pkanji = PopupKanjiSearch::getInstance();
    if (snd == pdict)
    {
        disconnect(PopupDictionary::getInstance(), &PopupDictionary::beingClosed, this, &GlobalUI::popupClosing);
        if (pkanji == nullptr || !pkanji->isVisible())
            pkanji = nullptr;
    }
    else if (snd == pkanji)
    {
        disconnect(PopupKanjiSearch::getInstance(), &PopupKanjiSearch::beingClosed, this, &GlobalUI::popupClosing);
        if (pdict == nullptr || !pdict->isVisible())
            pdict = nullptr;
    }

    for (QWidget *w : wlist)
    {
        if (w->parentWidget() == snd)
        {
            bool wasvisible = w->isVisible();
            // Either the dictionary or the kanji search are null, which means the other one is
            // being closed right now. There's no other window that could be the new parent.
            if (pdict == nullptr || pkanji == nullptr)
            {
                // TODO: close word editing window even if state is questionable when popup closes.
                w->close();
            }
            else if (snd == pdict)
            {
                w->setParent(pkanji, w->windowFlags());
                if (wasvisible)
                    w->show();
            }
            else
            {
                w->setParent(pdict, w->windowFlags());
                if (wasvisible)
                    w->show();
            }
        }
    }
}

void GlobalUI::mainStateChanged(bool minimized)
{
    installShortcuts(minimized);
    if (minimized)
        kanjiinfo = nullptr;
    else
    {
        const QWidgetList wlist = qApp->topLevelWidgets();
        for (QWidget *w : wlist)
        {
            if (w->parentWidget() == mainForm() && dynamic_cast<KanjiInfoForm*>(w) != nullptr)
            {
                if (!((KanjiInfoForm*)w)->locked())
                    kanjiinfo = (KanjiInfoForm*)w;
                if (infoblock == 0)
                    w->show();
            }
        }
    }
}

void GlobalUI::appFocusChanged(QWidget * /*lost*/, QWidget * /*received*/)
{
    //if (lost != nullptr)
    //    lost = lost->window();
    //if (received != nullptr)
    //    received = received->window();

    //if ((lost != nullptr && lost->isModal()) == (received != nullptr && received->isModal()))
    //    return;

    static bool hasmodal = false;
    if (qApp->modalWindow() == nullptr)
    {
        if (hasmodal)
        {
            hasmodal = false;
            toggleKanjiInfo(!hasmodal);
        }
    }
    else
    {
        if (!hasmodal)
        {
            hasmodal = true;
            toggleKanjiInfo(!hasmodal);
        }
    }

    //toggleKanjiInfo(received == nullptr || !received->isModal());

    //IsImportantWindowEvent e1;
    //if (lost != nullptr)
    //    qApp->sendEvent(lost->window(), &e1);
    //IsImportantWindowEvent e2;
    //if (received != nullptr)
    //    qApp->sendEvent(received->window(), &e2);

    //if (e1.result() == e2.result())
    //    return;
    //toggleKanjiInfo(!e2.result());
}

void GlobalUI::hiddenWindowDestroyed(QObject *o)
{
    for (int ix = 0, siz = tosigned(appwin.size()); ix != siz; ++ix)
    {
        if (appwin[ix] == o)
        {
            appwin.erase(appwin.begin() + ix);
            break;
        }
    }
}

void GlobalUI::trayActivate(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
        restoreFromTray();
}

void GlobalUI::formDestroyed(QObject *form)
{
    auto it = std::find(mainforms.begin(), mainforms.end(), (ZKanjiForm*)form);
    if (it != mainforms.end())
    {
        mainforms.erase(it);
        return;
    }
}

void GlobalUI::formActivated(ZKanjiForm *form, bool active)
{
    if (!active)
        return;

    auto it = std::find(mainforms.begin(), mainforms.end(), form);
    if (it != mainforms.end() && it != mainforms.begin())
    {
        mainforms.erase(it);
        mainforms.push_back((ZKanjiForm*)form);
        return;
    }
}

void GlobalUI::hideAppWindows()
{
    if (hiddencounter++ != 0)
        return;

    emit appWindowsVisibilityChanged(false);

    qApp->setQuitOnLastWindowClosed(false);
    for (QWidget *w : qApp->topLevelWidgets())
    {
        if (!w->isVisible())
            continue;
        connect(w, &QWidget::destroyed, this, &GlobalUI::hiddenWindowDestroyed);
        appwin.push_back(w);
        w->hide();
    }
}

void GlobalUI::showAppWindows()
{
    if (hiddencounter == 0)
        return;
    if (--hiddencounter != 0)
        return;

    for (QWidget *w : appwin)
    {
        w->show();
        disconnect(w, &QWidget::destroyed, this, &GlobalUI::hiddenWindowDestroyed);
    }
    qApp->setQuitOnLastWindowClosed(true);
    appwin.clear();

    emit appWindowsVisibilityChanged(true);
}

void GlobalUI::showAbout()
{
    QString aboutsection = tr("This program is alpha, use for testing only.\n\nzkanji is a FREE, open-source, cross-platform, feature rich Japanese language study suite and dictionary software.\n\nSee readme.txt and LICENSE files for usage and license details. Released under the terms of the GNU GPLv3.\n\nzkanji uses parts of the LibQxt project. See QxtCOPYING for its license.\n\nSee \"Dictionary/Dictionary information\" menu option for dictionary authors and licenses.");
    QString examplesection = tr("zkanji relies on the Tanaka Corpus for the Japanese example sentences data, which is released under a Creative Commons CC-BY license. See the EXAMPLES file for authors and license location.");
    QString maddr = QString("freem") % "ail.hu";

    QMessageBox::about(activeMainForm(), tr("About zkanji"), "zkanji " % QString::fromLatin1(ZKANJI_PROGRAM_VERSION) % "\n\n"
        % aboutsection % (ZKanji::sentences.creationDate().isValid() ? ("\n\n" % examplesection) : QString()) % "\n\n" % tr("Contact me via e-mail at: ") % "z-one@" % maddr % "\n" % tr("zkanji development blog: ") % "ht" "tp://zkanji." % "wordpress" % ".com/\n" % "zkanji project home: https://github.com/z1dev/zkanji/");
}

void GlobalUI::saveBeforeQuit()
{
    static bool saved = false;
    if (saved || mainforms.empty())
        return;

    saved = true;

    saveSettingsAndStates();
    lastsave = QDateTime::currentDateTimeUtc();
    ZKanji::saveUserData();
}

void GlobalUI::disableAutoSave()
{
    autosavecounter++;
}

void GlobalUI::enableAutoSave()
{
    if (autosavecounter == 0)
        throw "Called without disableAutoSave().";

    --autosavecounter;
}

void GlobalUI::scaledWidgetDestroyed(QObject *o)
{
    scaledwidgets.remove((QWidget*)o);
}

void GlobalUI::installShortcuts(bool install)
{
    if (!install)
    {
        if (fromPopupDictShortcut != nullptr)
            fromPopupDictShortcut->deleteLater();
        if (toPopupDictShortcut != nullptr)
            toPopupDictShortcut->deleteLater();
        if (kanjiPopupShortcut != nullptr)
            kanjiPopupShortcut->deleteLater();
        fromPopupDictShortcut = nullptr;
        toPopupDictShortcut = nullptr;
        kanjiPopupShortcut = nullptr;

        PopupDictionary::hidePopup();
        PopupKanjiSearch::hidePopup();

        return;
    }

    const QString modifiers[3] = { tr("Ctrl+Alt+"), tr("Alt+"), tr("Ctrl+") };
    const QString shift = tr("Shift+");

    if (Settings::shortcuts.fromenable && fromPopupDictShortcut == nullptr)
    {
        fromPopupDictShortcut = new QxtGlobalShortcut(this);
        fromPopupDictShortcut->setShortcut(QKeySequence(modifiers[(int)Settings::shortcuts.frommodifier] + (Settings::shortcuts.fromshift ? shift : QString()) + Settings::shortcuts.fromkey));

        connect(fromPopupDictShortcut, &QxtGlobalShortcut::activated, &popupmap, (void (QSignalMapper::*)())&QSignalMapper::map);
        popupmap.setMapping(fromPopupDictShortcut, 0);

        //connect(fromPopupDictShortcut, &QxtGlobalShortcut::activated, this, &GlobalUI::showPopup);
    }
    if (Settings::shortcuts.toenable && toPopupDictShortcut == nullptr)
    {
        toPopupDictShortcut = new QxtGlobalShortcut(this);
        toPopupDictShortcut->setShortcut(QKeySequence(modifiers[(int)Settings::shortcuts.tomodifier] + (Settings::shortcuts.toshift ? shift : QString()) + Settings::shortcuts.tokey));

        connect(toPopupDictShortcut, &QxtGlobalShortcut::activated, &popupmap, (void (QSignalMapper::*)())&QSignalMapper::map);
        popupmap.setMapping(toPopupDictShortcut, 1);

        //connect(toPopupDictShortcut, &QxtGlobalShortcut::activated, this, &GlobalUI::showPopup);
    }
    if (Settings::shortcuts.kanjienable && kanjiPopupShortcut == nullptr)
    {
        kanjiPopupShortcut = new QxtGlobalShortcut(this);
        kanjiPopupShortcut->setShortcut(QKeySequence(modifiers[(int)Settings::shortcuts.kanjimodifier] + (Settings::shortcuts.kanjishift ? shift : QString()) + Settings::shortcuts.kanjikey));

        connect(kanjiPopupShortcut, &QxtGlobalShortcut::activated, &popupmap, (void (QSignalMapper::*)())&QSignalMapper::map);
        popupmap.setMapping(kanjiPopupShortcut, 2);

        //connect(kanjiPopupShortcut, &QxtGlobalShortcut::activated, this, &GlobalUI::showPopup);
    }
}

void GlobalUI::_scaleSpacerItem(QSpacerItem *s)
{
    QSizePolicy sp = s->sizePolicy();
    QSize sh = s->sizeHint();
    s->changeSize(Settings::scaled(sh.width()), Settings::scaled(sh.height()), sp.horizontalPolicy(), sp.verticalPolicy());
}

void GlobalUI::_scaleLayout(QLayout *l)
{
    if (l == nullptr)
        return;

    int ll, t, r, b;
    l->getContentsMargins(&ll, &t, &r, &b);
    l->setContentsMargins(Settings::scaled(ll), Settings::scaled(t), Settings::scaled(r), Settings::scaled(b));

    if (dynamic_cast<QMainWindow*>(l->parent()) == nullptr)
    {
        for (int ix = 0, siz = l->count(); ix != siz; ++ix)
        {
            QLayoutItem *la = l->itemAt(ix);
            if (la->isEmpty())
                continue;
            QSpacerItem *si = la->spacerItem();
            if (si != nullptr)
                _scaleSpacerItem(si);
            QLayout *lal = la->layout();
            if (lal != nullptr)
                _scaleLayout(lal);
        }
    }

    // Spacing can be inherited so we set it only after the rest is done.

    if (dynamic_cast<QBoxLayout*>(l) != nullptr)
        l->setSpacing(Settings::scaled(l->spacing()));
    else if (dynamic_cast<QFormLayout*>(l) != nullptr)
    {
        ((QFormLayout*)l)->setHorizontalSpacing(Settings::scaled(((QFormLayout*)l)->horizontalSpacing()));
        ((QFormLayout*)l)->setVerticalSpacing(Settings::scaled(((QFormLayout*)l)->verticalSpacing()));
    }
    else if (dynamic_cast<QGridLayout*>(l) != nullptr)
    {
        ((QGridLayout*)l)->setHorizontalSpacing(Settings::scaled(((QGridLayout*)l)->horizontalSpacing()));
        ((QGridLayout*)l)->setVerticalSpacing(Settings::scaled(((QGridLayout*)l)->verticalSpacing()));
    }
}

void GlobalUI::_scaleWidget(QWidget *w)
{
    if (scaledwidgets.contains(w))
        return;
    _registerWidgetScale(w);

    int minw = w->minimumWidth();
    int maxw = w->maximumWidth();
    if (minw != 0)
        w->setMinimumWidth(Settings::scaled(minw));
    if (maxw != QWIDGETSIZE_MAX && maxw < Settings::scaled(maxw))
        w->setMaximumWidth(Settings::scaled(maxw));
    int minh = w->minimumHeight();
    int maxh = w->maximumHeight();
    if (minh != 0)
        w->setMinimumHeight(Settings::scaled(minh));
    if (maxh != QWIDGETSIZE_MAX && maxh < Settings::scaled(maxh))
        w->setMaximumHeight(Settings::scaled(maxh));

    if (w->font() != qApp->font())
    {
        QFont f = w->font();
        f.setPointSizeF(Settings::scaled(f.pointSizeF()));
        w->setFont(f);
    }

    //int l, t, r, b;
    //w->getContentsMargins(&l, &t, &r, &b);
    //w->setContentsMargins(Settings::scaled(l), Settings::scaled(t), Settings::scaled(r), Settings::scaled(b));

    if (dynamic_cast<QSplitter*>(w) != nullptr)
    {
        QSplitter *spl = (QSplitter*)w;
        spl->setHandleWidth(Settings::scaled(spl->handleWidth()));
    }

    QAbstractButton *btn = dynamic_cast<QAbstractButton*>(w);
    if (btn != nullptr)
    {
        QSize siz = btn->iconSize();
        btn->setIconSize(QSize(Settings::scaled(siz.width()), Settings::scaled(siz.height())));
    }

    // Some attributes can only be set recursively because they use what was inherited by
    // the parent, and would increase spacing/padding etc. too much in nested widgets. Use
    // direct-children-only to avoid this.

    QList<QWidget*> wlist = w->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (int ix = 0, siz = wlist.size(); ix != siz; ++ix)
        _scaleWidget(wlist.at(ix));

    _scaleLayout(w->layout());
}

void GlobalUI::_registerWidgetScale(QWidget *w)
{
    scaledwidgets.insert(w);
    connect(w, &QWidget::destroyed, this, &GlobalUI::scaledWidgetDestroyed);
}

void GlobalUI::closeAll()
{
    while (!mainforms.empty())
    {
        delete mainforms.back();
    }
}

//void GlobalUI::wordFilterErased(int index)
//{
//    //class FilterEditorForm;
//    std::map<int, FilterEditorForm*> tmp = filterforms;
//    filterforms.clear();
//    for (auto p : tmp)
//    {
//        if (p.first == index)
//            continue;
//        else if (p.first < index)
//            filterforms[p.first] = p.second;
//        else
//            filterforms[p.first - 1] = p.second;
//    }
//}

//-------------------------------------------------------------
