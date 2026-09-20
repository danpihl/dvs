#ifndef STREAM_OBJECT_BASE_H
#define STREAM_OBJECT_BASE_H

#include "project_state/project_settings.h"
#include "serial_interface/object_types.h"
#include "shader.h"

class StreamObjectBase
{
protected:
    SubscribedStreamSettings subscribed_stream_settings_;
    ShaderCollection shader_collection_;

public:
    StreamObjectBase();
    StreamObjectBase(const SubscribedStreamSettings& subscribed_stream_settings,
                     const ShaderCollection& shader_collection);

    virtual void appendNewData(const std::shared_ptr<objects::BaseObject>& obj) = 0;
    virtual void appendNewData(const std::vector<std::shared_ptr<objects::BaseObject>>& obj) = 0;
    virtual void clear() = 0;

    virtual void render() = 0;
};

#endif  // STREAM_OBJECT_BASE_H
