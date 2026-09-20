#pragma once
#include <iostream>
#include "opengl_low_level/opengl_header.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"

class VertexArray
{
public:
    VertexArray();
    ~VertexArray();
    void createVertexArray();
    void AddBuffer(const VertexBuffer2& vbo, const VertexBufferLayout& layout);
    void Bind() const;
    void UnBind() const;
private:
    uint32_t va_id;
};
