/*
** Copyright 2007-2013, 2017 S�lyom Zolt�n
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef DIALOGSETTINGS_H
#define DIALOGSETTINGS_H

class GroupBase;

// Saved state or settings for dialog windows. For example default selections.
struct DialogSettings
{
    QString defwordgroup;
    QString defkanjigroup;

    // Name of the last page that was open in the settings window.
    QString lastsettingspage;
};

namespace Settings
{
    extern DialogSettings dialog;
}

#endif // DIALOGSETTINGS_H

