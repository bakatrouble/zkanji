/*
** Copyright 2007-2013, 2017 S�lyom Zolt�n
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QSettings>
#include <QFile>
#include <QFontDatabase>
#include <QDir>
#include <QByteArray>
#include "settings.h"
#include "fontsettings.h"
#include "generalsettings.h"
#include "printsettings.h"
#include "dialogsettings.h"
#include "popupsettings.h"
#include "recognizersettings.h"
#include "colorsettings.h"
#include "dictionarysettings.h"
#include "groupsettings.h"
#include "shortcutsettings.h"
#include "kanjisettings.h"
#include "studysettings.h"
#include "datasettings.h"
#include "zkanjimain.h"
#include "globalui.h"
#include "zkanjiform.h"
#include "zradicalgrid.h"
#include "zui.h"
#include "formstate.h"
#include "kanji.h"
#include "sites.h"
#include "groups.h"
#include "words.h"

namespace Settings
{
    FontSettings fonts;
    GeneralSettings general;
    GroupSettings group;
    PrintSettings print;
    DialogSettings dialog;
    PopupSettings popup;
    RecognizerSettings recognizer;
    ColorSettings colors;
    DictionarySettings dictionary;
    ShortcutSettings shortcuts;
    KanjiSettings kanji;
    StudySettings study;
    DataSettings data;

    void saveIniFile()
    {
        QSettings ini(ZKanji::userFolder() + "/zkanji.ini", QSettings::IniFormat);

        // General settings

        ini.setValue("dateformat", general.dateformat == GeneralSettings::YearMonthDay ? "YearMonthDay" : general.dateformat == GeneralSettings::MonthDayYear ? "MonthDayYear" : "DayMonthYear");
        ini.setValue("savewinstates", general.savewinstates);
        ini.setValue("savetoolstates", general.savetoolstates);
        ini.setValue("startstate", (int)general.startstate);
        ini.setValue("minimizetotray", general.minimizetotray);

        // Font settings

        ini.setValue("fonts/kanji", fonts.kanji);
        ini.setValue("fonts/kanjifontsize", fonts.kanjifontsize);

        ini.setValue("fonts/nokanjialias", fonts.nokanjialias);
        ini.setValue("fonts/kana", fonts.kana);
        ini.setValue("fonts/definition", fonts.definition);
        ini.setValue("fonts/notes", fonts.info);
        ini.setValue("fonts/notesstyle", fonts.infostyle == FontSettings::Bold ? "Bold" : fonts.infostyle == FontSettings::Italic ? "Italic" : fonts.infostyle == FontSettings::BoldItalic ? "BoldItalic" : "Normal");
        ini.setValue("fonts/extra", fonts.extra);
        ini.setValue("fonts/extrastyle", fonts.extrastyle == FontSettings::Bold ? "Bold" : fonts.extrastyle == FontSettings::Italic ? "Italic" : fonts.extrastyle == FontSettings::BoldItalic ? "BoldItalic" : "Normal");

        ini.setValue("fonts/printkana", fonts.printkana);
        ini.setValue("fonts/printdefinition", fonts.printdefinition);
        ini.setValue("fonts/printinfo", fonts.printinfo);
        ini.setValue("fonts/printinfostyle", fonts.printinfostyle == FontSettings::Bold ? "Bold" : fonts.printinfostyle == FontSettings::Italic ? "Italic" : fonts.printinfostyle == FontSettings::BoldItalic ? "BoldItalic" : "Normal");

        ini.setValue("fonts/mainsize", (int)fonts.mainsize);
        ini.setValue("fonts/popsize", (int)fonts.popsize);

        // Group settings

        ini.setValue("groups/hidesuspended", group.hidesuspended);

        // Print settings

        ini.setValue("print/usedictionaryfonts", print.dictfonts);
        ini.setValue("print/doublepage", print.doublepage);
        ini.setValue("print/separatecolumns", print.separated);
        ini.setValue("print/linesize", (int)print.linesize);
        ini.setValue("print/columns", print.columns);
        ini.setValue("print/usekanji", print.usekanji);
        ini.setValue("print/showtype", print.showtype);
        ini.setValue("print/userdefs", print.userdefs);
        ini.setValue("print/reversed", print.reversed);
        ini.setValue("print/readings", (int)print.readings);
        ini.setValue("print/background", print.background);
        ini.setValue("print/separatorline", print.separator);
        ini.setValue("print/pagenum", print.pagenum);
        ini.setValue("print/printername", print.printername);
        ini.setValue("print/portraitmode", print.portrait);
        ini.setValue("print/leftmargin", print.leftmargin);
        ini.setValue("print/topmargin", print.topmargin);
        ini.setValue("print/rightmargin", print.rightmargin);
        ini.setValue("print/bottommargin", print.bottommargin);
        ini.setValue("print/sizeid", print.pagesizeid);

        // Dialog settings

        ini.setValue("dialogs/defaultwordgroup", dialog.defwordgroup);
        ini.setValue("dialogs/defaultkanjigroup", dialog.defkanjigroup);
        ini.setValue("dialogs/settingspage", dialog.lastsettingspage);

        // Popup settings

        ini.setValue("popup/autohide", popup.autohide);
        ini.setValue("popup/widescreen", popup.widescreen);
        //ini.setValue("popup/floating", popup.floating);
        //ini.setValue("popup/floatrect", popup.dictfloatrect);
        //ini.setValue("popup/size", popup.normalsize);
        ini.setValue("popup/autosearch", (int)popup.activation);
        ini.setValue("popup/transparency", popup.transparency);
        ini.setValue("popup/dictionary", popup.dict);
        //ini.setValue("popup/kanjirect", popup.kanjifloatrect);
        ini.setValue("popup/kanjiautohide", popup.kanjiautohide);
        ini.setValue("popup/kanjidictionary", popup.kanjidict);

        // Recognizer settings

        ini.setValue("recognizer/savesize", recognizer.savesize);
        ini.setValue("recognizer/savepos", recognizer.saveposition);
        ini.setValue("recognizer/rect", recognizer.rect);

        // Color settings

        ini.setValue("colors/grid", colors.grid);
        ini.setValue("colors/bg", colors.bg);
        ini.setValue("colors/text", colors.text);
        ini.setValue("colors/selbg", colors.selbg);
        ini.setValue("colors/seltext", colors.seltext);
        ini.setValue("colors/bgi", colors.bgi);
        ini.setValue("colors/texti", colors.texti);
        ini.setValue("colors/selbgi", colors.selbgi);
        ini.setValue("colors/seltexti", colors.seltexti);
        ini.setValue("colors/inf", colors.inf);
        ini.setValue("colors/attrib", colors.attrib);
        ini.setValue("colors/types", colors.types);
        ini.setValue("colors/notes", colors.notes);
        ini.setValue("colors/fields", colors.fields);
        ini.setValue("colors/dialect", colors.dialect);
        ini.setValue("colors/kanaonly", colors.kanaonly);
        ini.setValue("colors/kanjiexamplebg", colors.kanjiexbg);
        ini.setValue("colors/n5", colors.n5);
        ini.setValue("colors/n4", colors.n4);
        ini.setValue("colors/n3", colors.n3);
        ini.setValue("colors/n2", colors.n2);
        ini.setValue("colors/n1", colors.n1);
        ini.setValue("colors/kanjinowords", colors.kanjinowords);
        ini.setValue("colors/kanjiunsorted", colors.kanjiunsorted);
        ini.setValue("colors/coloroku", colors.coloroku);
        ini.setValue("colors/oku", colors.okucolor);
        ini.setValue("colors/katabg", colors.katabg);
        ini.setValue("colors/hirabg", colors.hirabg);
        ini.setValue("colors/similarbg", colors.similarbg);
        ini.setValue("colors/similartext", colors.similartext);
        ini.setValue("colors/partsbg", colors.partsbg);
        ini.setValue("colors/partofbg", colors.partofbg);

        // Dictionary

        ini.setValue("dict/autosize", dictionary.autosize);
        ini.setValue("dict/inflection", (int)dictionary.inflection);
        ini.setValue("dict/browseorder", dictionary.browseorder == BrowseOrder::ABCDE ? "abcde" : "aiueo");
        ini.setValue("dict/showgroup", dictionary.showingroup);
        ini.setValue("dict/showjlpt", dictionary.showjlpt);
        ini.setValue("dict/jlptcol", (int)dictionary.jlptcolumn);
        ini.setValue("dict/historylimit", dictionary.historylimit);
        ini.setValue("dict/historytimer", dictionary.historytimer);
        ini.setValue("dict/historytimeout", dictionary.historytimeout);

        // Shortcut settings

        ini.setValue("shortcuts/from", shortcuts.fromenable);
        ini.setValue("shortcuts/frommod", shortcuts.frommodifier == ShortcutSettings::Alt ? "Alt" : shortcuts.frommodifier == ShortcutSettings::Control ? "Control" : "AltControl");
        ini.setValue("shortcuts/fromshift", shortcuts.fromshift);
        ini.setValue("shortcuts/fromkey", QString(shortcuts.fromkey));

        ini.setValue("shortcuts/to", shortcuts.toenable);
        ini.setValue("shortcuts/tomod", shortcuts.tomodifier == ShortcutSettings::Alt ? "Alt" : shortcuts.tomodifier == ShortcutSettings::Control ? "Control" : "AltControl");
        ini.setValue("shortcuts/toshift", shortcuts.toshift);
        ini.setValue("shortcuts/tokey", QString(shortcuts.tokey));

        ini.setValue("shortcuts/kanji", shortcuts.kanjienable);
        ini.setValue("shortcuts/kanjimod", shortcuts.kanjimodifier == ShortcutSettings::Alt ? "Alt" : shortcuts.kanjimodifier == ShortcutSettings::Control ? "Control" : "AltControl");
        ini.setValue("shortcuts/kanjishift", shortcuts.kanjishift);
        ini.setValue("shortcuts/kanjikey", QString(shortcuts.kanjikey));

        // Kanji settings

        ini.setValue("kanji/savefilters", kanji.savefilters);
        ini.setValue("kanji/popupreset", kanji.resetpopupfilters);
        ini.setValue("kanji/ref1", kanji.mainref1);
        ini.setValue("kanji/ref2", kanji.mainref2);
        ini.setValue("kanji/ref3", kanji.mainref3);
        ini.setValue("kanji/ref4", kanji.mainref4);
        ini.setValue("kanji/parts", kanji.listparts);
        ini.setValue("kanji/tooltip", kanji.tooltip);
        ini.setValue("kanji/hidetooltip", kanji.hidetooltip);
        ini.setValue("kanji/tooltipdelay", kanji.tooltipdelay);

        for (int ix = 0; ix != 33; ++ix)
        {
            ini.setValue("kanji/showref" + QString::number(ix), kanji.showref[ix]);
            ini.setValue("kanji/reforder" + QString::number(ix), kanji.reforder[ix]);
        }

        // Study settings

        ini.setValue("study/starthour", study.starthour);
        ini.setValue("study/includefreq", (int)study.includedays);
        ini.setValue("study/includecnt", study.includecount);
        ini.setValue("study/onlyunique", study.onlyunique);
        ini.setValue("study/limitnew", study.limitnew);
        ini.setValue("study/limitcount", study.limitcount);

        ini.setValue("study/kanjihint", (int)study.kanjihint);
        ini.setValue("study/kanahint", (int)study.kanahint);
        ini.setValue("study/defhint", (int)study.defhint);

        ini.setValue("study/showestimate", study.showestimate);
        ini.setValue("study/writekanji", study.writekanji);
        ini.setValue("study/writekana", study.writekana);
        ini.setValue("study/writedef", study.writedef);
        ini.setValue("study/delaywrong", study.delaywrong);
        ini.setValue("study/review", study.review);
        ini.setValue("study/idlesuspend", study.idlesuspend);
        ini.setValue("study/idletime", (int)study.idletime);
        ini.setValue("study/itemsuspend", study.itemsuspend);
        ini.setValue("study/itemsuspendnum", study.itemsuspendnum);
        ini.setValue("study/timesuspend", study.timesuspend);
        ini.setValue("study/timesuspendnum", study.timesuspendnum);

        ini.setValue("study/readings", (int)study.readings);
        ini.setValue("study/readingsfrom", (int)study.readingsfrom);

        // Data settings

        ini.setValue("data/autosave", data.autosave);
        ini.setValue("data/interval", data.interval);
        ini.setValue("data/backup", data.backup);
        ini.setValue("data/backupcount", data.backupcnt);
        ini.setValue("data/backupskip", data.backupskip);
        ini.setValue("data/location", data.location);

        // Dictionary order

        QStringList str;
        for (int ix = 0, siz = ZKanji::dictionaryCount(); ix != siz; ++ix)
            str << ZKanji::dictionary(ZKanji::dictionaryPosition(ix))->name();

        ini.setValue("dictionaryorder", str.join(QChar('/')));
    }

    void saveStatesFile()
    {
        QString fname = ZKanji::userFolder() + "/states.xml";

        QFile f(fname);
        if (!f.open(QIODevice::WriteOnly))
        {
            QMessageBox::warning(gUI->activeMainForm(), "zkanji", gUI->tr("Couldn't create or overwrite the state settings file. Please make sure you have permission to modify the file %1.").arg(QDir::toNativeSeparators(fname)), QMessageBox::Ok);
            return;
        }

        QXmlStreamWriter writer(&f);
        writer.setAutoFormatting(true);

        writer.writeStartDocument();

        writer.writeStartElement("ZKanjiSettings");

        if (!radicalFiltersModel().empty() || !wordFiltersEmpty())
        {
            writer.writeStartElement("Filters");

            if (!radicalFiltersModel().empty())
            {
                writer.writeStartElement("RadicalFilters");
                saveXMLRadicalFilters(writer);
                writer.writeEndElement(); /* RadicalFilters */
            }

            if (!wordFiltersEmpty())
            {
                writer.writeStartElement("WordFilters");
                saveXMLWordFilters(writer);
                writer.writeEndElement(); /* WordFilters */
            }

            writer.writeEndElement(); /* Filters */
        }

        if (!ZKanji::lookup_sites.empty())
        {
            writer.writeStartElement("LookupSites");
            ZKanji::saveXMLLookupSites(writer);
            writer.writeEndElement(); /* LookupSites */
        }

        writer.writeStartElement("WindowStates");

        for (auto &state : FormStates::splitter)
        {
            if (!FormStates::emptyState(state.second))
            {
                writer.writeStartElement(state.first);
                FormStates::saveXMLSettings(state.second, writer);
                writer.writeEndElement();
            }
        }

        if (!FormStates::emptyState(FormStates::collectform))
        {
            writer.writeStartElement("CollectWords");
            FormStates::saveXMLSettings(FormStates::collectform, writer);
            writer.writeEndElement();
        }

        if (!FormStates::emptyState(FormStates::kanjiinfo))
        {
            writer.writeStartElement("KanjiInformation");
            FormStates::saveXMLSettings(FormStates::kanjiinfo, writer);
            writer.writeEndElement();
        }

        if (!FormStates::emptyState(FormStates::wordstudylist))
        {
            writer.writeStartElement("WordStudyListForm");
            FormStates::saveXMLSettings(FormStates::wordstudylist, writer);
            writer.writeEndElement();
        }

        if (!FormStates::emptyState(FormStates::popupkanji))
        {
            writer.writeStartElement("PopupKanji");
            FormStates::saveXMLSettings(FormStates::popupkanji, writer);
            writer.writeEndElement();
        }

        if (!FormStates::emptyState(FormStates::popupdict))
        {
            writer.writeStartElement("PopupDictionary");
            FormStates::saveXMLSettings(FormStates::popupdict, writer);
            writer.writeEndElement();
        }

        writer.writeEndElement(); /* WindowStates */

        writer.writeStartElement("Windows");

        writer.writeStartElement("MainWindow");
        gUI->mainForm()->saveXMLSettings(writer);
        writer.writeEndElement(); /* MainWindow */

        for (int ix = 1, siz = gUI->formCount(); ix != siz; ++ix)
        {
            writer.writeStartElement("Window");
            gUI->forms(ix)->saveXMLSettings(writer);
            writer.writeEndElement(); /* Window */
        }

        if (Settings::general.savetoolstates)
        {
            writer.writeStartElement("KanjiInfoWindows");
            gUI->saveXMLKanjiInfo(writer);
            writer.writeEndElement(); /* KanjiInfoWindows */
        }

        writer.writeEndElement(); /* Windows */

        writer.writeEndElement(); /* ZKanjiSettings */

        writer.writeEndDocument();

        if (writer.hasError())
        {
            f.close();

            int r = showAndReturn(gUI->activeMainForm(), "zkanji", gUI->tr("There was an error writing the state settings file."), gUI->tr("Please make sure there's enough disk space to write to %1 and \"%2\".").arg(QDir::toNativeSeparators(ZKanji::userFolder())).arg(gUI->tr("Try again")),
            {
                { gUI->tr("Try again"), QMessageBox::AcceptRole },
                { gUI->tr("Cancel"), QMessageBox::RejectRole }
            });
            if (r == 0)
                saveSettingsToFile();
            return;
        }
    }

    void saveSettingsToFile()
    {
        saveIniFile();
        saveStatesFile();
    }

    static bool hasSimSun = false;
    static QString defaultJapaneseFont(const QStringList &allJpFonts)
    {
        QString defjp;

        // Find either Meiryo (Win), MS UI Gothic (Win), MS Gothic (Win), Hiragino Kaku Gothic Pro (Mac), Osaka (Mac), Arial Unicode
        if (!allJpFonts.isEmpty())
        {
#ifdef Q_OS_MAC
            if (allJpFonts.indexOf("Hiragino Kaku Gothic Pro") != -1)
                defjp = "Hiragino Kaku Gothic Pro";
            else if (allJpFonts.indexOf("Osaka") != -1)
                defjp = "Osaka";
            else
            {
                // Try to find a font which starts with Osaka and use the first one found.
                for (QString &s : allJpFonts)
                {
                    if (s.startsWith("Osaka"))
                    {
                        defjp = s;
                        break;
                    }
                }
            }
#elif defined Q_OS_WIN
            if (defjp.isEmpty())
            {
                // Try to find Meiryo, MS UI Gothic, MS Gothic, Arial Unicode in this order.
                int mix = -1;
                bool mx = false;
                int uix = -1;
                bool ux = false;
                int gix = -1;
                bool gx = false;

                int ix = 0;
                for (const QString &s : allJpFonts)
                {
                    if (!mx && s.startsWith("Meiryo"))
                    {
                        if (s == "Meiryo")
                            mx = true;
                        mix = ix;
                        if (mx)
                            break;
                    }
                    if (!ux && s.startsWith("MS UI Gothic"))
                    {
                        if (s == "MS UI Gothic")
                            ux = true;
                        uix = ix;
                    }
                    else if (!gx && s.startsWith("MS Gothic"))
                    {
                        if (s == "MS Gothic")
                            gx = true;
                        gix = ix;
                    }
                    //else if (!ax && s == "Arial Unicode")
                    //{
                    //    if (s == "Arial Unicode" || s == "Arial Unicode MS")
                    //        ax = true;
                    //    aix = ix;
                    //}
                    if (mx && ux && gx)
                        break;
                    ++ix;
                }

                if (mix != -1)
                    defjp = allJpFonts.at(mix);
                else if (uix != -1)
                    defjp = allJpFonts.at(uix);
                else if (gx)
                    defjp = allJpFonts.at(gix);
            }
#endif
            if (defjp.isEmpty())
            {
                for (const QString &s : allJpFonts)
                {
                    if (s.startsWith("Arial Unicode"))
                    {
                        defjp = s;
                        break;
                    }
                }
            }

            if (defjp.isEmpty())
            {
                // Find the first font that starts with "Arial" or "Adobe" or contains "Gothic".
                for (const QString &s : allJpFonts)
                {
                    if (s.startsWith("Arial") || s.startsWith("Adobe") || s.contains("Gothic"))
                    {
                        defjp = s;
                        break;
                    }
                }
            }

            // Use a random font.
            if (defjp.isEmpty())
                defjp = allJpFonts.at(rnd(0, allJpFonts.size() - 1));
        }


        if (defjp.isEmpty())
            defjp = qApp->font().family();

        return defjp;
    }

    void loadIniFile()
    {
        const int kanjirefcnt = ZKanji::kanjiReferenceCount();

        memset(kanji.showref, true, sizeof(bool) * kanjirefcnt);
        for (int ix = 0, siz = kanjirefcnt; ix != siz; ++ix)
            kanji.reforder[ix] = ix;

        QSettings ini(ZKanji::userFolder() + "/zkanji.ini", QSettings::IniFormat);

        QString tmp;
        bool ok;
        int val;

        // General settings

        tmp = ini.value("dateformat", "DayMonthYear").toString();
        general.dateformat = tmp == "YearMonthDay" ? GeneralSettings::YearMonthDay : tmp == "MonthDayYear" ? GeneralSettings::MonthDayYear : GeneralSettings::DayMonthYear;

        //val = ini.value("settingspage", 0).toInt(&ok);
        //if (val >= 0 && val <= 999)
        //    general.settingspage = val;

        general.savewinstates = ini.value("savewinstates", true).toBool();
        general.savetoolstates = ini.value("savetoolstates", true).toBool();
        val = ini.value("startstate", 0).toInt(&ok);
        if (ok && val >= 0 && val <= 3)
            general.startstate = (GeneralSettings::StartState)val;
        general.minimizetotray = ini.value("minimizetotray", false).toBool();

        // Font settings

        fonts.kanji = ini.value("fonts/kanji", QString()).toString();
        val = ini.value("fonts/kanjifontsize", QString()).toInt(&ok);
        if (ok)
            fonts.kanjifontsize = val;
        fonts.nokanjialias = ini.value("fonts/nokanjialias", true).toBool();
        fonts.kana = ini.value("fonts/kana", QString()).toString();
        fonts.definition = ini.value("fonts/definition", qApp->font().family()).toString();
        fonts.info = ini.value("fonts/notes", qApp->font().family()).toString();
        tmp = ini.value("fonts/notesstyle", "Normal").toString();
        fonts.infostyle = (tmp == "Bold") ? FontSettings::Bold : (tmp == "Italic") ? FontSettings::Italic : (tmp == "BoldItalic") ? FontSettings::BoldItalic : FontSettings::Normal;
        fonts.extra = ini.value("fonts/extra", qApp->font().family()).toString();
        tmp = ini.value("fonts/extrastyle", "Bold").toString();
        fonts.extrastyle = (tmp == "Bold") ? FontSettings::Bold : (tmp == "Italic") ? FontSettings::Italic : (tmp == "BoldItalic") ? FontSettings::BoldItalic : FontSettings::Normal;

        fonts.printkana = ini.value("fonts/printkana", QString()).toString();
        fonts.printdefinition = ini.value("fonts/printdefinition", qApp->font().family()).toString();
        fonts.printinfo = ini.value("fonts/printinfo", qApp->font().family()).toString();
        tmp = ini.value("fonts/printinfostyle", "Italic").toString();
        fonts.printinfostyle = (tmp == "Bold") ? FontSettings::Bold : (tmp == "Italic") ? FontSettings::Italic : (tmp == "BoldItalic") ? FontSettings::BoldItalic : FontSettings::Normal;

        val = ini.value("fonts/mainsize", 1).toInt(&ok);
        if (ok && val >= 0 && val <= 2)
            fonts.mainsize = (FontSettings::LineSize)val;
        val = ini.value("fonts/popsize", 1).toInt(&ok);
        if (ok && val >= 0 && val <= 2)
            fonts.popsize = (FontSettings::LineSize)val;

        // Group settings

        group.hidesuspended = ini.value("groups/hidesuspended", true).toBool();

        // Print settings

        print.dictfonts = ini.value("print/usedictionaryfonts", true).toBool();
        print.doublepage = ini.value("print/doublepage", false).toBool();
        print.separated = ini.value("print/separatecolumns", false).toBool();

        val = ini.value("print/linesize", 2).toInt(&ok);
        if (ok && val >= 0 && val <= 6)
            print.linesize = (PrintSettings::LineSizes)val;
        val = ini.value("print/columns", 2).toInt(&ok);
        if (ok && val >= 1 && val <= 3)
            print.columns = val;

        print.usekanji = ini.value("print/usekanji", true).toBool();
        print.showtype = ini.value("print/showtype", true).toBool();
        print.userdefs = ini.value("print/userdefs", true).toBool();
        print.reversed = ini.value("print/reversed", false).toBool();

        val = ini.value("print/readings", 2).toInt(&ok);
        if (ok && val >= 0 && val <= 2)
            print.readings = (PrintSettings::Readings)val;

        print.background = ini.value("print/background", false).toBool();
        print.separator = ini.value("print/separatorline", false).toBool();
        print.pagenum = ini.value("print/pagenum", false).toBool();

        ini.value("print/printername", print.printername);
        print.portrait = ini.value("print/portraitmode", true).toBool();
        val = ini.value("print/leftmargin", 16).toInt(&ok);
        if (ok && val >= 0)
            print.leftmargin = val;
        val = ini.value("print/topmargin", 16).toInt(&ok);
        if (ok && val >= 0)
            print.topmargin = val;
        val = ini.value("print/rightmargin", 16).toInt(&ok);
        if (ok && val >= 0)
            print.rightmargin = val;
        val = ini.value("print/bottommargin", 16).toInt(&ok);
        if (ok && val >= 0)
            print.bottommargin = val;
        val = ini.value("print/sizeid", -1).toInt(&ok);
        if (ok && val >= -1)
            print.pagesizeid = val;


        // Dialog settings

        dialog.defwordgroup = ini.value("dialogs/defaultwordgroup", QString()).toString();
        dialog.defkanjigroup = ini.value("dialogs/defaultkanjigroup", QString()).toString();
        dialog.lastsettingspage = ini.value("dialogs/settingspage", QString()).toString();

        // Popup settings

        popup.autohide = ini.value("popup/autohide", true).toBool();
        popup.widescreen = ini.value("popup/widescreen", false).toBool();
        //popup.floating = ini.value("popup/floating", false).toBool();
        //popup.dictfloatrect = ini.value("popup/floatrect", QRect()).toRect();
        //popup.normalsize = ini.value("popup/size", QSize()).toSize();
        val = ini.value("popup/autosearch", 0).toInt(&ok);
        if (ok && val >= 0 && val <= 2)
            popup.activation = (PopupSettings::Activation)val;
        val = ini.value("popup/transparency", 0).toInt(&ok);
        if (ok && val >= 0 && val <= 10)
            popup.transparency = val;

        popup.dict = ini.value("popup/dictionary", QString()).toString();

        //popup.kanjifloatrect = ini.value("popup/kanjirect", QRect()).toRect();
        popup.kanjiautohide = ini.value("popup/kanjiautohide", false).toBool();

        popup.kanjidict = ini.value("popup/kanjidictionary", QString()).toString();

        // Recognizer settings

        recognizer.savesize = ini.value("recognizer/savesize", true).toBool();
        recognizer.saveposition = ini.value("recognizer/savepos", false).toBool();
        recognizer.rect = ini.value("recognizer/rect", recognizer.rect).toRect();

        // Color settings

        colors.grid = ini.value("colors/grid").value<QColor>();
        colors.bg = ini.value("colors/bg").value<QColor>();
        colors.text = ini.value("colors/text").value<QColor>();
        colors.selbg = ini.value("colors/selbg").value<QColor>();
        colors.seltext = ini.value("colors/seltext").value<QColor>();
        colors.bgi = ini.value("colors/bgi").value<QColor>();
        colors.texti = ini.value("colors/texti").value<QColor>();
        colors.selbgi = ini.value("colors/selbgi").value<QColor>();
        colors.seltexti = ini.value("colors/seltexti").value<QColor>();
        colors.inf = ini.value("colors/inf").value<QColor>();
        colors.attrib = ini.value("colors/attrib").value<QColor>();
        colors.types = ini.value("colors/types").value<QColor>();
        colors.notes = ini.value("colors/notes").value<QColor>();
        colors.fields = ini.value("colors/fields").value<QColor>();
        colors.dialect = ini.value("colors/dialect").value<QColor>();
        colors.kanaonly = ini.value("colors/kanaonly").value<QColor>();
        colors.kanjiexbg = ini.value("colors/kanjiexamplebg").value<QColor>();

        colors.n5 = ini.value("colors/n5").value<QColor>();
        colors.n4 = ini.value("colors/n4").value<QColor>();
        colors.n3 = ini.value("colors/n3").value<QColor>();
        colors.n2 = ini.value("colors/n2").value<QColor>();
        colors.n1 = ini.value("colors/n1").value<QColor>();
        colors.kanjinowords = ini.value("colors/kanjinowords").value<QColor>();
        colors.kanjiunsorted = ini.value("colors/kanjiunsorted").value<QColor>();
        colors.coloroku = ini.value("colors/coloroku", true).toBool();
        colors.okucolor = ini.value("colors/oku").value<QColor>();

        colors.katabg = ini.value("colors/katabg").value<QColor>();
        colors.hirabg = ini.value("colors/hirabg").value<QColor>();
        colors.similarbg = ini.value("colors/similarbg").value<QColor>();
        colors.similartext = ini.value("colors/similartext").value<QColor>();
        colors.partsbg = ini.value("colors/partsbg").value<QColor>();
        colors.partofbg = ini.value("colors/partofbg").value<QColor>();

        // Dictionary

        dictionary.autosize = ini.value("dict/autosize", false).toBool();
        val = ini.value("dict/inflection", 1).toInt(&ok);
        if (ok && val >= 0 && val <= 2)
            dictionary.inflection = (DictionarySettings::InflectionShow)val;
        tmp = ini.value("dict/browseorder", (int)BrowseOrder::ABCDE).toString();
        dictionary.browseorder = tmp == "abcde" ? BrowseOrder::ABCDE : BrowseOrder::AIUEO;
        dictionary.showingroup = ini.value("dict/showgroup", false).toBool();
        dictionary.showjlpt = ini.value("dict/showjlpt", true).toBool();
        val = ini.value("dict/jlptcol", 0).toInt(&ok);
        if (ok && val >= 0 && val <= 2)
            dictionary.jlptcolumn = (DictionarySettings::JlptColumn)val;
        val = ini.value("dict/historylimit", 1000).toInt(&ok);
        if (ok && val >= 0 && val < 10000)
            dictionary.historylimit = val;
        dictionary.historytimer = ini.value("dict/historytimer", true).toBool();
        val = ini.value("dict/historytimeout", 4).toInt(&ok);
        if (ok && val > 1 && val <= 100)
            dictionary.historytimeout = val;

        // Shortcut settings

        shortcuts.fromenable = ini.value("shortcuts/from", false).toBool();
        tmp = ini.value("shortcuts/frommod", "AltControl").toString();
        shortcuts.frommodifier = tmp == "Alt" ? ShortcutSettings::Alt : tmp == "Control" ? ShortcutSettings::Control : ShortcutSettings::AltControl;
        shortcuts.fromshift = ini.value("shortcuts/fromshift", false).toBool();
        tmp = ini.value("shortcuts/fromkey", "J").toString();
        if (tmp.size() == 1)
            shortcuts.fromkey = tmp.at(0);
        if (shortcuts.fromkey.unicode() == 0 || shortcuts.fromkey < 'A' || shortcuts.fromkey > 'Z')
        {
            shortcuts.fromenable = false;
            shortcuts.fromkey = 'J';
        }

        shortcuts.toenable = ini.value("shortcuts/to", false).toBool();
        tmp = ini.value("shortcuts/tomod", "AltControl").toString();
        shortcuts.tomodifier = tmp == "Alt" ? ShortcutSettings::Alt : tmp == "Control" ? ShortcutSettings::Control : ShortcutSettings::AltControl;
        shortcuts.toshift = ini.value("shortcuts/toshift", false).toBool();
        tmp = ini.value("shortcuts/tokey", "E").toString();
        if (tmp.size() == 1)
            shortcuts.tokey = tmp.at(0);
        if (shortcuts.tokey < 'A' || shortcuts.tokey > 'Z' || (shortcuts.fromenable && shortcuts.tokey == shortcuts.fromkey))
        {
            shortcuts.toenable = false;
            shortcuts.tokey = 'E';
        }

        shortcuts.kanjienable = ini.value("shortcuts/kanji", false).toBool();
        tmp = ini.value("shortcuts/kanjimod", "AltControl").toString();
        shortcuts.kanjimodifier = tmp == "Alt" ? ShortcutSettings::Alt : tmp == "Control" ? ShortcutSettings::Control : ShortcutSettings::AltControl;
        shortcuts.kanjishift = ini.value("shortcuts/kanjishift", false).toBool();
        tmp = ini.value("shortcuts/kanjikey", "K").toString();
        if (tmp.size() == 1)
            shortcuts.kanjikey = tmp.at(0);
        if (shortcuts.kanjikey < 'A' || shortcuts.kanjikey > 'Z' || (shortcuts.fromenable && shortcuts.kanjikey == shortcuts.fromkey) || (shortcuts.toenable && shortcuts.kanjikey == shortcuts.tokey))
        {
            shortcuts.kanjienable = false;
            shortcuts.kanjikey = 'K';
        }

        // Kanji settings

        kanji.savefilters = ini.value("kanji/savefilters", true).toBool();
        kanji.resetpopupfilters = ini.value("kanji/popupreset", false).toBool();
        val = ini.value("kanji/ref1", 3).toInt(&ok);
        if (ok && val >= 0 && val <= kanjirefcnt)
            kanji.mainref1 = val;
        val = ini.value("kanji/ref2", 4).toInt(&ok);
        if (ok && val >= 0 && val <= kanjirefcnt)
            kanji.mainref2 = val;
        val = ini.value("kanji/ref3", 0).toInt(&ok);
        if (ok && val >= 0 && val <= kanjirefcnt)
            kanji.mainref3 = val;
        val = ini.value("kanji/ref4", 0).toInt(&ok);
        if (ok && val >= 0 && val <= kanjirefcnt)
            kanji.mainref4 = val;
        kanji.listparts = ini.value("kanji/parts", false).toBool();
        kanji.tooltip = ini.value("kanji/tooltip", true).toBool();
        kanji.hidetooltip = ini.value("kanji/hidetooltip", true).toBool();
        val = ini.value("kanji/tooltipdelay", 5).toInt(&ok);
        if (ok && val >= 1 && val <= 9999)
            kanji.tooltipdelay = val;

        QSet<int> foundrefs;
        bool brokenorder = false;
        for (int ix = 0; ix != kanjirefcnt; ++ix)
        {
            kanji.showref[ix] = ini.value("kanji/showref" + QString::number(ix), true).toBool();
            val = ini.value("kanji/reforder" + QString::number(ix), ix).toInt(&ok);
            if (ok && val >= 0 && val < kanjirefcnt)
            {
                if (!foundrefs.contains(val))
                    kanji.reforder[ix] = val;
                else
                    brokenorder = true;
            }
            else
                brokenorder = true;
        }
        if (brokenorder)
            for (int ix = 0; ix != kanjirefcnt; ++ix)
                kanji.reforder[ix] = ix;

        // Study settings

        val = ini.value("study/starthour", 4).toInt(&ok);
        if (ok && val >= 1 && val <= 12)
            study.starthour = val;
        val = ini.value("study/includefreq", (int)StudySettings::One).toInt(&ok);
        if (ok && val >= 0 && val <= 4)
            study.includedays = (StudySettings::IncludeDays)val;
        val = ini.value("study/includecnt", 0).toInt(&ok);
        if (ok && val >= 0 && val <= 9999)
            study.includecount = val;
        study.onlyunique = ini.value("study/onlyunique", true).toBool();
        study.limitnew = ini.value("study/limitnew", true).toBool();
        val = ini.value("study/limitcount", 60).toInt(&ok);
        if (ok && val >= 0 && val <= 9999)
            study.limitcount = val;

        val = ini.value("study/kanjihint", 2).toInt(&ok);
        if (ok && val >= 0 && val <= 3 && val != 0)
            study.kanjihint = (WordParts)val;
        val = ini.value("study/kanahint", 2).toInt(&ok);
        if (ok && val >= 0 && val <= 3 && val != 1)
            study.kanahint = (WordParts)val;
        val = ini.value("study/defhint", 0).toInt(&ok);
        if (ok && val >= 0 && val <= 3 && val != 2)
            study.defhint = (WordParts)val;

        study.showestimate = ini.value("study/showestimate", true).toBool();
        study.writekanji = ini.value("study/writekanji", false).toBool();
        study.writekana = ini.value("study/writekana", false).toBool();
        study.writedef = ini.value("study/writedef", false).toBool();
        val = ini.value("study/delaywrong", 10).toInt(&ok);
        if (ok && val >= 1 && val <= 60)
            study.delaywrong = val;
        study.review = ini.value("study/review", true).toBool();
        study.idlesuspend = ini.value("study/idlesuspend", false).toBool();
        val = ini.value("study/idletime", 3).toInt(&ok);
        if (ok && val >= 0 && val <= 4)
            study.idletime = (StudySettings::SuspendWait)val;
        study.itemsuspend = ini.value("study/itemsuspend", false).toBool();
        val = ini.value("study/itemsuspendnum", 60).toInt(&ok);
        if (ok && val >= 1 && val <= 9999)
            study.itemsuspendnum = val;
        study.timesuspend = ini.value("study/timesuspend", false).toBool();
        val = ini.value("study/timesuspendnum", 20).toInt(&ok);
        if (ok && val >= 1 && val <= 9999)
            study.timesuspendnum = val;

        val = ini.value("study/readings", 2).toInt(&ok);
        if (ok && val >= 0 && val <= 3)
            study.readings = (StudySettings::ReadingType)val;
        ini.value("study/readingsfrom", 0).toInt(&ok);
        if (ok && val >= 0 && val <= 3)
            study.readingsfrom = (StudySettings::ReadingFrom)val;

        // Data settings

        data.autosave = ini.value("data/autosave", false).toBool();
        val = ini.value("data/interval", 2).toInt(&ok);
        if (ok && val >= 1 && val <= 60)
            data.interval = val;
        data.backup = ini.value("data/backup", false).toBool();
        val = ini.value("data/backupcount", 4).toInt(&ok);
        if (ok && val >= 1 && val <= 9999)
            data.backupcnt = val;
        val = ini.value("data/backupskip",3).toInt(&ok);
        if (ok && val >= 1 && val <= 100)
            data.backupskip = val;
        data.location = ini.value("data/location", QString()).toString();

        // Dictionary order

        QSet<quint8> found;
        std::list<quint8> order;
        QStringList strs = ini.value("dictionaryorder", QString()).toString().split('/');
        for (int ix = strs.size() - 1; ix != -1; --ix)
        {
            int dix = ZKanji::dictionaryIndex(strs[ix]);
            if (dix == -1)
            {
                strs.removeAt(ix);
                continue;
            }
            order.push_front(dix);
            found.insert(dix);
        }
        for (int ix = 0, siz = ZKanji::dictionaryCount(); ix != siz; ++ix)
        {
            if (found.contains(ix))
                continue;
            order.push_back(ix);
        }

        ZKanji::changeDictionaryOrder(order);
    }

    void loadSettingsFromFile()
    {
        loadIniFile();
        
        QFontDatabase db;
        QStringList allJpFonts = db.families(QFontDatabase::Japanese);
        hasSimSun = db.families(QFontDatabase::Any).indexOf("SimSun") != -1;

        if (!fonts.kanji.isEmpty() && allJpFonts.indexOf(fonts.kanji) == -1)
            fonts.kanji.clear();
        if (!fonts.kana.isEmpty() && allJpFonts.indexOf(fonts.kana) == -1)
            fonts.kana.clear();
        if (!fonts.printkana.isEmpty() && allJpFonts.indexOf(fonts.printkana) == -1)
            fonts.printkana.clear();

        // A default Japanese font that can be used when nothing is set.
        QString defjp;

        if (fonts.kanji.isEmpty() || fonts.kana.isEmpty() || fonts.printkana.isEmpty())
            defjp = defaultJapaneseFont(allJpFonts);
        if (fonts.kanji.isEmpty())
            fonts.kanji = defjp;
        if (fonts.kana.isEmpty())
            fonts.kana = defjp;
        if (fonts.printkana.isEmpty())
            fonts.printkana = defjp;

        QString fname = ZKanji::userFolder() + "/states.xml";
        QFile f(fname);
        if (!f.open(QIODevice::ReadOnly))
        {
            if (!f.exists())
                return;
            QMessageBox::warning(nullptr, "zkanji", gUI->tr("Couldn't open the state settings file for reading. Please make sure you have permission to access the file %1.").arg(QDir::toNativeSeparators(fname)), QMessageBox::Ok);
            return;
        }

        QXmlStreamReader reader(&f);
        reader.setNamespaceProcessing(false);

        reader.readNextStartElement();
        if (reader.name() != "ZKanjiSettings")
            reader.raiseError("MissingZKanjiSettingsElement");


        // When expanding later: Every loadXML****() function MUST leave the current block,
        // and any level before that should expect this to happen, either by calling the
        // stream reader's skipCurrentElement(), or by a while(reader.readNextStartElement())
        // loop.
        // The reader's readNextStartElement() function reads until the next starting OR next
        // ending element, and returns true if it currently is at a starting element. When it
        // returns false, the attributes will be valid for the ending, and the next read will
        // start the next block outside the one that just ended.

        while (!reader.atEnd())
        {
            if (!reader.readNextStartElement() || reader.atEnd())
                break;

            if (reader.name() == "Filters")
            {
                while (reader.readNextStartElement())
                {
                    if (reader.name() == "RadicalFilters")
                        loadXMLRadicalFilters(reader);
                    else if (reader.name() == "WordFilters")
                        loadXMLWordFilters(reader);
                }
            }
            else if (reader.name() == "LookupSites")
            {
                ZKanji::loadXMLLookupSites(reader);
            }
            else if (reader.name() == "WindowStates")
            {
                if (!Settings::general.savewinstates)
                    reader.skipCurrentElement();
                else while (reader.readNextStartElement())
                {
                    if (reader.name() == "WordEditor" || reader.name() == "WordToGroup" || reader.name() == "WordToDictionary")
                        FormStates::loadXMLDialogSplitterState(reader);
                    else if (reader.name() == "CollectWords")
                        FormStates::loadXMLSettings(FormStates::collectform, reader);
                    else if (reader.name() == "KanjiInformation")
                        FormStates::loadXMLSettings(FormStates::kanjiinfo, reader);
                    else if (reader.name() == "WordStudyListForm")
                        FormStates::loadXMLSettings(FormStates::wordstudylist, reader);
                    else if (reader.name() == "PopupKanji")
                        FormStates::loadXMLSettings(FormStates::popupkanji, reader);
                    else if (reader.name() == "PopupDictionary")
                        FormStates::loadXMLSettings(FormStates::popupdict, reader);
                    else
                        reader.skipCurrentElement();
                }
            }
            else if (reader.name() == "Windows")
            {
                if (!Settings::general.savewinstates)
                    reader.skipCurrentElement();
                else while (reader.readNextStartElement())
                {
                    bool ismain = reader.name() == "MainWindow";
                    if (ismain && gUI->mainForm() != nullptr)
                    {
                        // Multiple MainWindow tags found in the xml.
                        reader.skipCurrentElement();
                        continue;
                    }
                    if (ismain || reader.name() == "Window")
                        gUI->loadXMLWindow(ismain, reader);
                    else if (Settings::general.savetoolstates && reader.name() == "KanjiInfoWindows")
                        gUI->loadXMLKanjiInfo(reader);
                    else
                        reader.skipCurrentElement();
                }
            }
            else
                reader.skipCurrentElement();
        }

        if (reader.hasError())
        {
            QString errorstr;
            switch (reader.error())
            {
            case QXmlStreamReader::UnexpectedElementError:
                errorstr = gUI->tr("UnexpectedElementError");
                break;
            case QXmlStreamReader::NotWellFormedError:
                errorstr = gUI->tr("NotWellFormedError");
                break;
            case QXmlStreamReader::PrematureEndOfDocumentError:
                errorstr = gUI->tr("PrematureEndOfDocumentError");
                break;
            case QXmlStreamReader::CustomError:
                errorstr = reader.errorString();
                break;
            }

            if (QMessageBox::warning(nullptr, "zkanji", gUI->tr("There was an error in states.xml at line %1, position %2, of the type %3. The program can load normally with some settings lost.").arg(reader.lineNumber()).arg(reader.columnNumber()).arg(errorstr), QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Cancel)
                exit(0);
        }
    }


    // TODO: update these to reflect settings, and to work on other OS-s. Or use SOD.
    QString radicalFontName()
    {
#ifdef Q_OS_WIN
        if (hasSimSun)
            return QStringLiteral("SimSun");
#endif
        return Settings::fonts.kanji;
    }

    QFont kanjiFont()
    {
        QFont f = { fonts.kanji, fonts.kanjifontsize };
        if (Settings::fonts.nokanjialias)
        {
            QFont::StyleStrategy ss = f.styleStrategy();
            ss = QFont::StyleStrategy(ss | QFont::NoSubpixelAntialias);
            f.setStyleStrategy(ss);
        }
        return f;
    }

    QFont radicalFont()
    {
        return QFont{ radicalFontName(), fonts.kanjifontsize };
    }

    QFont kanaFont()
    {
        return QFont{ Settings::fonts.kana, 9 };
    }

    QFont defFont()
    {
        return QFont{ Settings::fonts.definition, 9 };
    }

    QFont notesFont()
    {
        return QFont{ Settings::fonts.info, 7, Settings::fonts.infostyle == FontSettings::Bold || Settings::fonts.infostyle == FontSettings::BoldItalic ? QFont::Bold : -1, Settings::fonts.infostyle == FontSettings::Italic || Settings::fonts.infostyle == FontSettings::BoldItalic };
    }

    QFont extraFont()
    {
        return QFont{ Settings::fonts.extra, 9, Settings::fonts.extrastyle == FontSettings::Bold || Settings::fonts.extrastyle == FontSettings::BoldItalic ? QFont::Bold : -1, Settings::fonts.extrastyle == FontSettings::Italic || Settings::fonts.extrastyle == FontSettings::BoldItalic };
    }

    QFont printKanaFont()
    {
        return QFont{ !Settings::print.dictfonts ? Settings::fonts.printkana : Settings::fonts.kana, 120 };
    }

    QFont printDefFont()
    {
        return QFont{ !Settings::print.dictfonts ? Settings::fonts.printdefinition : Settings::fonts.definition, 120 };
    }

    QFont printInfoFont()
    {
        return QFont{ !Settings::print.dictfonts ? Settings::fonts.printinfo : Settings::fonts.info, 120, Settings::print.dictfonts ? false : Settings::fonts.printinfostyle == FontSettings::Bold || Settings::fonts.printinfostyle == FontSettings::BoldItalic ? QFont::Bold : -1, Settings::print.dictfonts ? true : Settings::fonts.printinfostyle == FontSettings::Italic || Settings::fonts.printinfostyle == FontSettings::BoldItalic };
    }

    QColor color(const QPalette &pal, QPalette::ColorGroup group, ColorSettings::ColorTypes type)
    {
        switch (type)
        {
        case ColorSettings::Bg:
            if ((group == QPalette::Active && colors.bg.isValid()) || (group == QPalette::Inactive && colors.bgi.isValid()))
                return group == QPalette::Active ? colors.bg : colors.bgi;
            return pal.color(group, QPalette::Base);
        case ColorSettings::Text:
            if ((group == QPalette::Active && colors.text.isValid()) || (group == QPalette::Inactive && colors.texti.isValid()))
                return group == QPalette::Active ? colors.text : colors.texti;
            return pal.color(group, QPalette::Text);
        case ColorSettings::SelBg:
            if ((group == QPalette::Active && colors.selbg.isValid()) || (group == QPalette::Inactive && colors.selbgi.isValid()))
                return group == QPalette::Active ? colors.selbg : colors.selbgi;
            return pal.color(group, QPalette::Highlight);
        case ColorSettings::SelText:
            if ((group == QPalette::Active && colors.seltext.isValid()) || (group == QPalette::Inactive && colors.seltexti.isValid()))
                return group == QPalette::Active ? colors.seltext : colors.seltexti;
            return pal.color(group, QPalette::HighlightedText);
        default:
            return QColor();
        }
    }

    void updatePalette(QWidget *w)
    {
        QPalette newpal = w->palette();
        newpal.setColor(QPalette::Active, QPalette::Base, color(qApp->palette(), QPalette::Active, ColorSettings::Bg));
        newpal.setColor(QPalette::Active, QPalette::Text, color(qApp->palette(), QPalette::Active, ColorSettings::Text));
        newpal.setColor(QPalette::Active, QPalette::Highlight, color(qApp->palette(), QPalette::Active, ColorSettings::SelBg));
        newpal.setColor(QPalette::Active, QPalette::HighlightedText, color(qApp->palette(), QPalette::Active, ColorSettings::SelText));
        newpal.setColor(QPalette::Inactive, QPalette::Base, color(qApp->palette(), QPalette::Inactive, ColorSettings::Bg));
        newpal.setColor(QPalette::Inactive, QPalette::Text, color(qApp->palette(), QPalette::Inactive, ColorSettings::Text));
        newpal.setColor(QPalette::Inactive, QPalette::Highlight, color(qApp->palette(), QPalette::Inactive, ColorSettings::SelBg));
        newpal.setColor(QPalette::Inactive, QPalette::HighlightedText, color(qApp->palette(), QPalette::Inactive, ColorSettings::SelText));
        w->setPalette(newpal);
    }
}




