#include "main_application/plot_data_handler.h"

#include "lumos/math.h"
#include "lumos/plotting/utils.h"
#include "misc/rgb_triplet.h"
#include "plot_objects/plot_object_base/plot_object_base.h"
#include "plot_objects/plot_objects.h"
#include "shader.h"

PlotDataHandler::PlotDataHandler(const ShaderCollection& shader_collection)
    : pending_soft_clear_(false), shader_collection_{shader_collection}
{
    awaiting_user_supplied_properties_.resize(UINT8_MAX);
}

void PlotDataHandler::clear()
{
    for (size_t k = 0; k < plot_datas_.size(); k++)
    {
        delete plot_datas_[k];
    }
    for (size_t k = 0; k < old_plot_datas_.size(); k++)
    {
        delete old_plot_datas_[k];
    }
    plot_datas_.clear();
    old_plot_datas_.clear();
    pending_soft_clear_ = false;
    color_picker_.reset();
}

void PlotDataHandler::setTransform(const ItemId id,
                                   const FixedSizeMatrix<double, 3, 3>& rotation,
                                   const Vec3<double>& translation,
                                   const FixedSizeMatrix<double, 3, 3>& scale)
{
    const auto q = std::find_if(plot_datas_.begin(), plot_datas_.end(), [&id](const PlotObjectBase* const pd) -> bool {
        return pd->getId() == id;
    });

    if (q != plot_datas_.end())
    {
        (*q)->setTransform(rotation, translation, scale);
    }
    else
    {
        throw std::runtime_error("Called setTransform for non existing id " + std::to_string(static_cast<int>(id)));
    }
}

void PlotDataHandler::propertiesExtensionMultiple(const ReceivedData& received_data)
{
    const std::uint8_t num_property_sets{received_data.payloadData()[0]};

    const std::uint8_t* const raw_user_supplied_properties_data{received_data.payloadData() + 1U};

    std::size_t idx{0U};

    for (std::uint8_t k = 0; k < num_property_sets; k++)
    {
        CommunicationHeader hdr{internal::Function::PROPERTIES_EXTENSION_MULTIPLE};
        // For each property set
        // Num properties in prop set
        const std::uint8_t num_props{raw_user_supplied_properties_data[idx]};
        idx += 1U;

        ItemId id;
        std::memcpy(&id, raw_user_supplied_properties_data + idx, sizeof(ItemId));
        idx += sizeof(ItemId);

        hdr.append(CommunicationHeaderObjectType::ITEM_ID, id);

        for (std::uint8_t i = 0; i < num_props; i++)
        {
            CommunicationHeaderObject obj;
            obj.type = CommunicationHeaderObjectType::PROPERTY;
            obj.size = raw_user_supplied_properties_data[idx];

            idx += 1U;
            std::memcpy(obj.data, raw_user_supplied_properties_data + idx, obj.size);

            idx += obj.size;

            hdr.appendPropertyFromRawObject(obj);
        }

        const UserSuppliedProperties user_supplied_properties{hdr};

        const auto q = std::find_if(plot_datas_.begin(),
                                    plot_datas_.end(),
                                    [&id](const PlotObjectBase* const pd) -> bool { return pd->getId() == id; });
        if (q == plot_datas_.end())
        {
            awaiting_user_supplied_properties_[static_cast<int>(id)].appendProperties(user_supplied_properties);
        }
        else
        {
            (*q)->updateProperties(user_supplied_properties);
        }
    }
}

void PlotDataHandler::propertiesExtension(const ItemId id, const UserSuppliedProperties& user_supplied_properties)
{
    const auto q = std::find_if(plot_datas_.begin(), plot_datas_.end(), [&id](const PlotObjectBase* const pd) -> bool {
        return pd->getId() == id;
    });
    if (q == plot_datas_.end())
    {
        awaiting_user_supplied_properties_[static_cast<int>(id)].appendProperties(user_supplied_properties);
    }
    else
    {
        (*q)->updateProperties(user_supplied_properties);
    }
}

void PlotDataHandler::deletePlotObject(const ItemId id)
{
    const auto q = std::find_if(plot_datas_.begin(), plot_datas_.end(), [&id](const PlotObjectBase* const pd) -> bool {
        return pd->getId() == id;
    });

    if (q != plot_datas_.end())
    {
        delete (*q);
        plot_datas_.erase(q);
    }
}

void PlotDataHandler::addData(const CommunicationHeader& hdr,
                              const PlotObjectAttributes& plot_object_attributes,
                              const UserSuppliedProperties& user_supplied_properties,
                              ReceivedData& received_data,
                              const std::shared_ptr<const ConvertedDataBase>& converted_data)
{
    if (pending_soft_clear_)
    {
        pending_soft_clear_ = false;
        color_picker_.reset();

        for (size_t k = 0; k < old_plot_datas_.size(); k++)
        {
            delete old_plot_datas_[k];
        }

        old_plot_datas_.clear();
    }

    UserSuppliedProperties full_user_supplied_properties;

    if (hdr.hasObjectWithType(CommunicationHeaderObjectType::ITEM_ID))
    {
        const ItemId id = hdr.value<ItemId>();

        if (awaiting_user_supplied_properties_[static_cast<int>(id)].hasProperties())
        {
            full_user_supplied_properties.appendProperties(awaiting_user_supplied_properties_[static_cast<int>(id)]);
            awaiting_user_supplied_properties_[static_cast<int>(id)].clear();
        }

        const auto q = std::find_if(plot_datas_.begin(),
                                    plot_datas_.end(),
                                    [&id](const PlotObjectBase* const pd) -> bool { return pd->getId() == id; });

        if (q != plot_datas_.end())
        {
            full_user_supplied_properties.appendProperties(user_supplied_properties);

            if ((*q)->isAppendable())
            {
                (*q)->appendNewData(received_data, hdr, converted_data, full_user_supplied_properties);
            }
            else if ((*q)->isUpdateable())
            {
                (*q)->updateWithNewData(received_data, hdr, converted_data, full_user_supplied_properties);
            }

            return;
        }
    }

    full_user_supplied_properties.appendProperties(user_supplied_properties);

    switch (hdr.getFunction())
    {
        case Function::STAIRS:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new Stairs(hdr,
                                                                           received_data,
                                                                           converted_data,
                                                                           plot_object_attributes,
                                                                           full_user_supplied_properties,
                                                                           shader_collection_,
                                                                           color_picker_)));
            break;

        case Function::PLOT2:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new Plot2D(hdr,
                                                                           received_data,
                                                                           converted_data,
                                                                           plot_object_attributes,
                                                                           full_user_supplied_properties,
                                                                           shader_collection_,
                                                                           color_picker_)));
            break;

        case Function::PLOT3:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new Plot3D(hdr,
                                                                           received_data,
                                                                           converted_data,
                                                                           plot_object_attributes,
                                                                           full_user_supplied_properties,
                                                                           shader_collection_,
                                                                           color_picker_)));
            break;

        case Function::SCREEN_SPACE_PRIMITIVE:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new ScreenSpacePrimitive(hdr,
                                                                                       received_data,
                                                                                       converted_data,
                                                                                       plot_object_attributes,
                                                                                       full_user_supplied_properties,
                                                                                       shader_collection_,
                                                                                       color_picker_)));
            break;
        case Function::FAST_PLOT2:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new FastPlot2D(hdr,
                                                                               received_data,
                                                                               converted_data,
                                                                               plot_object_attributes,
                                                                               full_user_supplied_properties,
                                                                               shader_collection_,
                                                                               color_picker_)));
            break;

        case Function::FAST_PLOT3:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new FastPlot3D(hdr,
                                                                               received_data,
                                                                               converted_data,
                                                                               plot_object_attributes,
                                                                               full_user_supplied_properties,
                                                                               shader_collection_,
                                                                               color_picker_)));
            break;

        case Function::LINE_COLLECTION2:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new LineCollection2D(hdr,
                                                                                     received_data,
                                                                                     converted_data,
                                                                                     plot_object_attributes,
                                                                                     full_user_supplied_properties,
                                                                                     shader_collection_,
                                                                                     color_picker_)));
            break;

        case Function::LINE_COLLECTION3:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new LineCollection3D(hdr,
                                                                                     received_data,
                                                                                     converted_data,
                                                                                     plot_object_attributes,
                                                                                     full_user_supplied_properties,
                                                                                     shader_collection_,
                                                                                     color_picker_)));
            break;

        case Function::STEM:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new Stem(hdr,
                                                                         received_data,
                                                                         converted_data,
                                                                         plot_object_attributes,
                                                                         full_user_supplied_properties,
                                                                         shader_collection_,
                                                                         color_picker_)));
            break;

        case Function::SCATTER2:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new Scatter2D(hdr,
                                                                              received_data,
                                                                              converted_data,
                                                                              plot_object_attributes,
                                                                              full_user_supplied_properties,
                                                                              shader_collection_,
                                                                              color_picker_)));
            break;

        case Function::SCATTER3:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new Scatter3D(hdr,
                                                                              received_data,
                                                                              converted_data,
                                                                              plot_object_attributes,
                                                                              full_user_supplied_properties,
                                                                              shader_collection_,
                                                                              color_picker_)));
            break;

        case Function::SURF:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new Surf(hdr,
                                                                         received_data,
                                                                         converted_data,
                                                                         plot_object_attributes,
                                                                         full_user_supplied_properties,
                                                                         shader_collection_,
                                                                         color_picker_)));
            break;

        case Function::IM_SHOW:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new ImShow(hdr,
                                                                           received_data,
                                                                           converted_data,
                                                                           plot_object_attributes,
                                                                           full_user_supplied_properties,
                                                                           shader_collection_,
                                                                           color_picker_)));
            break;

        case Function::PLOT_COLLECTION2:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new PlotCollection2D(hdr,
                                                                                     received_data,
                                                                                     converted_data,
                                                                                     plot_object_attributes,
                                                                                     full_user_supplied_properties,
                                                                                     shader_collection_,
                                                                                     color_picker_)));
            break;

        case Function::PLOT_COLLECTION3:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new PlotCollection3D(hdr,
                                                                                     received_data,
                                                                                     converted_data,
                                                                                     plot_object_attributes,
                                                                                     full_user_supplied_properties,
                                                                                     shader_collection_,
                                                                                     color_picker_)));
            break;

        case Function::DRAW_MESH_SEPARATE_VECTORS:
        case Function::DRAW_MESH:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new DrawMesh(hdr,
                                                                             received_data,
                                                                             converted_data,
                                                                             plot_object_attributes,
                                                                             full_user_supplied_properties,
                                                                             shader_collection_,
                                                                             color_picker_)));
            break;

        case Function::REAL_TIME_PLOT:
            plot_datas_.push_back(dynamic_cast<PlotObjectBase*>(new ScrollingPlot2D(hdr,
                                                                                    received_data,
                                                                                    converted_data,
                                                                                    plot_object_attributes,
                                                                                    full_user_supplied_properties,
                                                                                    shader_collection_,
                                                                                    color_picker_)));
            break;
        default:
            throw std::runtime_error("Invalid function!");
            break;
    }
}

void PlotDataHandler::render()
{
    for (size_t k = 0; k < plot_datas_.size(); k++)
    {
        plot_datas_[k]->modifyShader();
        plot_datas_[k]->render();
    }

    for (size_t k = 0; k < old_plot_datas_.size(); k++)
    {
        old_plot_datas_[k]->modifyShader();
        old_plot_datas_[k]->render();
    }
}

std::vector<LegendProperties> PlotDataHandler::getLegendStrings() const
{
    std::vector<LegendProperties> names;
    for (size_t k = 0; k < plot_datas_.size(); k++)
    {
        if (plot_datas_[k]->hasLabel())
        {
            names.push_back(plot_datas_[k]->getLegendProperties());
        }
    }

    for (size_t k = 0; k < old_plot_datas_.size(); k++)
    {
        if (old_plot_datas_[k]->hasLabel())
        {
            names.push_back(old_plot_datas_[k]->getLegendProperties());
        }
    }

    return names;
}

std::pair<Vec3d, Vec3d> PlotDataHandler::getMinMaxVectors() const
{
    if (plot_datas_.size() == 0)
    {
        std::cout << "Is zero!" << std::endl;
        return std::pair<Vec3d, Vec3d>(Vec3d(-1, -1, -1), Vec3d(1, 1, 1));
    }
    else
    {
        const std::pair<Vec3d, Vec3d> min_max = plot_datas_[0]->getMinMaxVectors();
        size_t num_dimensions = plot_datas_[0]->getNumDimensions();
        Vec3d min_vec = min_max.first;
        Vec3d max_vec = min_max.second;

        bool z_is_set = num_dimensions == 3 ? true : false;

        for (size_t k = 1; k < plot_datas_.size(); k++)
        {
            const std::pair<Vec3d, Vec3d> current_min_max = plot_datas_[k]->getMinMaxVectors();

            const size_t current_num_dimensions = plot_datas_[k]->getNumDimensions();
            num_dimensions = std::max(num_dimensions, current_num_dimensions);

            if (current_num_dimensions == 3)
            {
                if (!z_is_set)
                {
                    min_vec.z = current_min_max.first.z;
                    max_vec.z = current_min_max.second.z;
                    z_is_set = true;
                }
                else
                {
                    min_vec.z = std::min(min_vec.z, current_min_max.first.z);
                    max_vec.z = std::max(max_vec.z, current_min_max.second.z);
                }
            }

            min_vec.x = std::min(min_vec.x, current_min_max.first.x);
            min_vec.y = std::min(min_vec.y, current_min_max.first.y);

            max_vec.x = std::max(max_vec.x, current_min_max.second.x);
            max_vec.y = std::max(max_vec.y, current_min_max.second.y);
        }

        if (num_dimensions == 2)
        {
            min_vec.z = -1.0;
            max_vec.z = 1.0;
        }

        if (min_vec.x == max_vec.x)
        {
            min_vec.x -= 1.0;
            max_vec.x += 1.0;
        }

        if (min_vec.y == max_vec.y)
        {
            min_vec.y -= 1.0;
            max_vec.y += 1.0;
        }

        if (min_vec.z == max_vec.z)
        {
            min_vec.z -= 1.0;
            max_vec.z += 1.0;
        }

        if (std::isnan(min_vec.x))
        {
            min_vec.x = -1.0;
        }
        if (std::isnan(min_vec.y))
        {
            min_vec.y = -1.0;
        }
        if (std::isnan(min_vec.z))
        {
            min_vec.z = -1.0;
        }

        if (std::isnan(max_vec.x))
        {
            max_vec.x = 1.0;
        }
        if (std::isnan(max_vec.y))
        {
            max_vec.y = 1.0;
        }
        if (std::isnan(max_vec.z))
        {
            max_vec.z = 1.0;
        }

        return std::pair<Vec3d, Vec3d>(min_vec, max_vec);
    }
}

void PlotDataHandler::softClear()
{
    pending_soft_clear_ = true;
    std::vector<PlotObjectBase*> new_plot_datas;
    for (size_t k = 0; k < plot_datas_.size(); k++)
    {
        if (plot_datas_[k]->isPersistent())
        {
            new_plot_datas.push_back(plot_datas_[k]);
        }
        else
        {
            old_plot_datas_.push_back(plot_datas_[k]);
        }
    }
    plot_datas_ = new_plot_datas;
}

PlotDataHandler::~PlotDataHandler()
{
    for (size_t k = 0; k < plot_datas_.size(); k++)
    {
        delete plot_datas_[k];
    }

    for (size_t k = 0; k < old_plot_datas_.size(); k++)
    {
        delete old_plot_datas_[k];
    }
}
