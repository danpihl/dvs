#pragma once
#include <iostream>
#include "opengl_low_level/opengl_header.h"

class IndexBuffer
{
public:
    IndexBuffer();
    ~IndexBuffer();
    void createIndexBuffer(const uint32_t* data, uint32_t count);
    void Bind() const;
    void UnBind() const;
private:
    uint32_t ib_id;
    uint32_t m_count;
};
