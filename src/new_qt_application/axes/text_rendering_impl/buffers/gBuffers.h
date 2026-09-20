#pragma once
#include "VertexBuffer.h"
#include "VertexArray.h"
#include "IndexBuffer.h"

class gBuffers
{
public:
    gBuffers();
    ~gBuffers();
    void Bind() const;
    void UnBind() const;
    void CreateBuffers(const void* vb_data,
        uint32_t& vb_size,
        const uint32_t* ib_indices,
        uint32_t& ib_count,
        VertexBufferLayout& layout);
    VertexBuffer2 vbo;
    VertexArray   vao;
    IndexBuffer   ibo;
};
