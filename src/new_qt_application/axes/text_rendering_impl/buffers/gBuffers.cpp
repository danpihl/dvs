#include "gBuffers.h"

gBuffers::gBuffers()
    : vbo(), vao(), ibo()
{
}

gBuffers::~gBuffers()
{
}

void gBuffers::CreateBuffers(const void* vb_data,
    uint32_t& vb_size,
    const uint32_t* ib_indices,
    uint32_t& ib_count,
    VertexBufferLayout& vb_layout)
{
    vao.createVertexArray();
    vbo.createVertexBuffer(vb_data, vb_size);
    ibo.createIndexBuffer(ib_indices, ib_count);
    vao.AddBuffer(vbo, vb_layout);
}

void gBuffers::Bind() const
{
    vao.Bind();
    ibo.Bind();
}

void gBuffers::UnBind() const
{
    vao.UnBind();
    ibo.UnBind();
}
