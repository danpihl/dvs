#include "user_supplied_properties.h"

UserSuppliedProperties::UserSuppliedProperties() : has_properties_{false} {}

bool UserSuppliedProperties::hasFlag(const PropertyFlag f) const
{
    return flags_[static_cast<uint8_t>(f)];
}

bool UserSuppliedProperties::hasProperty(const lumos::internal::PropertyType tp) const
{
    return props_lut_.data[static_cast<uint8_t>(tp)] != 255;
}

UserSuppliedProperties::UserSuppliedProperties(const CommunicationHeader& hdr) :
    has_properties_{false},
    props_{hdr.getPropertiesObjects()},
    props_lut_{hdr.getPropertyLookupTable()},
    flags_{hdr.getFlags()}
{
    // TODO: Should check which function it is, and return if it's
    // not a function that requires UserSuppliedProperties

    // Flags
    is_persistent = hasFlag(PropertyFlag::PERSISTENT);
    interpolate_colormap = hasFlag(PropertyFlag::INTERPOLATE_COLORMAP);
    is_updateable = hasFlag(PropertyFlag::UPDATABLE);
    is_appendable = hasFlag(PropertyFlag::APPENDABLE);
    exclude_from_selection = hasFlag(PropertyFlag::EXCLUDE_FROM_SELECTION);
    dynamic_or_static_usage = (is_updateable || is_appendable) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    has_properties_ = has_properties_ || is_persistent || interpolate_colormap || is_updateable || is_appendable ||
                        exclude_from_selection;

    // Properties
    if (hasProperty(PropertyType::ALPHA))
    {
        alpha = getProperty<Alpha>().data;
        has_properties_ = true;
    }

    if (hasProperty(PropertyType::BUFFER_SIZE))
    {
        buffer_size = getProperty<BufferSize>().data;
        has_properties_ = true;
    }

    if (hasProperty(PropertyType::SCATTER_STYLE))
    {
        scatter_style = getProperty<ScatterStyle>();
        has_properties_ = true;
    }

    if (hasProperty(PropertyType::LINE_STYLE))
    {
        line_style = getProperty<LineStyle>();
        has_properties_ = true;
    }

    if (hasProperty(PropertyType::LINE_WIDTH))
    {
        line_width = getProperty<LineWidth>().data;
        has_properties_ = true;
    }

    if (hasProperty(PropertyType::POINT_SIZE))
    {
        point_size = getProperty<PointSize>().data;
        has_properties_ = true;
    }

    if (hasProperty(PropertyType::Z_OFFSET))
    {
        z_offset = getProperty<ZOffset>().data;
        has_properties_ = true;
    }

    if ((hasProperty(PropertyType::NAME)))
    {
        label = std::string(getProperty<Label>().data);
        has_properties_ = true;
    }

    if ((hasProperty(PropertyType::COLOR)))
    {
        const internal::ColorInternal col{getProperty<internal::ColorInternal>()};

        color = RGBTripletf{static_cast<float>(col.red) / 255.0f,
                            static_cast<float>(col.green) / 255.0f,
                            static_cast<float>(col.blue) / 255.0f};
        has_properties_ = true;
    }

    if (hasProperty(PropertyType::DISTANCE_FROM))
    {
        distance_from = getProperty<DistanceFrom>();
        has_properties_ = true;
    }

    if (hasProperty(PropertyType::TRANSFORM))
    {
        custom_transform = getProperty<Transform>();
        has_properties_ = true;
    }

    if (hasProperty(PropertyType::COLOR_MAP))
    {
        color_map = getProperty<ColorMap>();
        has_properties_ = true;
    }

    if (hasProperty(PropertyType::EDGE_COLOR))
    {
        const EdgeColor ec = getProperty<EdgeColor>();

        edge_color = RGBTripletf{static_cast<float>(ec.red) / 255.0f,
                                    static_cast<float>(ec.green) / 255.0f,
                                    static_cast<float>(ec.blue) / 255.0f};

        if (!ec.use_color)
        {
            no_edges = true;
        }
        has_properties_ = true;
    }

    if (hasProperty(PropertyType::FACE_COLOR))
    {
        const FaceColor fc = getProperty<FaceColor>();

        face_color = RGBTripletf{static_cast<float>(fc.red) / 255.0f,
                                    static_cast<float>(fc.green) / 255.0f,
                                    static_cast<float>(fc.blue) / 255.0f};

        if (!fc.use_color)
        {
            no_faces = true;
        }
        has_properties_ = true;
    }

    if (hasProperty(PropertyType::SILHOUETTE))
    {
        const Silhouette s = getProperty<Silhouette>();

        silhouette = RGBTripletf{static_cast<float>(s.red) / 255.0f,
                                    static_cast<float>(s.green) / 255.0f,
                                    static_cast<float>(s.blue) / 255.0f};

        silhouette_percentage = s.percentage;
        has_silhouette = true;
        has_properties_ = true;
    }
}

void UserSuppliedProperties::clear()
{
    has_properties_ = false;

    label = std::nullopt;

    scatter_style = std::nullopt;
    line_style = std::nullopt;

    z_offset = std::nullopt;
    alpha = std::nullopt;
    line_width = std::nullopt;
    point_size = std::nullopt;

    buffer_size = std::nullopt;

    color = std::nullopt;

    edge_color = std::nullopt;
    no_edges = kDefaultNoEdges;

    face_color = std::nullopt;
    no_faces = kDefaultNoFaces;

    silhouette = std::nullopt;
    has_silhouette = kDefaultHasSilhouette;
    silhouette_percentage = kDefaultSilhouettePercentage;

    color_map = std::nullopt;

    distance_from = std::nullopt;

    custom_transform = std::nullopt;

    is_persistent = kDefaultIsPersistent;
    is_updateable = kDefaultIsUpdateable;
    is_appendable = kDefaultIsAppendable;
    exclude_from_selection = kDefaultExcludeFromSelection;
    interpolate_colormap = kDefaultInterpolateColormap;
    dynamic_or_static_usage = kDefaultDynamicOrStaticUsage;
}

void UserSuppliedProperties::appendProperties(const UserSuppliedProperties& props)
{
    overwritePropertyFromOtherIfPresent(alpha, props.alpha);
    overwritePropertyFromOtherIfPresent(buffer_size, props.buffer_size);
    overwritePropertyFromOtherIfPresent(scatter_style, props.scatter_style);
    overwritePropertyFromOtherIfPresent(line_style, props.line_style);
    overwritePropertyFromOtherIfPresent(line_width, props.line_width);
    overwritePropertyFromOtherIfPresent(point_size, props.point_size);
    overwritePropertyFromOtherIfPresent(z_offset, props.z_offset);
    overwritePropertyFromOtherIfPresent(label, props.label);
    overwritePropertyFromOtherIfPresent(color, props.color);
    overwritePropertyFromOtherIfPresent(distance_from, props.distance_from);
    overwritePropertyFromOtherIfPresent(custom_transform, props.custom_transform);
    overwritePropertyFromOtherIfPresent(color_map, props.color_map);

    if (props.edge_color.has_value())
    {
        edge_color = props.edge_color.value();
        no_edges = props.no_edges;

        has_properties_ = true;
    }

    if (props.face_color.has_value())
    {
        face_color = props.face_color.value();
        no_faces = props.no_faces;

        has_properties_ = true;
    }

    if (props.silhouette.has_value())
    {
        silhouette = props.silhouette.value();
        has_silhouette = true;
        silhouette_percentage = props.silhouette_percentage;

        has_properties_ = true;
    }

    is_persistent = is_persistent || props.is_persistent;
    is_updateable = is_updateable || props.is_updateable;
    is_appendable = is_appendable || props.is_appendable;
    exclude_from_selection = exclude_from_selection || props.exclude_from_selection;
    interpolate_colormap = interpolate_colormap || props.interpolate_colormap;
    dynamic_or_static_usage = (is_updateable || is_appendable) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    has_properties_ =
        has_properties_ || is_persistent || interpolate_colormap || is_updateable || exclude_from_selection;
}

bool UserSuppliedProperties::hasProperties() const
{
    return has_properties_;
}
