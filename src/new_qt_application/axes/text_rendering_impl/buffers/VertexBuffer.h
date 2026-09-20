#pragma once
#include <iostream>
#include "opengl_low_level/opengl_header.h"

class VertexBuffer2
{
public:
    VertexBuffer2();
    ~VertexBuffer2();
    void createVertexBuffer(const void* const data, const uint32_t size);
    void Bind() const;
    void UnBind() const;
private:
    uint32_t vb_id;
};
