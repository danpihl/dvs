#pragma once
#include <iostream>
#include "opengl_low_level/opengl_header.h"

#include <vector>
#include <assert.h>

struct VertexBufferElement
{
public:
    uint32_t type;
    uint32_t count;
    unsigned char normalized;

    static uint32_t GetSizeOfType(uint32_t type)
    {
        switch (type)
        {
        case GL_FLOAT:          return 4;
        case GL_UNSIGNED_INT:   return 4;
        case GL_UNSIGNED_BYTE:  return 1;
        }
        assert(false);
        return 0;
    }
private:
};

class VertexBufferLayout
{
public:
    VertexBufferLayout();
    void AddFloat(const uint32_t count);
    void AddUnsignedInt(uint32_t count);
    void AddUnsignedChar(uint32_t count);
    inline const std::vector<VertexBufferElement> GetElements() const { return m_Elements; }
    inline uint32_t GetStride() const { return m_Stride; }
private:
    std::vector<VertexBufferElement> m_Elements;
    uint32_t m_Stride;
    void Push(uint32_t type, uint32_t count, unsigned char normalized);
};
