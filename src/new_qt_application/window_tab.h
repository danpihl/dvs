#ifndef NEW_QT_APPLICATION_WINDOW_TAB_H_
#define NEW_QT_APPLICATION_WINDOW_TAB_H_

#include <QWidget>

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "color.h"
#include "editing_silhouette.h"
#include "gui_callbacks.h"
#include "gui_element.h"
#include "plot_pane.h"
#include "project_state/project_settings.h"

// Adapted from main_application/gui_tab.{h,cpp}'s WindowTab/ZOrderQueue.
// The wx original threads an `element_x_offset` through every element (a
// reserved-margin width for the tab-select strip) — dropped here since the
// tab strip now gets real screen space via a QHBoxLayout in GuiWindow, so
// elements are simply a fraction of their parent's actual size (see
// gui_element.h's class comment).
class ZOrderQueue
{
private:
    std::vector<std::string> elements_;

    bool elementExistsInQueue(const std::string& element_handle_string) const;

public:
    ZOrderQueue() = default;

    int getOrderOfElement(const std::string& element_handle_string) const;
    void raise(const std::string& element_handle_string);
    void lower(const std::string& element_handle_string);
    void eraseElement(const std::string& element_handle_string);
};

// wx computes an element's placeholder construction geometry with a plain
// fraction * window-size formula (no minimum_x_pos_/ratio adjustment — that
// correction is applied immediately after, via setMinXPos+updateSizeFromParent).
// See main_application/gui_tab.cpp's free function of the same name.
std::pair<QPoint, QSize> getPosAndSizeInPixelCoords(const QSize& current_window_size,
                                                    const ElementSettings* const element_settings);

class WindowTab
{
private:
    std::string name_;
    std::vector<PlotPane*> plot_panes_;
    std::vector<GuiElement*> gui_elements_;
    QWidget* parent_window_;
    // Copied from the constructor's `callbacks` param, then `tab_about_editing`
    // is overwritten with a lambda tied to this tab's own editing_silhouette_
    // before being passed down to each GuiElement/PlotPane this tab creates —
    // see gui_callbacks.h and ARCHITECTURE_IMPROVEMENTS.md #2.
    GuiCallbacks callbacks_;
    int current_element_idx_;
    RGBTripletf background_color_;
    RGBTripletf button_normal_color_;
    RGBTripletf button_clicked_color_;
    RGBTripletf button_selected_color_;
    RGBTripletf button_text_color_;
    ZOrderQueue z_order_queue_;

    EditingSilhouette* editing_silhouette_;

public:
    WindowTab(QWidget* parent_window, const TabSettings& tab_settings, const GuiCallbacks& callbacks);
    void initializeZOrder(const TabSettings& tab_settings);
    ~WindowTab();
    std::vector<GuiElement*> getPlotPanes() const;
    std::vector<GuiElement*> getGuiElements() const;
    std::vector<GuiElement*> getAllGuiElements() const;
    void updateAllElements();
    void createNewPlotPane();
    void createNewPlotPane(const std::string& element_handle_string);
    void createNewPlotPane(const std::shared_ptr<ElementSettings>& element_settings);

    // Deferred: no GuiElement subclass exists yet for these types (see
    // CURRENT_STATE.md). Each logs a warning and does nothing rather than
    // silently dropping the element or crashing.
    void createNewButton(const std::shared_ptr<ElementSettings>& elem_settings);
    void createNewSlider(const std::shared_ptr<ElementSettings>& elem_settings);
    void createNewCheckbox(const std::shared_ptr<ElementSettings>& elem_settings);
    void createNewTextLabel(const std::shared_ptr<ElementSettings>& elem_settings);
    void createNewListBox(const std::shared_ptr<ElementSettings>& element_settings);
    void createNewEditableText(const std::shared_ptr<ElementSettings>& element_settings);
    void createDropdownMenu(const std::shared_ptr<ElementSettings>& element_settings);
    void createRadioButtonGroup(const std::shared_ptr<ElementSettings>& element_settings);
    void createScrollingText(const std::shared_ptr<ElementSettings>& element_settings);

    void show();
    void hide();
    void updateSizeFromParent(const QSize new_size) const;
    RGBTripletf getBackgroundColor() const;
    std::string getName() const;
    TabSettings getTabSettings() const;
    GuiElement* getGuiElement(const std::string& element_handle_string) const;
    void notifyChildrenOnKeyPressed(const char key);
    void notifyChildrenOnKeyReleased(const char key);
    void setMouseInteractionType(const MouseInteractionType mit);
    bool deleteElementIfItExists(const std::string& element_handle_string);
    void toggleProjectionMode(const std::string& element_handle_string);
    bool elementWithNameExists(const std::string& element_handle_string);
    bool changeNameOfElementIfElementExists(const std::string& element_handle_string,
                                            const std::map<std::string, std::string>& new_values);
    bool raiseElement(const std::string& element_handle_string);
    bool lowerElement(const std::string& element_handle_string);
    std::vector<std::string> getElementNames() const;
    void setName(const std::string& new_name);
};

#endif  // NEW_QT_APPLICATION_WINDOW_TAB_H_
