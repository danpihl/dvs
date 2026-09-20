#include "IndexBuffer.h"

IndexBuffer::IndexBuffer()
    : ib_id(0), m_count(0)
{
}

IndexBuffer::~IndexBuffer()
{
    glDeleteBuffers(1, &ib_id);
}

void IndexBuffer::createIndexBuffer(const uint32_t* indices, uint32_t count)
{
    m_count = count;
    glGenBuffers(1, &ib_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
}

void IndexBuffer::Bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib_id);
}

void IndexBuffer::UnBind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
