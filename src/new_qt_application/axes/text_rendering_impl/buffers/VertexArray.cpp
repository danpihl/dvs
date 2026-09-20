#include "VertexArray.h"

VertexArray::VertexArray()
    : va_id(0)
{
}

VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &va_id);
}

void VertexArray::createVertexArray()
{
    glGenVertexArrays(1, &va_id);
}

void VertexArray::Bind() const
{
    glBindVertexArray(va_id);
}

void VertexArray::UnBind() const
{
    glBindVertexArray(0);
}

void VertexArray::AddBuffer(const VertexBuffer2& vbo, const VertexBufferLayout& layout)
{
    Bind();
    vbo.Bind();

    const auto& elements = layout.GetElements();
    uint32_t offset = 0;

    for (int i = 0; i < (int)elements.size(); i++)
    {
        const auto& element = elements[i];
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, element.count, element.type,
            element.normalized, layout.GetStride(), (const void*)(uintptr_t)offset);
        offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
    }

    vbo.UnBind();
    UnBind();
}
