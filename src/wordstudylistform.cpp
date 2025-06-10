/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMessageBox>
#include <QPushButton>
#include <QSignalMapper>
#include <QGraphicsLayout>
#include <QMenu>
#include <QtEvents>
#include <QToolTip>
#include <map>

#include "wordstudylistform.h"
#include "ui_wordstudylistform.h"
#include "zstudylistmodel.h"
#include "dictionarywidget.h"
#include "globalui.h"
#include "worddeck.h"
#include "wordtodeckform.h"
#include "zlistview.h"
#include "zdictionarylistview.h"
#include "wordstudyform.h"
#include "words.h"
#include "zui.h"
#include "grouppickerform.h"
#include "groups.h"
#include "formstates.h"
#include "dialogs.h"
#include "colorsettings.h"
#include "generalsettings.h"
#include "ztooltip.h"
#include "zstrings.h"

#include "checked_cast.h"


//-------------------------------------------------------------


static const int TickSpacing = 70;


//-------------------------------------------------------------

enum class StatRoles {
    // Role for maximum value to be displayed
    MaxRole = Qt::UserRole + 1,
    // Number of statistic values represented in a single bar.
    StatCountRole,
    // Role for statistic number 1. To get stat 2 etc. add a positive number to this value.
    StatRole,
};


WordStudyTestsModel::WordStudyTestsModel(WordDeck *deck, QObject *parent) : base(parent), deck(deck), maxval(0)
{
    StudyDeck *study = deck->getStudyDeck();
    stats.reserve(study->dayStatSize() * 1.2);
    for (int ix = 0, siz = study->dayStatSize(); ix != siz; ++ix)
    {
        if (ix != 0 && study->dayStat(ix - 1).day.daysTo(study->dayStat(ix).day) > 1)
            stats.push_back(-1);
        stats.push_back(ix);
        maxval = std::max(maxval, study->dayStat(ix).testcount);
    }
}

WordStudyTestsModel::~WordStudyTestsModel()
{

}

int WordStudyTestsModel::count() const
{
    return tosigned(stats.size());
}

int WordStudyTestsModel::barWidth(ZStatView *view, int col) const
{
    QFontMetrics fm = view->fontMetrics();
    if (stats[col] == -1)
        return fm.horizontalAdvance(QStringLiteral("...")) + 16;
    return fm.horizontalAdvance(QStringLiteral("9999:99:99")) + 16;
}

int WordStudyTestsModel::maxValue() const
{
    return maxval;
}

QString WordStudyTestsModel::axisLabel(Qt::Orientation ori) const
{
    if (ori == Qt::Vertical)
        return tr("Tested items");
    return tr("Date of test");
}

QString WordStudyTestsModel::barLabel(int ix) const
{
    if (stats[ix] == -1)
        return QStringLiteral("...");

    StudyDeck *study = deck->getStudyDeck();
    return DateTimeFunctions::formatDay(study->dayStat(stats[ix]).day);
}

int WordStudyTestsModel::valueCount() const
{
    return 3;
}

int WordStudyTestsModel::value(int col, int valpos) const
{
    if (stats[col] == -1)
        return 0;

    StudyDeck *study = deck->getStudyDeck();
    const DeckDayStat &day = study->dayStat(stats[col]);
    switch (valpos)
    {
    case 0:
        return day.testlearned;
    case 1:
        return day.testcount - day.testlearned - day.testwrong;
    case 2:
        return day.testwrong;
    default:
        return 0;
    }
}

QString WordStudyTestsModel::tooltip(int col) const
{
    if (col < 0 || col >= tosigned(stats.size()))
        return QString();

    if (stats[col] == -1)
        return tr("Not studied");

    StudyDeck *study = deck->getStudyDeck();
    const DeckDayStat &day = study->dayStat(stats[col]);
    return tr("Tested: %1\nLearned: %2\nWrong: %3\n%4").arg(day.testcount).arg(day.testlearned).arg(day.testwrong).arg(DateTimeFunctions::formatDay(day.day));
}


//-------------------------------------------------------------


WordStudyLevelsModel::WordStudyLevelsModel(WordDeck *deck, QObject *parent) : base(parent), deck(deck), maxval(0)
{
    list.resize(12);
    for (int ix = 0, siz = deck->studySize(); ix != siz; ++ix)
    {
        int lv = deck->studyLevel(ix);
        if (tosigned(list.size()) <= lv)
            list.resize(lv + 1);
        ++list[lv];
    }

    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        maxval = std::max(maxval, list[ix]);
}

WordStudyLevelsModel::~WordStudyLevelsModel()
{

}

int WordStudyLevelsModel::count() const
{
    return tosigned(list.size());
}

int WordStudyLevelsModel::maxValue() const
{
    return maxval;
}

QString WordStudyLevelsModel::axisLabel(Qt::Orientation ori) const
{
    if (ori == Qt::Horizontal)
        return tr("Level");
    else
        return tr("Item count");
}

QString WordStudyLevelsModel::barLabel(int ix) const
{
    return QString::number(ix + 1);
}

int WordStudyLevelsModel::valueCount() const
{
    return 1;
}

int WordStudyLevelsModel::value(int col, int /*valpos*/) const
{
    return list[col];
}


//-------------------------------------------------------------


WordStudyAreaModel::WordStudyAreaModel(WordDeck *deck, DeckStatAreaType type, DeckStatIntervals interval, QObject *parent) : base(parent), deck(deck), type(type), interval(interval), maxval(-1)
{
    update();
}

WordStudyAreaModel::~WordStudyAreaModel()
{
    ;
}

void WordStudyAreaModel::setInterval(DeckStatIntervals newinterval)
{
    if (interval == newinterval)
        return;
    interval = newinterval;

    //list.clear();
    maxval = -1;
    //update();

    emit modelChanged();
}

int WordStudyAreaModel::count() const
{
    return tosigned(list.size());
}

int WordStudyAreaModel::maxValue() const
{
    if (maxval == -1)
    {
        // Calculate the max value here.
        int first = std::lower_bound(list.begin(), list.end(), firstDate(), [](const std::pair<qint64, std::tuple<int, int, int>> &p, qint64 d) {
            return p.first < d;
        }) - list.begin();
        int last = std::lower_bound(list.begin(), list.end(), lastDate(), [](const std::pair<qint64, std::tuple<int, int, int>> &p, qint64 d) {
            return p.first < d;
        }) - list.begin();
        for (int ix = first, siz = std::min<int>(tosigned(list.size()), last + 1); ix != siz; ++ix)
        {
            const auto &val = list[ix].second;
            maxval = std::max<int>(std::get<0>(val) + std::get<1>(val) + std::get<2>(val), maxval);
        }
    }

    return maxval;
}

int WordStudyAreaModel::valueCount() const
{
    return type == DeckStatAreaType::Items ? 3 : 1;
}

int WordStudyAreaModel::value(int col, int valpos) const
{
    switch (valpos)
    {
    case 0:
        return std::get<0>(list[col].second);
    case 1:
        return std::get<1>(list[col].second);
    case 2:
        return std::get<2>(list[col].second);
    default:
        return 0;
    }
}

QString WordStudyAreaModel::axisLabel(Qt::Orientation ori) const
{
    if (ori == Qt::Horizontal)
        return tr("Date");
    else
        return tr("Item count");
}

qint64 WordStudyAreaModel::firstDate() const
{
    if (list.empty())
        return QDateTime::currentMSecsSinceEpoch();

    if (interval == DeckStatIntervals::All || type == DeckStatAreaType::Forecast)
        return list.front().first;
    else
    {
        QDateTime first = QDateTime::fromMSecsSinceEpoch(list.back().first);
        if (interval == DeckStatIntervals::Year)
            first = first.addYears(-1);
        else if (interval == DeckStatIntervals::HalfYear)
            first = first.addDays(-187);
        else
            first = first.addMonths(-1);

        return first.toMSecsSinceEpoch();
    }
}

qint64 WordStudyAreaModel::lastDate() const
{
    if (list.empty())
        return QDateTime::currentDateTime().addDays(7).toMSecsSinceEpoch();

    if (interval == DeckStatIntervals::All || type == DeckStatAreaType::Items)
        return list.back().first;
    else
    {
        QDateTime last = QDateTime::fromMSecsSinceEpoch(list.front().first);
        if (interval == DeckStatIntervals::Year)
            last = last.addYears(1);
        else if (interval == DeckStatIntervals::HalfYear)
            last = last.addDays(187);
        else
            last = last.addMonths(1);

        return last.toMSecsSinceEpoch();
    }
}

qint64 WordStudyAreaModel::valueDate(int col) const
{
    return list[col].first;
}

QString WordStudyAreaModel::tooltip(int col) const
{
    if (col < 0 || col >= tosigned(list.size()))
        return QString();

    QDateTime date = QDateTime::fromMSecsSinceEpoch(list[col].first);
    int itemcount = std::get<0>(list[col].second);
    int learnedcount = std::get<1>(list[col].second);
    int testcount = std::get<2>(list[col].second);
    if (type == DeckStatAreaType::Items)
        return tr("Items: %1\nLearned: %2\nTested: %3\n%4").arg(itemcount + learnedcount + testcount).arg(learnedcount).arg(testcount).arg(DateTimeFunctions::formatDay(date.date()));
    else if (type == DeckStatAreaType::Forecast)
        return tr("Items: %1\n%2").arg(itemcount).arg(DateTimeFunctions::formatDay(date.date()));

    return QString();
}

void WordStudyAreaModel::update()
{
    const StudyDeck *study = deck->getStudyDeck();
    qreal hi = 0;
    QDateTime last;
    int lastcount = 0;
    int lastlearned = 0;
    if (type == DeckStatAreaType::Items)
    {
        for (int ix = 0, siz = study->dayStatSize(); ix != siz; ++ix)
        {
            const DeckDayStat &stat = study->dayStat(ix);

            // When no data between stats, item counts stay the same, but test count should be
            // 0. Add two zeroes in the middle.
            int lastdays = (last.isValid() ? last.date().daysTo(stat.day) : 0);

            //if (lastdays > 1)
            //{
            //    last = last.addDays(1);
            //    qint64 lasttimesince = last.toMSecsSinceEpoch();
            //    std::pair<qint64, std::tuple<int, int, int>> data;
            //    data.first = lasttimesince;
            //    data.second = std::make_tuple(lastcount - lastlearned, lastlearned, 0);
            //    list.push_back(data);
            //
            //    if (lastdays > 2)
            //    {
            //        last = QDateTime(stat.day.addDays(-1), QTime());
            //        lasttimesince = last.toMSecsSinceEpoch();
            //        data.first = lasttimesince;
            //        list.push_back(data);
            //    }
            //}

            while (lastdays > 1)
            {
                last = last.addDays(1);
                qint64 lasttimesince = last.toMSecsSinceEpoch();
                std::pair<qint64, std::tuple<int, int, int>> data;
                data.first = lasttimesince;
                data.second = std::make_tuple(lastcount - lastlearned, lastlearned, 0);
                list.push_back(data);
                --lastdays;
            }

            last = QDateTime(stat.day, QTime());
            qint64 timesince = last.toMSecsSinceEpoch();

            hi = std::max(hi, (qreal)stat.itemcount);
            list.push_back(std::make_pair(timesince, std::make_tuple(stat.itemcount - std::max(0, stat.itemlearned - stat.testcount) - stat.testcount, std::max(0, stat.itemlearned - stat.testcount), stat.testcount)));
            lastcount = stat.itemcount;
            lastlearned = stat.itemlearned;
        }

        QDate now = ltDay(QDateTime::currentDateTime());
        int nowdays = (last.isValid() ? last.date().daysTo(now) : 0);
        //if (nowdays > 0)
        //{
        //    last = last.addDays(1);
        //    qint64 lasttimesince = last.toMSecsSinceEpoch();
        //    auto data = list.back();
        //    data.first = lasttimesince;
        //    std::get<0>(data.second) += std::get<2>(data.second);
        //    std::get<2>(data.second) = 0;
        //    list.push_back(data);

        //    if (nowdays > 1)
        //    {
        //        last = QDateTime(now, QTime());
        //        lasttimesince = last.toMSecsSinceEpoch();
        //        data.first = lasttimesince;
        //        list.push_back(data);
        //    }
        //}
        while (nowdays > 0)
        {
            last = last.addDays(1);
            qint64 lasttimesince = last.toMSecsSinceEpoch();
            auto data = list.back();
            data.first = lasttimesince;
            std::get<0>(data.second) += std::get<2>(data.second);
            std::get<2>(data.second) = 0;
            list.push_back(data);
            --nowdays;
        }

        return;
    }
    else if (type == DeckStatAreaType::Forecast)
    {
        //const StudyDeck *study = deck->getStudyDeck();

        std::vector<int> items;
        deck->dueItems(items);

        std::vector<int> days;
        days.resize(365);

        QDateTime now = QDateTime::currentDateTimeUtc();
        QDate nowday = ltDay(now);

        // [which date to test next, next spacing, multiplier]
        std::vector<std::tuple<QDateTime, int, double>> data;
        for (int ix = 0, siz = tosigned(items.size()); ix != siz; ++ix)
        {
            const LockedWordDeckItem *i = deck->studiedItems(items[ix]);
            quint32 ispace = study->cardSpacing(i->cardid);
            QDateTime idate = study->cardItemDate(i->cardid).addSecs(ispace);
            QDate iday = ltDay(idate);
            if (iday.daysTo(nowday) > 0)
            {
                idate = now;
                iday = nowday;
            }
            int pos = nowday.daysTo(iday);
            if (pos >= tosigned(days.size()))
                continue;
            data.push_back(std::make_tuple(idate, ispace * study->cardMultiplier(i->cardid), study->cardMultiplier(i->cardid)));
            ++days[pos];
        }

        while (!data.empty())
        {
            std::vector<std::tuple<QDateTime, int, double>> tmp;
            std::swap(tmp, data);
            for (int ix = 0, siz = tosigned(tmp.size()); ix != siz; ++ix)
            {
                quint32 ispace = std::get<1>(tmp[ix]);
                QDateTime idate = std::get<0>(tmp[ix]).addSecs(ispace);
                QDate iday = ltDay(idate);
                if (iday.daysTo(nowday) >= 0)
                {
                    idate = now.addDays(1);
                    iday = nowday.addDays(1);
                }
                int pos = nowday.daysTo(iday);
                if (pos >= tosigned(days.size()))
                    continue;
                double multi = std::get<2>(tmp[ix]);
                data.push_back(std::make_tuple(idate, ispace * multi, multi));
                ++days[pos];
            }
        }

        qint64 timesince = QDateTime(now.date(), QTime()).toMSecsSinceEpoch();
        for (int val : days)
        {
            list.push_back(std::make_pair(timesince, std::make_tuple(val, 0, 0)));
            timesince += 1000 * 60 * 60 * 24;
        }
    }

    maxval = -1;
}


//-------------------------------------------------------------


// TODO: column sizes not saved in list form.


// Single instance of WordDeckForm 
std::map<WordDeck*, WordStudyListForm*> WordStudyListForm::insts;

WordStudyListForm* WordStudyListForm::Instance(WordDeck *deck, DeckStudyPages page, bool createshow)
{
    auto it = insts.find(deck);

    WordStudyListForm *inst = nullptr;
    if (it != insts.end())
        inst = it->second;

    if (inst == nullptr && createshow)
    {
        inst = insts[deck] = new WordStudyListForm(deck, page, gUI->activeMainForm());
        //i->connect(worddeckform, &QMainWindow::destroyed, i, &GlobalUI::formDestroyed);

        inst->show();
    }
    else if (page != DeckStudyPages::None && inst != nullptr)
        inst->showPage(page);

    if (createshow)
    {
        inst->raise();
        inst->activateWindow();
    }


    return inst;
}

WordStudyListForm::WordStudyListForm(WordDeck *deck, DeckStudyPages page, QWidget *parent) : base(parent), ui(new Ui::WordStudyListForm),
        dict(deck->dictionary()), deck(deck), itemsinited(false), statsinited(false), statpage((DeckStatPages)-1), itemsint(DeckStatIntervals::All),
        forecastint(DeckStatIntervals::Month), ignoreop(false)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    ui->dictWidget->setSaveColumnData(false);

    startButton = ui->buttonBox->addButton(QString(), QDialogButtonBox::AcceptRole);
    closeButton = ui->buttonBox->button(QDialogButtonBox::Close);

    connect(startButton, &QPushButton::clicked, this, &WordStudyListForm::startTest);
    connect(closeButton, &QPushButton::clicked, this, &WordStudyListForm::close);

    translateTexts();

    queuesort = { 0, Qt::AscendingOrder };
    studiedsort = { 0, Qt::AscendingOrder };
    testedsort = { 0, Qt::AscendingOrder };

    ui->dictWidget->addFrontWidget(ui->buttonsWidget);
    ui->dictWidget->setExamplesVisible(false);
    ui->dictWidget->setMultilineVisible(false);
    ui->dictWidget->setAcceptWordDrops(true);
    ui->dictWidget->setSortIndicatorVisible(true);
    ui->dictWidget->setWordFiltersVisible(false);
    ui->dictWidget->setSelectionType(ListSelectionType::Extended);
    ui->dictWidget->setInflButtonVisible(false);
    ui->dictWidget->setStudyDefinitionUsed(true);
    ui->dictWidget->setDictionary(dict);

    StudyListModel::defaultColumnWidths(DeckItemViewModes::Queued, queuesizes);
    StudyListModel::defaultColumnWidths(DeckItemViewModes::Studied, studiedsizes);
    StudyListModel::defaultColumnWidths(DeckItemViewModes::Tested, testedsizes);

    queuecols.assign(queuesizes.size() - 2, 1);
    studiedcols.assign(studiedsizes.size() - 2, 1);
    testedcols.assign(testedsizes.size() - 2, 1);

    ui->dictWidget->view()->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->dictWidget->view()->horizontalHeader(), &QWidget::customContextMenuRequested, this, &WordStudyListForm::showColumnContextMenu);
    connect(ui->dictWidget, &DictionaryWidget::customizeContextMenu, this, &WordStudyListForm::showContextMenu);

    connect(ui->queuedButton, &QToolButton::toggled, this, &WordStudyListForm::modeButtonClicked);
    connect(ui->studiedButton, &QToolButton::toggled, this, &WordStudyListForm::modeButtonClicked);
    connect(ui->testedButton, &QToolButton::toggled, this, &WordStudyListForm::modeButtonClicked);

    connect(ui->wButton, &QToolButton::toggled, this, &WordStudyListForm::partButtonClicked);
    connect(ui->kButton, &QToolButton::toggled, this, &WordStudyListForm::partButtonClicked);
    connect(ui->dButton, &QToolButton::toggled, this, &WordStudyListForm::partButtonClicked);

    connect(deck->owner(), &WordDeckList::deckToBeRemoved, this, &WordStudyListForm::closeCancel);

    restoreFormState(FormStates::wordstudylist);

    ui->tabWidget->setCurrentIndex(-1);
    ui->statFrame->setBackgroundRole(QPalette::Base);
    ui->statFrame->setAutoFillBackground(true);

    Settings::updatePalette(ui->statFrame);
    Settings::updatePalette(ui->itemsIntervalWidget);
    Settings::updatePalette(ui->forecastIntervalWidget);

    if (page == DeckStudyPages::None)
        page = DeckStudyPages::Items;
    showPage(page, true);
}

WordStudyListForm::~WordStudyListForm()
{
    delete ui;
    insts.erase(insts.find(deck));
}

void WordStudyListForm::saveState(WordStudyListFormData &dat) const
{
    dat.siz = isMaximized() ? normalGeometry().size() : rect().size();

    if (itemsinited)
    {
        dat.items.showkanji = ui->wButton->isChecked();
        dat.items.showkana = ui->kButton->isChecked();
        dat.items.showdef = ui->dButton->isChecked();

        dat.items.mode = ui->queuedButton->isChecked() ? DeckItemViewModes::Queued : ui->studiedButton->isChecked() ? DeckItemViewModes::Studied : DeckItemViewModes::Tested;

        ui->dictWidget->saveState(dat.items.dict);

        dat.items.queuesort.column = queuesort.column;
        dat.items.queuesort.order = queuesort.order;
        dat.items.studysort.column = studiedsort.column;
        dat.items.studysort.order = studiedsort.order;
        dat.items.testedsort.column = testedsort.column;
        dat.items.testedsort.order = testedsort.order;

        dat.items.queuesizes = queuesizes;
        dat.items.studysizes = studiedsizes;
        dat.items.testedsizes = testedsizes;
        dat.items.queuecols = queuecols;
        dat.items.studycols = studiedcols;
        dat.items.testedcols = testedcols;
    }
    if (statsinited)
    {
        dat.stats.page = ui->itemsButton->isChecked() ? DeckStatPages::Items : ui->forecastButton->isChecked() ? DeckStatPages::Forecast : ui->levelsButton->isChecked() ? DeckStatPages::Levels : DeckStatPages::Tests;
        dat.stats.itemsinterval = itemsint;
        dat.stats.forecastinterval = forecastint;
    }
}

void WordStudyListForm::restoreFormState(const WordStudyListFormData &dat)
{
    if (!FormStates::emptyState(dat))
        resize(dat.siz);
}

void WordStudyListForm::restoreItemsState(const WordStudyListFormDataItems &dat)
{
    FlagGuard<bool> ignoreguard(&ignoreop, true, false);

    ui->wButton->setChecked(dat.showkanji);
    ui->kButton->setChecked(dat.showkana);
    ui->dButton->setChecked(dat.showdef);

    if (dat.mode == DeckItemViewModes::Queued)
        ui->queuedButton->setChecked(true);
    else if (dat.mode == DeckItemViewModes::Studied)
        ui->studiedButton->setChecked(true);
    else if (dat.mode == DeckItemViewModes::Tested)
        ui->testedButton->setChecked(true);

    ui->dictWidget->restoreState(dat.dict);

    queuesort.column = dat.queuesort.column;
    queuesort.order = dat.queuesort.order;
    studiedsort.column = dat.studysort.column;
    studiedsort.order = dat.studysort.order;
    testedsort.column = dat.testedsort.column;
    testedsort.order = dat.testedsort.order;

    queuesizes = dat.queuesizes;
    studiedsizes = dat.studysizes;
    testedsizes = dat.testedsizes;
    queuecols = dat.queuecols;
    studiedcols = dat.studycols;
    testedcols = dat.testedcols;

    ignoreguard.release(true);

    if (model->viewMode() == dat.mode)
        model->reset();
    else
        model->setViewMode(dat.mode);

    model->setShownParts(ui->wButton->isChecked(), ui->kButton->isChecked(), ui->dButton->isChecked());

    auto &s = dat.mode == DeckItemViewModes::Queued ? queuesort : dat.mode == DeckItemViewModes::Studied ? studiedsort : testedsort;
    if (s.column == -1)
        s.column = 0;

    // Without the forced update (happening in the sort) the widget won't have columns to
    // restore below.
    ui->dictWidget->forceUpdate();

    ui->dictWidget->setSortIndicator(s.column, s.order);
    ui->dictWidget->sortByIndicator();

    restoreColumns();
}

void WordStudyListForm::restoreStatsState(const WordStudyListFormDataStats &dat)
{
    itemsint = dat.itemsinterval;
    forecastint = dat.forecastinterval;

    FlagGuard<bool> ignoreguard(&ignoreop, true, false);
    ui->itemsButton->setChecked(dat.page == DeckStatPages::Items);
    ui->forecastButton->setChecked(dat.page == DeckStatPages::Forecast);
    ui->levelsButton->setChecked(dat.page == DeckStatPages::Levels);
    ui->testsButton->setChecked(dat.page == DeckStatPages::Tests);
    ignoreguard.release(true);
    showStat(dat.page);
}

void WordStudyListForm::showPage(DeckStudyPages newpage, bool forceinit)
{
    switch (newpage)
    {
    case DeckStudyPages::Items:
        if (ui->tabWidget->currentIndex() != 0)
            ui->tabWidget->setCurrentIndex(0);
        else if (forceinit)
            on_tabWidget_currentChanged(0);
        break;
    case DeckStudyPages::Stats:
        if (ui->tabWidget->currentIndex() != 1)
            ui->tabWidget->setCurrentIndex(1);
        else if (forceinit)
            on_tabWidget_currentChanged(1);
        break;
    default:
        return;
    }
}

void WordStudyListForm::showQueue()
{
    if (!ui->queuedButton->isChecked())
        ui->queuedButton->toggle();
}

void WordStudyListForm::startTest()
{
    // WARNING: if later this close() is removed, the model of the dictionary table must be
    // destroyed and recreated when the window is shown again. The test changes the deck items
    // and that doesn't show up in the model.
    close();
    WordStudyForm::studyDeck(deck);
}

void WordStudyListForm::addQuestions(const std::vector<int> &wordlist)
{
    addWordsToDeck(deck, wordlist, this);
}

void WordStudyListForm::removeItems(const std::vector<int> &items, bool queued)
{
    QString msg;
    if (queued)
        msg = tr("Do you want to remove the selected items?\n\nThe items will be removed from the queue, but their custom definitions shared between tests won't change.");
    else
        msg = tr("Do you want to remove the selected items?\n\nThe study data for the selected items will be reset and the items will be removed from this list. The custom definitions shared between tests won't change.\n\nThis can't be undone.");
    if (QMessageBox::question(this, "zkanji", msg, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    if (queued)
        deck->removeQueuedItems(items);
    else
        deck->removeStudiedItems(items);
}

void WordStudyListForm::requeueItems(const std::vector<int> &items)
{
    if (QMessageBox::question(this, "zkanji", tr("Do you want to move the selected items back to the queue?\n\nThe study data of the items will be reset, the items will be removed from this list and moved to the end of the queue. The custom definitions shared between tests won't change.\n\nThis can't be undone."), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    deck->requeueStudiedItems(items);
}

void WordStudyListForm::changePriority(const std::vector<int> &items, uchar val)
{
    deck->setQueuedPriority(items, val);
}

void WordStudyListForm::changeMainHint(const std::vector<int> &items, bool queued, WordParts val)
{
    deck->setItemHints(items, queued, val);
}

void WordStudyListForm::increaseLevel(int deckitem)
{
    deck->increaseSpacingLevel(deckitem);
}

void WordStudyListForm::decreaseLevel(int deckitem)
{
    deck->decreaseSpacingLevel(deckitem);
}

void WordStudyListForm::resetItems(const std::vector<int> &items)
{
    if (QMessageBox::question(this, "zkanji", tr("All study data, including past statistics, item level and difficulty will be reset for the selected items. The items will be shown like they were new the next time the test starts. Consider moving the items back to the queue instead.\n\nDo you want to reset the study data?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
        deck->resetCardStudyData(items);
}

bool WordStudyListForm::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        translateTexts();
    }

    return base::event(e);
}

void WordStudyListForm::closeEvent(QCloseEvent *e)
{
    if (itemsinited)
        saveColumns();

    saveState(FormStates::wordstudylist);
    base::closeEvent(e);
}

void WordStudyListForm::keyPressEvent(QKeyEvent *e)
{
    if (itemsinited && ui->tabWidget->currentIndex() == 0)
    {
        bool queue = model->viewMode() == DeckItemViewModes::Queued;
        if (queue && e->modifiers().testFlag(Qt::ControlModifier) && e->key() >= Qt::Key_1 && e->key() <= Qt::Key_9)
        {
            std::vector<int> rowlist;
            ui->dictWidget->selectedRows(rowlist);
            if (rowlist.empty())
            {
                base::keyPressEvent(e);
                return;
            }
            for (int &ix : rowlist)
                ix = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::DeckIndex).toInt();
            deck->setQueuedPriority(rowlist, e->key() - Qt::Key_1 + 1);
        }
    }

    base::keyPressEvent(e);
}

void WordStudyListForm::on_tabWidget_currentChanged(int index)
{
    if (index == -1 || (index == 0 && itemsinited) || (index == 1 && statsinited))
        return;

    if (!itemsinited && !statsinited) {
        restoreFormState(FormStates::wordstudylist);
        connect(gUI, &GlobalUI::dictionaryRemoved, this, &WordStudyListForm::dictionaryRemoved);
        connect(gUI, &GlobalUI::dictionaryReplaced, this, &WordStudyListForm::dictionaryReplaced);
    }

    if (index == 0)
    {
        itemsinited = true;

        model = new StudyListModel(deck, this);
        ui->dictWidget->setModel(model);
        ui->dictWidget->setSortFunction([this](DictionaryItemModel * /*d*/, int c, int a, int b) { return model->sortOrder(c, a, b); });

        connect(ui->dictWidget, &DictionaryWidget::rowSelectionChanged, this, &WordStudyListForm::rowSelectionChanged);
        connect(ui->dictWidget, &DictionaryWidget::sortIndicatorChanged, this, &WordStudyListForm::headerSortChanged);

        auto &s = ui->queuedButton->isChecked() ? queuesort : ui->studiedButton->isChecked() ? studiedsort : testedsort;
        ui->dictWidget->setSortIndicator(s.column, s.order);
        ui->dictWidget->sortByIndicator();

        connect(dict, &Dictionary::dictionaryReset, this, &WordStudyListForm::dictionaryReset);

        restoreItemsState(FormStates::wordstudylist.items);
    }
    else
    {
        statsinited = true;

        ui->dueLabel->setText(QString::number(deck->dueSize()));
        ui->queueLabel->setText(QString::number(deck->queueSize()));

        ui->studiedLabel->setText(QString::number(deck->studySize()));

        // Calculate unique words and kanji count by checking every word data.
        int wordcnt = 0;
        QSet<ushort> kanjis;
        for (int ix = 0, siz = deck->wordDataSize(); ix != siz; ++ix)
        {
            if (!deck->wordDataStudied(ix))
                continue;
            ++wordcnt;
            WordEntry *e = dict->wordEntry(deck->wordData(ix)->index);
            for (int iy = 0, siy = e->kanji.size(); iy != siy; ++iy)
                if (KANJI(e->kanji[iy].unicode()))
                    kanjis.insert(e->kanji[iy].unicode());
        }

        ui->wordsLabel->setText(QString::number(wordcnt));
        ui->kanjiLabel->setText(QString::number(kanjis.size()));

        ui->firstLabel->setText(DateTimeFunctions::formatPastDay(deck->firstDay()));
        ui->lastLabel->setText(DateTimeFunctions::formatPastDay(deck->lastDay()));
        ui->testDaysLabel->setText(QString::number(deck->testDayCount()));
        ui->skippedDaysLabel->setText(QString::number(deck->skippedDayCount()));

        ui->studyTimeLabel->setText(DateTimeFunctions::formatLength((int)((deck->totalTime() + 5) / 10)));
        ui->studyTimeAvgLabel->setText(DateTimeFunctions::formatLength((int)((deck->studyAverage() + 5) / 10)));
        ui->answerTimeAvgLabel->setText(DateTimeFunctions::formatLength((int)((deck->answerAverage() + 5) / 10)));

        //showStat(DeckStatPages::Items);

        restoreStatsState(FormStates::wordstudylist.stats);
    }
}

void WordStudyListForm::headerSortChanged(int column, Qt::SortOrder order)
{
    if (ignoreop)
        return;

    auto &s = model->viewMode() == DeckItemViewModes::Queued ? queuesort : model->viewMode() == DeckItemViewModes::Studied ? studiedsort : testedsort;
    if (order == s.order && column == s.column)
        return;

    if (column == model->columnCount() - 1)
    {
        // The last column shows the definitions of words and cannot be used for sorting. The
        // original column should be restored.
        ui->dictWidget->setSortIndicator(s.column, s.order);
        return;
    }

    s.column = column;
    s.order = order;

    ui->dictWidget->sortByIndicator();
}

void WordStudyListForm::rowSelectionChanged()
{
    std::vector<int> indexes;
    ui->dictWidget->selectedIndexes(indexes);
    ui->defEditor->setWords(dict, indexes);

    ui->addButton->setEnabled(!indexes.empty());
    ui->delButton->setEnabled(!indexes.empty());
    ui->backButton->setEnabled((ui->studiedButton->isChecked() || ui->testedButton->isChecked()) && !indexes.empty());
}

void WordStudyListForm::modeButtonClicked(bool checked)
{
    if (!checked)
        return;

    saveColumns();

    DeckItemViewModes mode = ui->queuedButton->isChecked() ? DeckItemViewModes::Queued : ui->studiedButton->isChecked() ? DeckItemViewModes::Studied : DeckItemViewModes::Tested;

    auto &s = mode == DeckItemViewModes::Queued ? queuesort : mode == DeckItemViewModes::Studied ? studiedsort : testedsort;
    model->setViewMode(mode);

    restoreColumns();

    ui->dictWidget->setSortIndicator(s.column, s.order);
    ui->dictWidget->sortByIndicator();
    ui->dictWidget->resetScrollPosition();
}

void WordStudyListForm::partButtonClicked()
{
    model->setShownParts(ui->wButton->isChecked(), ui->kButton->isChecked(), ui->dButton->isChecked());
}

void WordStudyListForm::on_addButton_clicked()
{
    std::vector<int> wordlist;
    ui->dictWidget->selectedIndexes(wordlist);
    addQuestions(wordlist);
}

void WordStudyListForm::on_delButton_clicked()
{
    std::vector<int> rowlist;
    ui->dictWidget->selectedRows(rowlist);
    for (int &ix : rowlist)
        ix = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::DeckIndex).toInt();

    removeItems(rowlist, ui->queuedButton->isChecked());
}

void WordStudyListForm::on_backButton_clicked()
{
    if (ui->queuedButton->isChecked())
        return;

    std::vector<int> rowlist;
    ui->dictWidget->selectedRows(rowlist);
    for (int &ix : rowlist)
        ix = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::DeckIndex).toInt();

    requeueItems(rowlist);
}

void WordStudyListForm::showColumnContextMenu(const QPoint &p)
{
    std::vector<char> &vec = model->viewMode() == DeckItemViewModes::Queued ? queuecols : model->viewMode() == DeckItemViewModes::Studied ? studiedcols : testedcols;
    QMenu m;

    for (int ix = 0, siz = tosigned(vec.size()); ix != siz; ++ix)
    {
        QAction *a = m.addAction(ui->dictWidget->view()->model()->headerData(ix, Qt::Horizontal, Qt::DisplayRole).toString());
        a->setCheckable(true);
        a->setChecked(vec[ix]);
    }

    QAction *a = m.exec(((QWidget*)sender())->mapToGlobal(p));
    if (a == nullptr)
        return;
    int aix = m.actions().indexOf(a);
    
    std::vector<int> &sizvec = model->viewMode() == DeckItemViewModes::Queued ? queuesizes : model->viewMode() == DeckItemViewModes::Studied ? studiedsizes : testedsizes;
    if (vec[aix])
        sizvec[aix] = ui->dictWidget->view()->columnWidth(aix);

    vec[aix] = !vec[aix];

    ui->dictWidget->view()->setColumnHidden(aix, !vec[aix]);

    if (vec[aix])
        ui->dictWidget->view()->setColumnWidth(aix, sizvec[aix]);
}

void WordStudyListForm::showContextMenu(QMenu *menu, QAction *insertpos, Dictionary * /*dict*/, DictColumnTypes /*coltype*/, QString /*selstr*/, const std::vector<int> &/*windexes*/, const std::vector<ushort> &/*kindexes*/)
{
    if (ui->queuedButton->isChecked())
    {
        QAction *actions[9];

        QMenu *m = new QMenu(tr("Main hint"));
        menu->insertMenu(insertpos, m);

        std::vector<int> selrows;
        std::vector<int> rowlist;
        ui->dictWidget->selectedRows(selrows);

        // Used in checking if all items have the same hint type. In case this is false, it
        // will be set to something else than the valid values. (=anything above 3)
        uchar mainhint = 255;
        // Lists all hint types that should be shown in the context menu.
        uchar shownhints = (uchar)WordPartBits::Default;
        for (int &ix : selrows)
        {
            uchar question = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::ItemQuestion).toInt();
            uchar hint = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::ItemHint).toInt();
            if (mainhint == 255)
                mainhint = hint;
            else if (mainhint != hint) // No match. Set mainhint to invalid.
                mainhint = 254;

            shownhints |= (int)WordPartBits::AllParts - question;

            rowlist.push_back(ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::DeckIndex).toInt());
        }

        QSignalMapper *map = new QSignalMapper(menu);
        connect(menu, &QMenu::aboutToHide, map, &QObject::deleteLater);
        QString strhint[4] = { tr("Default"), tr("Written"), tr("Kana"), tr("Definition") };
        for (int ix = 0; ix != 4; ++ix)
        {
            if (ix != 0 && ((shownhints & (1 << (ix - 1))) == 0))
                continue;

            actions[ix] = m->addAction(strhint[ix]);
            // The Default value in mainhint is 3 but we want to place default at the front.
            // Because of that ix must be converted to match mainhint.
            if ((mainhint == 3 && ix == 0) || (ix != 0 && ix - 1 == mainhint))
            {
                actions[ix]->setCheckable(true);
                actions[ix]->setChecked(true);
            }

            connect(actions[ix], &QAction::triggered, map, (void (QSignalMapper::*)())&QSignalMapper::map);
            map->setMapping(actions[ix], ix == 0 ? 3 : ix - 1);
        }

        connect(map, &QSignalMapper::mappedInt, this, [this, rowlist](int val) {
            changeMainHint(rowlist, true, (WordParts)val);
        });
        m->setEnabled(ui->dictWidget->hasSelection());

        m = new QMenu(tr("Priority"));
        menu->insertMenu(insertpos, m)->setEnabled(ui->dictWidget->hasSelection());
        menu->insertSeparator(insertpos);

        map = new QSignalMapper(menu);
        connect(menu, &QMenu::aboutToHide, map, &QObject::deleteLater);

        connect(map, &QSignalMapper::mappedInt, this, [this, rowlist](int val) {
            changePriority(rowlist, val);
        });

        for (int ix = 0; ix != 9; ++ix)
        {
            actions[ix] = m->addAction(QString("&%1 - %2").arg(9 - ix).arg(Strings::priorities(ix)));
            actions[ix]->setShortcut(QKeySequence(tr("Ctrl+%1").arg(9 - ix)));
            connect(actions[ix], &QAction::triggered, map, (void (QSignalMapper::*)())&QSignalMapper::map);
            map->setMapping(actions[ix], 9 - ix);
        }

        QAction *a = new QAction(tr("Remove from deck..."), this);
        menu->insertAction(insertpos, a);
        connect(a, &QAction::triggered, this, [this, rowlist]() {
            removeItems(rowlist, true);
        });
        a->setEnabled(ui->dictWidget->hasSelection());

        menu->insertSeparator(insertpos);

        return;
    }

    QMenu *m = new QMenu(tr("Main hint"));
    menu->insertMenu(insertpos, m);

    std::vector<int> selrows;
    std::vector<int> rowlist;
    ui->dictWidget->selectedRows(selrows);

    // Used in checking if all items have the same hint type. In case this is false, it
    // will be set to something else than the valid values. (=anything above 3)
    uchar mainhint = 255;
    // Lists all hint types that should be shown in the context menu.
    uchar shownhints = (uchar)WordPartBits::Default;
    for (int &ix : selrows)
    {
        uchar question = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::ItemQuestion).toInt();
        uchar hint = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::ItemHint).toInt();
        if (mainhint == 255)
            mainhint = hint;
        else if (mainhint != hint) // No match. Set mainhint to invalid.
            mainhint = 254;

        shownhints |= (int)WordPartBits::AllParts - question;

        rowlist.push_back(ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::DeckIndex).toInt());
    }

    QAction *actions[4];

    QSignalMapper *map = new QSignalMapper(menu);
    connect(menu, &QMenu::aboutToHide, map, &QObject::deleteLater);
    QString strhint[4] = { tr("Default"), tr("Written"), tr("Kana"), tr("Definition") };
    for (int ix = 0; ix != 4; ++ix)
    {
        if (ix != 0 && ((shownhints & (1 << (ix - 1))) == 0))
            continue;

        actions[ix] = m->addAction(strhint[ix]);
        // The Default value in mainhint is 3 but we want to place default at the front.
        // Because of that ix must be converted to match mainhint.
        if ((mainhint == 3 && ix == 0) || (ix != 0 && ix - 1 == mainhint))
        {
            actions[ix]->setCheckable(true);
            actions[ix]->setChecked(true);
        }

        connect(actions[ix], &QAction::triggered, map, (void (QSignalMapper::*)())&QSignalMapper::map);
        map->setMapping(actions[ix], ix == 0 ? 3 : ix - 1);
    }

    connect(map, &QSignalMapper::mappedInt, this, [this, rowlist](int val) {
        changeMainHint(rowlist, false, (WordParts)val);
    });
    m->setEnabled(ui->dictWidget->hasSelection());

    //menu->insertSeparator(insertpos);

    m = new QMenu(tr("Study options"));
    menu->insertMenu(insertpos, m);

    QAction *a = m->addAction(tr("Increase level"));
    m->setEnabled(rowlist.size() == 1);
    if (rowlist.size() == 1)
    {
        int dix = rowlist.front();
        connect(a, &QAction::triggered, this, [this, dix]() { increaseLevel(dix); });
    }

    a = m->addAction(tr("Decrease level"));
    m->setEnabled(rowlist.size() == 1);
    if (rowlist.size() == 1)
    {
        int dix = rowlist.front();
        connect(a, &QAction::triggered, this, [this, dix]() { decreaseLevel(dix); });
    }

    m->addSeparator();
    a = m->addAction(tr("Reset study data"));
    connect(a, &QAction::triggered, this, [this, rowlist]() { resetItems(rowlist); });
    a->setEnabled(ui->dictWidget->hasSelection());

    m->setEnabled(ui->dictWidget->hasSelection());

    menu->insertSeparator(insertpos);

    a = new QAction(tr("Remove from deck..."), this);
    menu->insertAction(insertpos, a);
    connect(a, &QAction::triggered, this, [this, rowlist]() {
        removeItems(rowlist, false);
    });
    a->setEnabled(ui->dictWidget->hasSelection());

    a = new QAction(tr("Move back to queue..."), this);
    menu->insertAction(insertpos, a);
    connect(a, &QAction::triggered, this, [this, rowlist]() {
        requeueItems(rowlist);
    });
    a->setEnabled(ui->dictWidget->hasSelection());

    menu->insertSeparator(insertpos);
}

void WordStudyListForm::on_items1Radio_toggled(bool checked)
{
    if (checked && statpage == DeckStatPages::Items && dynamic_cast<WordStudyAreaModel*>(ui->statView->model()) != nullptr)
        ((WordStudyAreaModel*)ui->statView->model())->setInterval(DeckStatIntervals::All);
}

void WordStudyListForm::on_items2Radio_toggled(bool checked)
{
    if (checked && statpage == DeckStatPages::Items && dynamic_cast<WordStudyAreaModel*>(ui->statView->model()) != nullptr)
        ((WordStudyAreaModel*)ui->statView->model())->setInterval(DeckStatIntervals::Year);
}

void WordStudyListForm::on_items3Radio_toggled(bool checked)
{
    if (checked && statpage == DeckStatPages::Items && dynamic_cast<WordStudyAreaModel*>(ui->statView->model()) != nullptr)
        ((WordStudyAreaModel*)ui->statView->model())->setInterval(DeckStatIntervals::HalfYear);
}

void WordStudyListForm::on_items4Radio_toggled(bool checked)
{
    if (checked && statpage == DeckStatPages::Items && dynamic_cast<WordStudyAreaModel*>(ui->statView->model()) != nullptr)
        ((WordStudyAreaModel*)ui->statView->model())->setInterval(DeckStatIntervals::Month);
}

void WordStudyListForm::on_fore1Radio_toggled(bool checked)
{
    if (checked && statpage == DeckStatPages::Forecast && dynamic_cast<WordStudyAreaModel*>(ui->statView->model()) != nullptr)
        ((WordStudyAreaModel*)ui->statView->model())->setInterval(DeckStatIntervals::Year);
}

void WordStudyListForm::on_fore2Radio_toggled(bool checked)
{
    if (checked && statpage == DeckStatPages::Forecast && dynamic_cast<WordStudyAreaModel*>(ui->statView->model()) != nullptr)
        ((WordStudyAreaModel*)ui->statView->model())->setInterval(DeckStatIntervals::HalfYear);
}

void WordStudyListForm::on_fore3Radio_toggled(bool checked)
{
    if (checked && statpage == DeckStatPages::Forecast && dynamic_cast<WordStudyAreaModel*>(ui->statView->model()) != nullptr)
        ((WordStudyAreaModel*)ui->statView->model())->setInterval(DeckStatIntervals::Month);
}

void WordStudyListForm::on_itemsButton_clicked()
{
    showStat(DeckStatPages::Items);
}

void WordStudyListForm::on_forecastButton_clicked()
{
    showStat(DeckStatPages::Forecast);
}

void WordStudyListForm::on_levelsButton_clicked()
{
    showStat(DeckStatPages::Levels);
}

void WordStudyListForm::on_testsButton_clicked()
{
    showStat(DeckStatPages::Tests);
}

void WordStudyListForm::dictionaryReset()
{
    if (!itemsinited)
        return;

    auto &s = ui->queuedButton->isChecked() ? queuesort : ui->studiedButton->isChecked() ? studiedsort : testedsort;
    model->reset();

    ui->dictWidget->setSortIndicator(s.column, s.order);
    ui->dictWidget->sortByIndicator();
}

void WordStudyListForm::dictionaryReplaced(Dictionary *olddict, Dictionary* /*newdict*/, int /*index*/)
{
    if (dict == olddict)
        close();
}

void WordStudyListForm::dictionaryRemoved(int /*index*/, int /*orderindex*/, void *oldaddress)
{
    if (dict == oldaddress)
        close();
}

void WordStudyListForm::translateTexts()
{
    startButton->setText(tr("Start the test"));
    ui->buttonBox->button(QDialogButtonBox::Close)->setText(qApp->translate("ButtonBox", "Close"));
}

void WordStudyListForm::saveColumns()
{
    std::vector<int> &oldvec = model->viewMode() == DeckItemViewModes::Queued ? queuesizes : model->viewMode() == DeckItemViewModes::Studied ? studiedsizes : testedsizes;
    oldvec.resize((model->viewMode() == DeckItemViewModes::Queued ? StudyListModel::queueColCount() : model->viewMode() == DeckItemViewModes::Studied ? StudyListModel::studiedColCount() : StudyListModel::testedColCount()) - 1, -1);
    for (int ix = 0, siz = tosigned(oldvec.size()); ix != siz; ++ix)
    {
        bool hid = ui->dictWidget->view()->isColumnHidden(ix);
        ui->dictWidget->view()->setColumnHidden(ix, false);
        oldvec[ix] = ui->dictWidget->view()->columnWidth(ix);
        if (hid)
            ui->dictWidget->view()->setColumnHidden(ix, true);
    }

    std::vector<char> &oldcolvec = model->viewMode() == DeckItemViewModes::Queued ? queuecols : model->viewMode() == DeckItemViewModes::Studied ? studiedcols : testedcols;
    oldcolvec.resize((model->viewMode() == DeckItemViewModes::Queued ? StudyListModel::queueColCount() : model->viewMode() == DeckItemViewModes::Studied ? StudyListModel::studiedColCount() : StudyListModel::testedColCount()) - 3, 1);
    for (int ix = 0, siz = tosigned(oldcolvec.size()); ix != siz; ++ix)
        oldcolvec[ix] = !ui->dictWidget->view()->isColumnHidden(ix);
}

void WordStudyListForm::restoreColumns()
{
    std::vector<char> &colvec = model->viewMode() == DeckItemViewModes::Queued ? queuecols : model->viewMode() == DeckItemViewModes::Studied ? studiedcols : testedcols;
    colvec.resize((model->viewMode() == DeckItemViewModes::Queued ? StudyListModel::queueColCount() : model->viewMode() == DeckItemViewModes::Studied ? StudyListModel::studiedColCount() : StudyListModel::testedColCount()) - 3, 1);
    for (int ix = 0, siz = tosigned(colvec.size()); ix != siz; ++ix)
        ui->dictWidget->view()->setColumnHidden(ix, !colvec[ix]);

    std::vector<int> &vec = model->viewMode() == DeckItemViewModes::Queued ? queuesizes : model->viewMode() == DeckItemViewModes::Studied ? studiedsizes : testedsizes;
    vec.resize((model->viewMode() == DeckItemViewModes::Queued ? StudyListModel::queueColCount() : model->viewMode() == DeckItemViewModes::Studied ? StudyListModel::studiedColCount() : StudyListModel::testedColCount()) - 1, -1);
    for (int ix = 0, siz = tosigned(vec.size()); ix != siz; ++ix)
    {
        bool hid = ui->dictWidget->view()->isColumnHidden(ix);
        ui->dictWidget->view()->setColumnHidden(ix, false);
        if (vec[ix] != -1)
            ui->dictWidget->view()->setColumnWidth(ix, vec[ix]);
        else
            ui->dictWidget->view()->setColumnWidth(ix, model->defaultColumnWidth(model->viewMode(), ix));
        if (hid)
            ui->dictWidget->view()->setColumnHidden(ix, true);
    }
}

void WordStudyListForm::showStat(DeckStatPages page)
{
    if (ignoreop)
        return;

    FlagGuard<bool> ignoreguard(&ignoreop, true, false);

    if (ui->statView->model() != nullptr)
        ui->statView->model()->deleteLater();
    ui->statView->setModel(nullptr);

    //int fmh = fontMetrics().height();

    if (statpage == DeckStatPages::Items)
    {
        if (ui->items1Radio->isChecked())
            itemsint = DeckStatIntervals::All;
        else if (ui->items2Radio->isChecked())
            itemsint = DeckStatIntervals::Year;
        else if (ui->items3Radio->isChecked())
            itemsint = DeckStatIntervals::HalfYear;
        else
            itemsint = DeckStatIntervals::Month;
    }
    else if (statpage == DeckStatPages::Forecast)
    {
        if (ui->fore1Radio->isChecked())
            forecastint = DeckStatIntervals::Year;
        else if (ui->fore2Radio->isChecked())
            forecastint = DeckStatIntervals::HalfYear;
        else
            forecastint = DeckStatIntervals::Month;
    }

    statpage = page;

    switch (page)
    {
    case DeckStatPages::Items:
    {
        if (itemsint == DeckStatIntervals::All)
            ui->items1Radio->setChecked(true);
        else if (itemsint == DeckStatIntervals::Year)
            ui->items2Radio->setChecked(true);
        else if (itemsint == DeckStatIntervals::HalfYear)
            ui->items3Radio->setChecked(true);
        else
            ui->items4Radio->setChecked(true);

        WordStudyAreaModel *m = new WordStudyAreaModel(deck, DeckStatAreaType::Items, itemsint, ui->statView);
        ui->statView->setModel(m);

        break;
    }
    case DeckStatPages::Forecast:
    {
        if (forecastint == DeckStatIntervals::Year)
            ui->fore1Radio->setChecked(true);
        else if (forecastint == DeckStatIntervals::HalfYear)
            ui->fore2Radio->setChecked(true);
        else
            ui->fore3Radio->setChecked(true);

        WordStudyAreaModel *m = new WordStudyAreaModel(deck, DeckStatAreaType::Forecast, forecastint, ui->statView);
        ui->statView->setModel(m);
        break;
    }
    case DeckStatPages::Levels:
    {
        WordStudyLevelsModel *m = new WordStudyLevelsModel(deck, ui->statView);
        ui->statView->setModel(m);
        break;
    }
    case DeckStatPages::Tests:
    {
        WordStudyTestsModel *m = new WordStudyTestsModel(deck, ui->statView);
        ui->statView->setModel(m);
        ui->statView->scrollTo(ui->statView->model()->count());
        break;
    }
    }

    ui->itemsIntervalWidget->setVisible(page == DeckStatPages::Items);
    ui->forecastIntervalWidget->setVisible(page == DeckStatPages::Forecast);
}


//-------------------------------------------------------------
