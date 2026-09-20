#include "VertexBufferLayout.h"

VertexBufferLayout::VertexBufferLayout()
    : m_Stride(0)
{
}

void VertexBufferLayout::AddFloat(const uint32_t count)
{
    Push(GL_FLOAT, count, GL_FALSE);
}

void VertexBufferLayout::AddUnsignedInt(uint32_t count)
{
    Push(GL_UNSIGNED_INT, count, GL_FALSE);
}

void VertexBufferLayout::AddUnsignedChar(uint32_t count)
{
    Push(GL_UNSIGNED_BYTE, count, GL_TRUE);
}

void VertexBufferLayout::Push(uint32_t type, uint32_t count, unsigned char normalized)
{
    m_Elements.push_back({ type, count, normalized });
    m_Stride += count * VertexBufferElement::GetSizeOfType(type);
}
