#include "message_router.h"

#include <algorithm>

#include "lumos/logging.h"
#include "lumos/plotting/enumerations.h"
#include "lumos/plotting/internal.h"
#include "plot_objects/draw_mesh/draw_mesh.h"
#include "plot_objects/fast_plot2d/fast_plot2d.h"
#include "plot_objects/fast_plot3d/fast_plot3d.h"
#include "plot_objects/im_show/im_show.h"
#include "plot_objects/line_collection2/line_collection2.h"
#include "plot_objects/line_collection3/line_collection3.h"
#include "plot_objects/plot2d/plot2d.h"
#include "plot_objects/plot3d/plot3d.h"
#include "plot_objects/plot_collection2/plot_collection2.h"
#include "plot_objects/plot_collection3/plot_collection3.h"
#include "plot_objects/scatter/scatter.h"
#include "plot_objects/scatter3/scatter3.h"
#include "plot_objects/screen_space_primitive/screen_space_primitive.h"
#include "plot_objects/scrolling_plot2d/scrolling_plot2d.h"
#include "plot_objects/stairs/stairs.h"
#include "plot_objects/stem/stem.h"
#include "plot_objects/surf/surf.h"

using namespace lumos::internal;

namespace
{
// Moved verbatim from main_window.cpp — faithful port of the free function
// of the same name in main_application/main_window_receive.cpp:54-156.
std::shared_ptr<const ConvertedDataBase> convertPlotObjectData(const CommunicationHeader& hdr,
                                                                const ReceivedData& received_data,
                                                                const PlotObjectAttributes& attributes,
                                                                const UserSuppliedProperties& user_supplied_properties)
{
    const Function fcn = received_data.getFunction();

    switch (fcn)
    {
        case Function::STAIRS:
            return Stairs::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::PLOT2:
            return Plot2D::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::PLOT3:
            return Plot3D::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::SCREEN_SPACE_PRIMITIVE:
            return ScreenSpacePrimitive::convertRawData(hdr, attributes, user_supplied_properties,
                                                        received_data.payloadData());
        case Function::FAST_PLOT2:
            return FastPlot2D::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::FAST_PLOT3:
            return FastPlot3D::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::LINE_COLLECTION2:
            return LineCollection2D::convertRawData(hdr, attributes, user_supplied_properties,
                                                    received_data.payloadData());
        case Function::LINE_COLLECTION3:
            return LineCollection3D::convertRawData(hdr, attributes, user_supplied_properties,
                                                    received_data.payloadData());
        case Function::STEM:
            return Stem::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::SCATTER2:
            return Scatter2D::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::SCATTER3:
            return Scatter3D::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::SURF:
            return Surf::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::IM_SHOW:
            return ImShow::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::PLOT_COLLECTION2:
            return PlotCollection2D::convertRawData(hdr, attributes, user_supplied_properties,
                                                    received_data.payloadData());
        case Function::PLOT_COLLECTION3:
            return PlotCollection3D::convertRawData(hdr, attributes, user_supplied_properties,
                                                    received_data.payloadData());
        case Function::DRAW_MESH_SEPARATE_VECTORS:
        case Function::DRAW_MESH:
            return DrawMesh::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::REAL_TIME_PLOT:
            return ScrollingPlot2D::convertRawData(hdr, attributes, user_supplied_properties,
                                                   received_data.payloadData());
        default:
            throw std::runtime_error("Invalid function!");
    }
}

bool isGuiRelatedFunction(const Function fcn)
{
    return fcn == Function::SET_GUI_ELEMENT_LABEL;
}
}  // namespace

MessageRouter::MessageRouter(const MessageRouterCallbacks& callbacks)
    : callbacks_{callbacks},
      data_receiver_{},
      tcp_receive_thread_{nullptr},
      open_project_file_queued_{false},
      screenshot_queued_{false}
{
}

void MessageRouter::start()
{
    tcp_receive_thread_ = new std::thread(&MessageRouter::tcpReceiveThreadFunction, this);
}

void MessageRouter::tcpReceiveThreadFunction()
{
    while (true)
    {
        ReceivedData received_data = data_receiver_.receiveAndGetDataFromTcp();
        if (received_data.rawData() == nullptr)
        {
            continue;
        }

        manageReceivedData(received_data);
    }
}

std::string MessageRouter::getCurrentElementName()
{
    const std::lock_guard<std::mutex> lg(flags_mtx_);
    return current_element_name_;
}

void MessageRouter::setActiveView(const ReceivedData& received_data)
{
    const CommunicationHeader& hdr = received_data.getCommunicationHeader();
    const std::string name = hdr.get(CommunicationHeaderObjectType::ELEMENT_NAME).as<properties::Label>().data;

    if (name.empty())
    {
        LUMOS_LOG_WARNING() << "Label string had zero length!";
        return;
    }

    const std::lock_guard<std::mutex> lg(flags_mtx_);
    current_element_name_ = name;

    if (callbacks_.find_plot_pane(current_element_name_) == nullptr)
    {
        if (std::find(pending_new_window_names_.begin(), pending_new_window_names_.end(),
                       current_element_name_) == pending_new_window_names_.end())
        {
            pending_new_window_names_.push_back(current_element_name_);
        }
    }
}

void MessageRouter::addActionToQueue(ReceivedData& received_data)
{
    const Function fcn = received_data.getFunction();

    if (fcn == Function::SET_CURRENT_ELEMENT)
    {
        setActiveView(received_data);
    }
    else if (fcn == Function::FLUSH_MULTIPLE_ELEMENTS)
    {
        mainWindowFlushMultipleElements(received_data);
    }
    else if (isPlotDataFunction(fcn))
    {
        const CommunicationHeader& hdr = received_data.getCommunicationHeader();
        const PlotObjectAttributes plot_object_attributes{hdr};
        const UserSuppliedProperties user_supplied_properties{hdr};

        const std::shared_ptr<const ConvertedDataBase> converted_data =
            convertPlotObjectData(hdr, received_data, plot_object_attributes, user_supplied_properties);
        const std::string element_name = getCurrentElementName();

        const std::lock_guard<std::mutex> lg(queued_data_mtx_);
        queued_data_[element_name].push(std::make_unique<InputData>(
            received_data, converted_data, plot_object_attributes, user_supplied_properties));
    }
    else if (fcn == Function::PROPERTIES_EXTENSION || fcn == Function::PROPERTIES_EXTENSION_MULTIPLE)
    {
        const CommunicationHeader& hdr = received_data.getCommunicationHeader();
        const PlotObjectAttributes plot_object_attributes{hdr};
        const UserSuppliedProperties user_supplied_properties{hdr};
        const std::string element_name = getCurrentElementName();

        const std::lock_guard<std::mutex> lg(queued_data_mtx_);
        queued_data_[element_name].push(
            std::make_unique<InputData>(received_data, plot_object_attributes, user_supplied_properties));
    }
    else
    {
        const std::string element_name = getCurrentElementName();
        const std::lock_guard<std::mutex> lg(queued_data_mtx_);
        queued_data_[element_name].push(std::make_unique<InputData>(received_data));
    }
}

void MessageRouter::handleGuiManipulation(ReceivedData& received_data)
{
    const CommunicationHeader& hdr = received_data.getCommunicationHeader();
    const std::string handle_string = hdr.get(CommunicationHeaderObjectType::HANDLE_STRING).as<properties::Label>().data;
    const std::string label = hdr.get(CommunicationHeaderObjectType::LABEL).as<properties::Label>().data;

    callbacks_.set_element_label(handle_string, label);
}

void MessageRouter::sendGuiStateToClient() const
{
    const std::vector<std::shared_ptr<GuiElementState>> gui_elements_state = callbacks_.get_all_gui_element_states();

    std::uint64_t total_num_bytes = 0U;
    for (const auto& ges : gui_elements_state)
    {
        total_num_bytes += ges->getTotalNumBytes();
    }
    total_num_bytes += sizeof(std::uint8_t);  // Number of gui elements

    FillableUInt8Array output_array{total_num_bytes};
    output_array.fillWithStaticType(static_cast<std::uint8_t>(gui_elements_state.size()));

    for (const auto& ges : gui_elements_state)
    {
        ges->serializeToBuffer(output_array);
    }

    sendThroughTcpInterface(output_array.view(), kGuiTcpPortNum);
}

void MessageRouter::manageReceivedData(ReceivedData& received_data)
{
    const Function fcn = received_data.getFunction();

    if (fcn == Function::OPEN_PROJECT_FILE)
    {
        const CommunicationHeader& hdr = received_data.getCommunicationHeader();
        const std::string path =
            hdr.get(CommunicationHeaderObjectType::PROJECT_FILE_NAME).as<properties::Label>().data;

        const std::lock_guard<std::mutex> lg(flags_mtx_);
        queued_project_file_name_ = path;
        open_project_file_queued_ = true;
    }
    else if (fcn == Function::SCREENSHOT)
    {
        const CommunicationHeader& hdr = received_data.getCommunicationHeader();
        const std::string path = hdr.get(CommunicationHeaderObjectType::SCREENSHOT_BASE_PATH).as<properties::Label>().data;

        // Queued, not called directly: perform_screenshot does real Qt/AppKit
        // widget work and must run on the GUI thread — see the comment on
        // perform_screenshot in message_router.h. Drained by poll().
        const std::lock_guard<std::mutex> lg(flags_mtx_);
        queued_screenshot_path_ = path;
        screenshot_queued_ = true;
    }
    else if (fcn == Function::QUERY_FOR_SYNC_OF_GUI_DATA)
    {
        sendGuiStateToClient();
    }
    else if (isGuiRelatedFunction(fcn))
    {
        handleGuiManipulation(received_data);
    }
    else
    {
        addActionToQueue(received_data);
    }
}

void MessageRouter::mainWindowFlushMultipleElements(const ReceivedData& received_data)
{
    const CommunicationHeader& hdr = received_data.getCommunicationHeader();
    const uint8_t num_names = hdr.get(CommunicationHeaderObjectType::NUM_NAMES).as<uint8_t>();
    const VectorConstView<uint8_t> name_lengths{received_data.payloadData(), static_cast<size_t>(num_names)};

    std::vector<std::string> names;
    size_t idx = num_names;

    for (size_t k = 0; k < num_names; k++)
    {
        names.emplace_back();
        std::string& current_elem = names.back();
        const uint8_t current_element_length = name_lengths(k);
        for (size_t i = 0; i < current_element_length; i++)
        {
            current_elem += received_data.payloadData()[idx];
            idx++;
        }
    }

    CommunicationHeader faked_hdr{Function::FLUSH_ELEMENT};
    const uint64_t num_bytes_hdr = faked_hdr.numBytes();
    const uint64_t num_bytes = num_bytes_hdr + 1 + 2 * sizeof(uint64_t);

    FillableUInt8Array fillable_array{num_bytes};
    fillable_array.fillWithStaticType(isBigEndian());
    fillable_array.fillWithStaticType(kMagicNumber);
    fillable_array.fillWithStaticType(num_bytes);
    faked_hdr.fillBufferWithData(fillable_array);

    const UInt8ArrayView array_view{fillable_array.data(), fillable_array.size()};

    const std::lock_guard<std::mutex> lg(queued_data_mtx_);
    for (const auto& name : names)
    {
        ReceivedData fake_received_data{array_view.size()};
        std::memcpy(fake_received_data.rawData(), array_view.data(), array_view.size());
        fake_received_data.parseHeader();

        queued_data_[name].push(std::make_unique<InputData>(fake_received_data));
    }
}

void MessageRouter::poll()
{
    bool should_open_project_file = false;
    std::string project_file_to_open;
    std::vector<std::string> new_window_element_names;
    bool should_take_screenshot = false;
    std::string screenshot_path;

    {
        const std::lock_guard<std::mutex> lg(flags_mtx_);

        if (open_project_file_queued_)
        {
            open_project_file_queued_ = false;
            should_open_project_file = true;
            project_file_to_open = queued_project_file_name_;
        }

        new_window_element_names.swap(pending_new_window_names_);

        if (screenshot_queued_)
        {
            screenshot_queued_ = false;
            should_take_screenshot = true;
            screenshot_path = queued_screenshot_path_;
        }
    }

    if (should_open_project_file)
    {
        callbacks_.open_project_file(project_file_to_open);
    }

    for (const std::string& element_name : new_window_element_names)
    {
        // Defensive re-check: something else (a manual "new window" click, a
        // project load, or an earlier name from this very batch) may have
        // already created a matching PlotPane since this name was queued.
        if (callbacks_.find_plot_pane(element_name) == nullptr)
        {
            callbacks_.create_new_window_for_element(element_name);
        }
    }

    if (should_take_screenshot)
    {
        callbacks_.perform_screenshot(screenshot_path);
    }

    std::map<std::string, std::queue<std::unique_ptr<InputData>>> local_queued_data;
    {
        const std::lock_guard<std::mutex> lg(queued_data_mtx_);
        std::swap(local_queued_data, queued_data_);
    }

    for (auto& qa : local_queued_data)
    {
        if (qa.second.empty())
        {
            continue;
        }

        PlotPane* plot_pane = callbacks_.find_plot_pane(qa.first);
        if (plot_pane != nullptr)
        {
            plot_pane->pushQueue(qa.second);
        }
        else
        {
            LUMOS_LOG_WARNING() << "No PlotPane found for element '" << qa.first << "' while draining queued data; "
                                 << qa.second.size() << " item(s) discarded.";
        }
    }
}
