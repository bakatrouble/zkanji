#ifndef RADFORM_H
#define RADFORM_H

#include <QMainWindow>
#include <memory>

namespace Ui {
    class RadicalForm;
}

enum class RadicalFilterModes : int;
class ZVariedModel;
struct RadicalFilter;
class QAbstractButton;
class QListWidgetItem;
class KanjiGridModel;
class RadicalForm : public QMainWindow
{
    Q_OBJECT
public:
    // 
    enum Results { Ok, Cancel };

    RadicalForm(QWidget *parent = nullptr);
    virtual ~RadicalForm();

    void setDisplayMode(RadicalFilterModes mode);

    // Sets the index of the filter in the radicals filter model to be
    // shown in the form.
    void setFilter(int index);

    // Calls the function with the same name in the radicals grid.
    // Not valid when handling the destroyed event.
    void setFromModel(KanjiGridModel *model);
    // Calls the function with the same name in the radicals grid.
    // Not valid when handling the destroyed event.
    void setFromList(const std::vector<ushort> &list);
    // Calls the function with the same name in the radicals grid.
    // When handling the destroyed event, only valid if the result is Ok.
    void activeFilter(RadicalFilter &f);
    // Calls the function with the same name in the radicals grid.
    // Not valid when handling the destroyed event.
    std::vector<ushort> selectionToFilter();

    // Which button was used to close the dialog. Only returns Ok when
    // the Ok button was pressed.
    Results result();
signals:
    // Notifies the kanji window that the selected radicals have changed, and
    // it should update its kanji display.
    void selectionChanged();
    // Notifies the kanji window that the ok button has been pressed, before the window
    // closes.
    void resultIsOk();
    // Notifies the kanji window that the meaning of a single radical selection changed. When
    // grouping is true, each selection also automatically means the other forms of the same
    // radical should be considered as selected.
    void groupingChanged(bool grouping);
public slots:
    // Called when a radical has been added to the global radicals filter model.
    // Updates the listed radicals.
    void radsAdded();
    // Called when a radical has been removed from the global radicals filter model.
    // Updates the listed radicals.
    void radsRemoved(int index);
    // Called when a radical has been moved in the global radicals filter model.
    // Updates the listed radicals.
    void radsMoved(int index, bool up);
    // Called when the global radicals filter model has been cleared.
    // Updates the listed radicals.
    void radsCleared();

    void radsMoveUp();
    void radsMoveDown();
    void radsRemove();
    void radsClear();
    void radsRowChanged(int row);
    void radsDoubleClicked(QListWidgetItem *item);

    void on_addButton_clicked();
protected:
    virtual void showEvent(QShowEvent *event) override;
private:
    // Switches between the parts and radicals display.
    void updateMode();
    // Sets the radical grid's filter settings depending on
    // the state of the widgets.
    void updateSettings();

   
    // Clears the current radical selection and the filter edit boxes.
    void clearSelection();

    // Shows/hides the previous selections list and updates the button above it.
    void showHidePrevSel();

    // Sets the label text for the selectionLabel to str.
    void updateSelectionText(const QString &str);

    void handleButtons(QAbstractButton *button);

    Ui::RadicalForm *ui;

    // The first time the window is shown, it should be resized
    // to hide the previous radical selections list. This value is
    // set to true after it happened.
    bool sizeinited;

    ZVariedModel *model;

    Results btnresult;
    std::unique_ptr<RadicalFilter> finalfilter;

    typedef QMainWindow base;
};


#endif // RADFORM_H