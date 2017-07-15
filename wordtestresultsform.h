#ifndef WORDTESTRESULTSFORM_H
#define WORDTESTRESULTSFORM_H

#include <QMainWindow>
#include "zdictionarymodel.h"

namespace Ui {
    class WordTestResultsForm;
}

enum class WordStudyColumnTypes { Score = (int)DictColumnTypes::Last + 1 };

class WordStudy;
class QImage;
class QAbstractButton;
class WordStudyItemModel : public DictionaryItemModel
{
    Q_OBJECT
public:
    WordStudyItemModel(WordStudy *study, QObject *parent = nullptr);
    virtual ~WordStudyItemModel();

    // Returns the index of a word in its dictionary at the passed position.
    virtual int indexes(int pos) const override;
    // Returns the dictionary the model shows items for.
    virtual Dictionary* dictionary() const override;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    //virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
protected slots:
    //virtual void entryAboutToBeRemoved(int windex) override;
    virtual void entryRemoved(int windex, int abcdeindex, int aiueoindex) override;
    virtual void entryChanged(int windex, bool studydef) override;
    virtual void entryAdded(int windex) override;
private:
    WordStudy *study;

    //static QImage* check;

    typedef DictionaryItemModel base;
};

class QCloseEvent;
class WordTestResultsForm : public QMainWindow
{
    Q_OBJECT
public:
    WordTestResultsForm(QWidget *parent = nullptr);
    virtual ~WordTestResultsForm();

    void exec(WordStudy *study);
protected:
    virtual void closeEvent(QCloseEvent *e) override;
protected slots:
    void on_buttonBox_clicked(QAbstractButton *button);
private:
    Ui::WordTestResultsForm *ui;

    WordStudy *study;

    bool nextround;

    typedef QMainWindow base;
};

#endif // WORDTESTRESULTSFORM_H