#include "plot_objects/stream_object_base/stream_object_base.h"

StreamObjectBase::StreamObjectBase() {}

StreamObjectBase::StreamObjectBase(const SubscribedStreamSettings& subscribed_stream_settings,
                                   const ShaderCollection& shader_collection)
    : subscribed_stream_settings_{subscribed_stream_settings}, shader_collection_{shader_collection}
{
}
