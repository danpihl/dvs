#ifndef MAIN_APPLICATION_SHADER_H_
#define MAIN_APPLICATION_SHADER_H_

#include <glm/glm.hpp>
#include <stdexcept>
#include <string>

#include "lumos/plotting/internal.h"
#include "lumos/math.h"
#include "misc/rgb_triplet.h"
#include "opengl_low_level/opengl_header.h"

enum class ShaderSource
{
    FILE,
    CODE
};

class Uniform
{
private:
    GLint handle_;

public:
    Uniform() : handle_{-1} {}
    Uniform(const GLuint program_id, const std::string& uniform_name)
    {
        handle_ = glGetUniformLocation(program_id, uniform_name.c_str());
    }

    void setFloat(const float val) const
    {
        glUniform1f(handle_, val);
    }

    void setInt(const int val) const
    {
        glUniform1i(handle_, val);
    }

    void setColor(const RGBTripletf& col)
    {
        glUniform3f(handle_, col.red, col.green, col.blue);
    }

    void setVec(const lumos::Vec3f& v)
    {
        glUniform3f(handle_, v.x, v.y, v.z);
    }

    void setVec(const lumos::Vec3d& v)
    {
        // TODO: Is this correct? Vec3d is double precision, but glUniform3f takes floats
        lumos::Vec3d vf{v.x, v.y, v.z};
        glUniform3f(handle_, vf.x, vf.y, vf.z);
    }

    void setVec(const float x, const float y, const float z)
    {
        glUniform3f(handle_, x, y, z);
    }

    void setMat4x4(const glm::mat4& mat)
    {
        glUniformMatrix4fv(handle_, 1, GL_FALSE, &(mat[0][0]));
    }
};

class ShaderBase
{
protected:
    GLuint program_id_;

    void loadShadersFromSourceCode(const std::string& vertex_code, const std::string& fragment_code);
    void loadShaderFromFiles(const std::string& vertex_path, const std::string& fragment_path);

    void setBaseUniformHandles();

public:
    ShaderBase() = default;
    ShaderBase(const std::string& vertex_shader, const std::string& fragment_shader, const ShaderSource src);

    GLuint programId() const;

    void use() const;

    struct BaseUniformHandles
    {
        // Custom transform
        Uniform has_custom_transform;
        GLint custom_translation_mat;
        GLint custom_rotation_mat;
        GLint custom_scale_mat;

        // Clip planes
        Uniform use_clip_plane;

        GLint clip_plane0;
        GLint clip_plane1;
        GLint clip_plane2;
        GLint clip_plane3;
        GLint clip_plane4;
        GLint clip_plane5;

        // Misc
        Uniform vertex_color;
        Uniform has_color_vec;
        Uniform has_point_sizes_vec;

        Uniform alpha;
        Uniform min_z;
        Uniform max_z;

        Uniform model_view_proj_mat;

        Uniform axes_width;
        Uniform axes_height;

        Uniform color_map_selection;

        Uniform scatter_mode;
        Uniform point_size;

        Uniform radius;

        Uniform gap_size;
        Uniform dash_size;
    };

    BaseUniformHandles base_uniform_handles;
};

class Plot2DShader : public ShaderBase
{
private:
    void setUniformHandles();

public:
    Plot2DShader() = default;
    Plot2DShader(const std::string& vertex_shader, const std::string& fragment_shader, const ShaderSource src);

    struct UniformHandles
    {
        Uniform half_line_width;
        Uniform z_offset;
        Uniform use_dash;
        Uniform first_length;
        Uniform first_point;
        Uniform second_point;

        Uniform inverse_model_view_proj_mat;
    };

    UniformHandles uniform_handles;
};

class Plot3DShader : public ShaderBase
{
private:
    void setUniformHandles();

public:
    Plot3DShader() = default;
    Plot3DShader(const std::string& vertex_shader, const std::string& fragment_shader, const ShaderSource src);

    struct UniformHandles
    {
        Uniform half_line_width;
        Uniform use_dash;

        Uniform inverse_model_view_proj_mat;
    };

    UniformHandles uniform_handles;
};

class DrawMeshShader : public ShaderBase
{
private:
    void setUniformHandles();

public:
    DrawMeshShader() = default;
    DrawMeshShader(const std::string& vertex_shader, const std::string& fragment_shader, const ShaderSource src);

    struct UniformHandles
    {
        Uniform face_color;
        Uniform edge_color;
        Uniform is_edge;
        Uniform interpolate_colormap;
        Uniform global_illumination_active;
        Uniform light_pos;
        Uniform rotation_mat;
    };

    UniformHandles uniform_handles;
};

class ImShowShader : public ShaderBase
{
private:
    void setUniformHandles();

public:
    ImShowShader() = default;
    ImShowShader(const std::string& vertex_shader, const std::string& fragment_shader, const ShaderSource src);

    struct UniformHandles
    {
        Uniform use_global_alpha;
        Uniform global_alpha;
    };

    UniformHandles uniform_handles;
};

class ScatterShader : public ShaderBase
{
private:
    void setUniformHandles();

public:
    ScatterShader() = default;
    ScatterShader(const std::string& vertex_shader, const std::string& fragment_shader, const ShaderSource src);

    struct UniformHandles
    {
        Uniform distance_from_point;
        Uniform min_dist;
        Uniform max_dist;
        Uniform has_distance_from;
        Uniform distance_from_type;
        Uniform color_map_selection;
        Uniform has_silhouette;
        Uniform silhouette_color;
        Uniform squared_silhouette_percentage;
    };

    UniformHandles uniform_handles;
};

class TextShader : public ShaderBase
{
private:
    void setUniformHandles();

public:
    TextShader() = default;
    TextShader(const std::string& vertex_shader, const std::string& fragment_shader, const ShaderSource src);

    struct UniformHandles
    {
        GLint text_sampler;
        GLint text_color;
    };

    UniformHandles uniform_handles;
};

struct ShaderCollection
{
    TextShader text_shader;
    ShaderBase plot_box_shader;
    ShaderBase pane_background_shader;
    ScatterShader scatter_shader;
    ShaderBase basic_plot_shader;
    Plot2DShader plot_2d_shader;
    Plot3DShader plot_3d_shader;
    ImShowShader img_plot_shader;
    DrawMeshShader draw_mesh_shader;
    ShaderBase legend_shader;
    ShaderBase screen_space_shader;
};

#endif  // MAIN_APPLICATION_SHADER_H_
