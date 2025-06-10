cmake_minimum_required(VERSION 3.16)
project(zkanji LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets Svg PrintSupport Network REQUIRED)
if (UNIX)
    find_package(Qt5 COMPONENTS X11Extras REQUIRED)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_definitions(-D_DEBUG)

set(SOURCE_FILES
        src/bits.cpp
        src/collectwordsform.cpp
        src/collectwordsform.ui
        src/definitionwidget.cpp
        src/definitionwidget.ui
        src/dialogwindow.cpp
        src/dictionaryeditorform.cpp
        src/dictionaryeditorform.ui
        src/dictionaryexportform.cpp
        src/dictionaryexportform.ui
        src/dictionaryimportform.cpp
        src/dictionaryimportform.ui
        src/dictionarystatsform.cpp
        src/dictionarystatsform.ui
        src/dictionarytextform.cpp
        src/dictionarytextform.ui
        src/dictionarywidget.cpp
        src/dictionarywidget.ui
        src/examplewidget.cpp
        src/examplewidget.ui
        src/filterlistform.cpp
        src/filterlistform.ui
        src/formstates.cpp
        src/furigana.cpp
        src/globalui.cpp
        src/grammar.cpp
        src/groupexportform.cpp
        src/groupexportform.ui
        src/groupimportform.cpp
        src/groupimportform.ui
        src/grouppickerform.cpp
        src/grouppickerform.ui
        src/groups.cpp
        src/groupslegacy.cpp
        src/groupstudy.cpp
        src/groupstudylegacy.cpp
        src/groupwidget.cpp
        src/groupwidget.ui
        src/import.cpp
        src/import.ui
        src/importreplaceform.cpp
        src/importreplaceform.ui
        src/jlptreplaceform.cpp
        src/jlptreplaceform.ui
        src/kanapracticesettingsform.cpp
        src/kanapracticesettingsform.ui
        src/kanareadingpracticeform.cpp
        src/kanareadingpracticeform.ui
        src/kanawritingpracticeform.cpp
        src/kanawritingpracticeform.ui
        src/kanji.cpp
        src/kanjidefform.cpp
        src/kanjidefform.ui
        src/kanjigroupwidget.cpp
        src/kanjigroupwidget.ui
        src/kanjiinfoform.cpp
        src/kanjiinfoform.ui
        src/kanjilegacy.cpp
        src/kanjireadingpracticeform.cpp
        src/kanjireadingpracticeform.ui
        src/kanjisearchwidget.cpp
        src/kanjisearchwidget.ui
        src/kanjistrokes.cpp
        src/kanjitogroupform.cpp
        src/kanjitogroupform.ui
        src/kanjitooltipwidget.cpp
        src/languageform.cpp
        src/languageform.ui
        src/languages.cpp
        src/main.cpp
        src/popupdict.cpp
        src/popupdict.ui
        src/popupkanjisearch.cpp
        src/popupkanjisearch.ui
        src/printpreviewform.cpp
        src/printpreviewform.ui
        src/qcharstring.cpp
        src/radform.cpp
        src/radform.ui
        src/ranges.cpp
        src/recognizerform.cpp
        src/recognizer.ui
        src/romajizer.cpp
        src/searchtree.cpp
        src/searchtreelegacy.cpp
        src/selectdictionarydialog.cpp
        src/selectdictionarydialog.ui
        src/sentences.cpp
        src/settings.cpp
        src/settingsform.cpp
        src/settingsform.ui
        src/sites.cpp
        src/stayontop_x11.cpp
        src/studydecks.cpp
        src/studydeckslegacy.cpp
        src/treebuilder.cpp
        src/wordattribwidget.cpp
        src/wordattribwidget.ui
        src/worddeck.cpp
        src/worddeckform.cpp
        src/worddeckform.ui
        src/worddecklegacy.cpp
        src/wordeditorform.cpp
        src/wordeditorform.ui
        src/wordgroupwidget.cpp
        src/wordgroupwidget.ui
        src/words.cpp
        src/wordslegacy.cpp
        src/wordstudyform.cpp
        src/wordstudyform.ui
        src/wordstudylistform.cpp
        src/wordstudylistform.ui
        src/wordtestresultsform.cpp
        src/wordtestresultsform.ui
        src/wordtestsettingsform.cpp
        src/wordtestsettingsform.ui
        src/wordtodeckform.cpp
        src/wordtodeckform.ui
        src/wordtodictionaryform.cpp
        src/wordtodictionaryform.ui
        src/wordtogroupform.cpp
        src/wordtogroupform.ui
        src/zabstracttablemodel.cpp
        src/zabstracttreemodel.cpp
        src/zcolorcombobox.cpp
        src/zcombobox.cpp
        src/zdictionariesmodel.cpp
        src/zdictionarycombobox.cpp
        src/zdictionarylistview.cpp
        src/zdictionarymodel.cpp
        src/zevents.cpp
        src/zexamplestrip.cpp
        src/zflowlayout.cpp
        src/zgrouptreemodel.cpp
        src/zitemscroller.cpp
        src/zkanalineedit.cpp
        src/zkanjidiagram.cpp
        src/zkanjiform.cpp
        src/zkanjiform.ui
        src/zkanjigridmodel.cpp
        src/zkanjigridview.cpp
        src/zkanjimain.cpp
        src/zkanjiwidget.cpp
        src/zkanjiwidget.ui
        src/zlineedit.cpp
        src/zlistbox.cpp
        src/zlistboxmodel.cpp
        src/zlistview.cpp
        src/zlistviewitemdelegate.cpp
        src/zproxytablemodel.cpp
        src/zradicalgrid.cpp
        src/zscrollarea.cpp
        src/zstackedwidget.cpp
        src/zstatusbar.cpp
        src/zstatview.cpp
        src/zstrings.cpp
        src/zstudylistmodel.cpp
        src/ztooltip.cpp
        src/ztreeview.cpp
        src/zui.cpp
        src/zwindow.cpp

        src/Qxt/qxtglobal.cpp
        src/Qxt/qxtglobalshortcut.cpp
        src/zabstractstatmodel.cpp

        Resources/resources.qrc
)

if (UNIX)
    list(APPEND SOURCE_FILES src/Qxt/qxtglobalshortcut_x11.cpp)
elseif (WIN32)
    list(APPEND SOURCE_FILES src/Qxt/qxtglobalshortcut_win.cpp)
elseif (MACOS)
    list(APPEND SOURCE_FILES src/Qxt/qxtglobalshortcut_mac.cpp)
endif()

include_directories(src)

add_executable(zkanji ${SOURCE_FILES})
target_link_libraries(zkanji PRIVATE Qt5::Widgets Qt5::Svg Qt5::PrintSupport Qt5::Network)

if (UNIX)
    target_link_libraries(zkanji PRIVATE Qt5::X11Extras X11 xcb)
endif()

