/*
** Copyright 2007-2013, 2017 S�lyom Zolt�n
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef FORMSTATE_H
#define FORMSTATE_H

// Collection of classes that can save and restore the state (size, widget data etc.) of
// windows in the program.

#include <QString>
#include <QRect>

#include <map>

class QMainWindow;
class CollectWordsForm;
class QSplitter;

class QXmlStreamWriter;

struct SplitterFormData
{
    // Top left corner of the window with frame.
    QPoint pos;
    // Size of the window geometry without frame.
    QSize siz;
    // Splitter widget sizes.
    int wsizes[2];
};


struct ListStateData
{
    // List holding pairs of [column index, column width].
    std::vector<std::pair<int, int>> columns;
};

enum class SearchMode : uchar;
enum class ExampleDisplay : uchar;
enum class Inclusion : uchar;

struct DictionaryWidgetData
{
    SearchMode mode = (SearchMode)1;
    bool multi = false;
    bool filter = false;

    bool showex = true;
    ExampleDisplay exmode = (ExampleDisplay)0;

    bool frombefore = false;
    bool fromafter = true;
    bool fromstrict = false;
    bool frominfl = false;

    bool toafter = true;
    bool tostrict = false;

    Inclusion conditionex = (Inclusion)0;
    Inclusion conditiongroup = (Inclusion)0;

    // List holding pairs of [Filter name, Inclusion data] pairs.
    std::vector<std::pair<QString, Inclusion>> conditions;

    // Only saving sort and tabledata if this is set to true. WordStudyListForm has to save
    // column states separately, and saving them here is useless.
    bool savecolumndata = true;

    int sortcolumn = -1;
    Qt::SortOrder sortorder = Qt::AscendingOrder;

    ListStateData tabledata;
};

struct CollectFormData
{
    QString kanjinum = "3";
    int kanalen = 8;
    int minfreq = 1000;

    bool strict = true;
    bool limit = true;

    // Saved dimensions of the window.
    QSize siz;

    DictionaryWidgetData dict;
};

struct KanjiInfoData
{
    // Window dimensions.
    QSize siz;

    bool grid = true;
    bool sod = true;

    bool words = false;
    bool similar = false;
    bool parts = false;
    bool partof = false;

    bool shadow = false;
    bool numbers = false;
    bool dir = false;
    int speed = 3;

    // Splitter widget sizes. First value is the stroke diagram with all its controls and
    // kanji information, the second is the dictionary list. The first value is only used when
    // the dictionary words listing should be also visible. Otherwise when opening the
    // splitter, the first value is based on the current size of the top part.
    int toph = -1;
    int dicth = -1;

    // The dictionary widget is set to show kanji example words.
    bool refdict = false;
    DictionaryWidgetData dict;
};

struct KanjiInfoFormData
{
    // The information window is locked. Getting information of another kanji will force open
    // the next window.
    bool locked = false;
    // Position of the window.
    QPoint pos;

    int kindex;
    QString dictname;

    KanjiInfoData data;
};

enum class DeckViewModes : int;
struct WordStudyListFormData
{
    // Window dimensions.
    QSize siz;

    bool showkanji = true;
    bool showkana = true;
    bool showdef = true;

    DeckViewModes mode = (DeckViewModes)0;

    DictionaryWidgetData dict;

    struct SortData
    {
        int column = -1;
        Qt::SortOrder order;
    };

    SortData queuesort;
    SortData studysort;
    SortData testedsort;

    // Column widths.
    std::vector<int> queuesizes;
    std::vector<int> studysizes;
    std::vector<int> testedsizes;

    // Column visibility.
    std::vector<char> queuecols;
    std::vector<char> studycols;
    std::vector<char> testedcols;
};

struct PopupDictData
{
    // Position of the popup dictionary. When false, it sits at the bottom of the screen.
    bool floating = false;
    // Popup dictionary window's position when floating.
    QRect floatrect;
    // Popup dictionary window's size when not floating.
    QSize normalsize;

    DictionaryWidgetData dict;
};

struct KanjiFilterData;
struct PopupKanjiData;

namespace FormStates
{
    // Saved state of dialog windows with splitter.
    extern std::map<QString, SplitterFormData> splitter;

    // Saved state of the CollectWordsForm window.
    extern CollectFormData collectform;

    extern KanjiInfoData kanjiinfo;
    extern WordStudyListFormData wordstudylist;
    extern PopupKanjiData popupkanji;
    extern PopupDictData popupdict;

    bool emptyState(const SplitterFormData &data);
    bool emptyState(const CollectFormData &data);
    bool emptyState(const DictionaryWidgetData &data);
    bool emptyState(const ListStateData &data);
    bool emptyState(const KanjiInfoData &data);
    bool emptyState(const WordStudyListFormData &data);
    bool emptyState(const KanjiFilterData &data);
    bool emptyState(const PopupKanjiData &data);
    bool emptyState(const PopupDictData &data);

    void saveXMLSettings(const SplitterFormData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(SplitterFormData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const CollectFormData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(CollectFormData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const DictionaryWidgetData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(DictionaryWidgetData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const ListStateData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(ListStateData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const KanjiInfoData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(KanjiInfoData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const KanjiInfoFormData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(KanjiInfoFormData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const WordStudyListFormData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(WordStudyListFormData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const KanjiFilterData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(KanjiFilterData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const PopupKanjiData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(PopupKanjiData &data, QXmlStreamReader &reader);

    void saveXMLSettings(const PopupDictData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(PopupDictData &data, QXmlStreamReader &reader);

    void saveDialogSplitterState(QString statename, QMainWindow *window, QSplitter *splitter);
    void restoreDialogSplitterState(QString statename, QMainWindow *window, QSplitter *splitter);

    void loadXMLDialogSplitterState(QXmlStreamReader &reader);
}

#endif // FORMSTATE_H

