#ifndef NEW_QT_APPLICATION_MESSAGE_ROUTER_H_
#define NEW_QT_APPLICATION_MESSAGE_ROUTER_H_

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "communication/data_receiver.h"
#include "communication/received_data.h"
#include "gui_element_state.h"
#include "input_data.h"
#include "plot_pane.h"

// Extracted from MainWindow (main_application/main_window_receive.cpp's
// original responsibility) — see ARCHITECTURE_IMPROVEMENTS.md #4. Owns the
// TCP receive thread and all wire-protocol dispatch/parsing, completely
// independent of window/element lifecycle and project management, which
// remain in MainWindow. This class has no knowledge of GuiWindow,
// MainWindow's WindowEntry, SaveManager, or any Qt widget beyond PlotPane
// (needed to call pushQueue() directly on the drained data) — everything
// else it needs from MainWindow's window/element bookkeeping comes through
// MessageRouterCallbacks, each of which is responsible for its own locking
// against whatever guards the underlying data on MainWindow's side (this
// class and MainWindow run on different threads for the TCP-thread-invoked
// callbacks: find_plot_pane, set_element_label, get_all_gui_element_states.
// perform_screenshot, open_project_file, and create_new_window_for_element
// are GUI-thread-only — invoked from poll(), never from the TCP thread).
struct MessageRouterCallbacks
{
    std::function<PlotPane*(const std::string&)> find_plot_pane;
    std::function<void(const std::string&, const std::string&)> set_element_label;
    std::function<std::vector<std::shared_ptr<GuiElementState>>()> get_all_gui_element_states;

    // Triggers back into MainWindow's window-lifecycle/project methods.
    // Invoked from poll() (GUI thread), never from the TCP thread —
    // perform_screenshot included: it does real Qt/AppKit widget work
    // (GuiWindow::screenshot() -> QWidget::raise()), which crashes if
    // called off the GUI thread (confirmed via a real, reproducible
    // EXC_BAD_INSTRUCTION when this was still invoked directly from
    // manageReceivedData on the TCP thread — see
    // ARCHITECTURE_IMPROVEMENTS.md #3's screenshot-crash note). SCREENSHOT
    // messages are now queued the same way OPEN_PROJECT_FILE/new-window
    // requests already were, and drained here.
    std::function<void(const std::string&)> perform_screenshot;
    std::function<void(const std::string&)> open_project_file;
    std::function<void(const std::string&)> create_new_window_for_element;
};

class MessageRouter
{
public:
    MessageRouter() = delete;
    explicit MessageRouter(const MessageRouterCallbacks& callbacks);

    void start();

    // Called periodically (from MainWindow's receive_timer_, on the GUI
    // thread): drains queued plot data into find_plot_pane()/pushQueue(),
    // and acts on any project-file-open/new-window request queued by the
    // TCP thread since the last call. Replaces the relevant half of the
    // old MainWindow::receiveData() (handleSerialData(), unrelated to
    // message routing, stays in MainWindow).
    void poll();

private:
    void tcpReceiveThreadFunction();
    void manageReceivedData(ReceivedData& received_data);
    void addActionToQueue(ReceivedData& received_data);
    void handleGuiManipulation(ReceivedData& received_data);
    void mainWindowFlushMultipleElements(const ReceivedData& received_data);
    void setActiveView(const ReceivedData& received_data);
    void sendGuiStateToClient() const;
    std::string getCurrentElementName();

    MessageRouterCallbacks callbacks_;

    DataReceiver data_receiver_;
    // Never joined/deleted, matching the pre-existing MainWindow behavior
    // this was extracted from — there's no clean shutdown path for the
    // receive loop today (see tcpReceiveThreadFunction's while(true)); this
    // is a pre-existing gap, not something introduced by the extraction.
    std::thread* tcp_receive_thread_;

    // Protects open_project_file_queued_/queued_project_file_name_/
    // pending_new_window_names_/current_element_name_ — all written from the
    // TCP thread's dispatch, read from poll() on the GUI thread. Deliberately
    // separate from MainWindow's own receive_mtx_ (which now protects only
    // plot_panes_/gui_elements_) and from queued_data_mtx_ below — each
    // mutex guards exactly one piece of state, per
    // ARCHITECTURE_IMPROVEMENTS.md #3's reasoning.
    std::mutex flags_mtx_;
    std::string current_element_name_;
    std::atomic<bool> open_project_file_queued_;
    std::string queued_project_file_name_;
    std::atomic<bool> screenshot_queued_;
    std::string queued_screenshot_path_;

    // Names passed to SET_CURRENT_ELEMENT that had no existing PlotPane at
    // the time (see setActiveView), awaiting an auto-created window. A
    // vector, not a single flag+string: with only one such slot, a client
    // setting two or more never-before-seen elements as current within the
    // same ~10ms poll() interval (e.g. rapidly creating several new plot
    // windows in a tight loop) would silently lose all but the last one —
    // the earlier names' queued plot data would sit in queued_data_ with no
    // matching PlotPane by the time poll() drained it, and be discarded with
    // no trace. Deduplicated on insert in setActiveView, and re-checked
    // defensively in poll() right before creating each window.
    std::vector<std::string> pending_new_window_names_;

    std::mutex queued_data_mtx_;
    std::map<std::string, std::queue<std::unique_ptr<InputData>>> queued_data_;
};

#endif  // NEW_QT_APPLICATION_MESSAGE_ROUTER_H_
