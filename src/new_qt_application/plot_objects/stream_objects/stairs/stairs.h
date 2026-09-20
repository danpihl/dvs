#ifndef STREAM_OBJECTS_STAIRS_H
#define STREAM_OBJECTS_STAIRS_H

#include "opengl_low_level/vertex_buffer.h"
#include "plot_objects/stream_object_base/stream_object_base.h"

struct __attribute__((packed)) TimeAndValue
{
    float t;
    float value;
};

class StairsStream : public StreamObjectBase
{
private:
    TimeAndValue* points_ptr_;
    GLuint sp_vertex_buffer_, sp_vertex_buffer_array_;
    size_t num_elements_to_draw_{0U};

    RGBTripletf color_{0.0f, 0.0f, 0.0f};
    uint64_t previous_timestamp_{0U};

public:
    StairsStream();
    StairsStream(const SubscribedStreamSettings& subscribed_stream_settings, const ShaderCollection& shader_collection);
    void appendNewData(const std::shared_ptr<objects::BaseObject>& obj) override;
    void appendNewData(const std::vector<std::shared_ptr<objects::BaseObject>>& obj) override;
    void clear() override;
    void render() override;
};

#endif  // STREAM_OBJECTS_STAIRS_H
