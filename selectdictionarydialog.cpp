/*
** Copyright 2007-2013, 2017 S�lyom Zolt�n
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPushButton>
#include "selectdictionarydialog.h"
#include "ui_selectdictionarydialog.h"
#include "zdictionariesmodel.h"
#include "globalui.h"
#include "words.h"


//-------------------------------------------------------------


SelectDictionaryDialog::SelectDictionaryDialog(QWidget *parent) : base(parent), ui(new Ui::SelectDictionaryDialog)
{
    ui->setupUi(this);
}

SelectDictionaryDialog::~SelectDictionaryDialog()
{
    delete ui;
}

int SelectDictionaryDialog::exec(QString labelstring, int firstselected, QString okstr)
{
    if (!okstr.isEmpty())
        ui->buttonBox->button(QDialogButtonBox::Ok)->setText(okstr);
    ui->label->setText(labelstring);
    if (firstselected == -1)
        firstselected = 0;
    else
        firstselected = ZKanji::dictionaryOrder(firstselected);
    ui->dictCBox->setModel(ZKanji::dictionariesModel());
    ui->dictCBox->setCurrentIndex(firstselected);

    setAttribute(Qt::WA_DontShowOnScreen);
    show();
    adjustSize();
    setFixedSize(size());
    hide();
    setAttribute(Qt::WA_DontShowOnScreen, false);

    showModal();

    if (modalResult() == ModalResult::Cancel)
        return -1;
    return ZKanji::dictionaryPosition(ui->dictCBox->currentIndex());
}

void SelectDictionaryDialog::on_buttonBox_accepted()
{
    modalClose(ModalResult::Ok);
}

void SelectDictionaryDialog::on_buttonBox_rejected()
{
    modalClose(ModalResult::Cancel);
}


//-------------------------------------------------------------


int selectDictionaryDialog(QString str, int firstselected, QString okstr)
{
    SelectDictionaryDialog form(gUI->activeMainForm());

    return form.exec(str, firstselected, okstr);
}


//-------------------------------------------------------------

