#include "VertexBuffer.h"

VertexBuffer2::VertexBuffer2()
    : vb_id(0)
{
}

VertexBuffer2::~VertexBuffer2()
{
    glDeleteBuffers(1, &vb_id);
}

void VertexBuffer2::createVertexBuffer(const void* const data, const uint32_t size)
{
    glGenBuffers(1, &vb_id);
    glBindBuffer(GL_ARRAY_BUFFER, vb_id);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

void VertexBuffer2::Bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, vb_id);
}

void VertexBuffer2::UnBind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
