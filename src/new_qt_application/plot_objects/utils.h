#ifndef MAIN_APPLICATION_PLOT_OBJECTS_UTILS_H_
#define MAIN_APPLICATION_PLOT_OBJECTS_UTILS_H_

#include <stdint.h>

#include <iostream>

#include "lumos/plotting/enumerations.h"
#include "lumos/logging.h"
#include "lumos/math.h"
#include "misc/rgb_triplet.h"
#include "opengl_low_level/opengl_header.h"

using namespace lumos::internal;
using namespace lumos;

inline size_t getNumDimensionsFromFunction(const Function fcn)
{
    // TODO: This shouldn't be "num dimensions", as it becomes weird when using imShow, which is 2D
    // but it's only one "dimension" (the image), as opposed to surf, which has 3 "dimensions": x, y, z matrices
    // Name it 'num_components'?
    switch (fcn)
    {
        case Function::STAIRS:
            return 2;
        case Function::PLOT2:
            return 2;
        case Function::PLOT3:
            return 3;
        case Function::SCREEN_SPACE_PRIMITIVE:
            return 2;
        case Function::FAST_PLOT2:
            return 2;
        case Function::FAST_PLOT3:
            return 3;
        case Function::REAL_TIME_PLOT:
            return 2;
        case Function::LINE_COLLECTION2:
            return 2;
        case Function::LINE_COLLECTION3:
            return 3;
        case Function::STEM:
            return 2;
        case Function::SCATTER2:
            return 2;
        case Function::SCATTER3:
            return 3;
        case Function::SURF:
            return 3;
        case Function::IM_SHOW:
            return 1;
        case Function::POLYGON_FROM_4_POINTS:
            return 1;
        case Function::PLANE_XY:
            return 1;
        case Function::PLANE_XZ:
            return 1;
        case Function::PLANE_YZ:
            return 1;
        case Function::DRAW_TRIANGLES_3D:
            return 3;
        case Function::DRAW_TRIANGLE_3D:
            return 3;
        case Function::DRAW_MESH_SEPARATE_VECTORS:
        case Function::DRAW_MESH:
            return 3;
        case Function::DRAW_LINE3D:
            return 3;
        case Function::DRAW_ARROW:
            return 1;
        case Function::QUIVER:
            return 2;
        case Function::DRAW_LINE_BETWEEN_POINTS_3D:
            return 3;
        case Function::DRAW_TILES:
            return 1;
        case Function::PLOT_COLLECTION2:
            return 2;
        case Function::PLOT_COLLECTION3:
            return 3;
        case Function::PROPERTIES_EXTENSION:
            return 1;
        case Function::PROPERTIES_EXTENSION_MULTIPLE:
            return 1;

        default:
            return 1;
    }
}

inline GLint dataTypeToGLInt(const DataType& data_type)
{
    GLint gl_type = 0;
    switch (data_type)
    {
        case DataType::FLOAT:
            gl_type = GL_FLOAT;
            break;
        case DataType::DOUBLE:
            gl_type = GL_DOUBLE;
            break;
        case DataType::INT8:
            gl_type = GL_BYTE;
            break;
        case DataType::INT16:
            gl_type = GL_SHORT;
            break;
        case DataType::INT32:
            gl_type = GL_INT;
            break;
        case DataType::INT64:
            gl_type = GL_FLOAT;
            LUMOS_ASSERT(false);  // Haven't fround int64 in opengl enums yet...
            break;
        case DataType::UINT8:
            gl_type = GL_UNSIGNED_BYTE;
            break;
        case DataType::UINT16:
            gl_type = GL_UNSIGNED_SHORT;
            break;
        case DataType::UINT32:
            gl_type = GL_UNSIGNED_INT;
            break;
        case DataType::UINT64:
            LUMOS_ASSERT(false);  // Haven't fround uint64 in opengl enums yet...
            gl_type = GL_FLOAT;
            break;
        case DataType::UNKNOWN:
            throw std::runtime_error("Got unknown data type!");
            break;
        default:
            throw std::runtime_error("Didn't find valid data type!");
            break;
    }

    return gl_type;
}

template <typename T>
std::pair<Vec3d, Vec3d> findMinMaxFromThreeVectorsInternal(uint8_t* const data_buffer,
                                                           const size_t num_elements,
                                                           const size_t num_bytes_for_one_vec)
{
    Vec3d min_vec, max_vec;
    VectorView<T> x{reinterpret_cast<T*>(data_buffer), num_elements},
        y{reinterpret_cast<T*>(&(data_buffer[num_bytes_for_one_vec])), num_elements},
        z{reinterpret_cast<T*>(&(data_buffer[2 * num_bytes_for_one_vec])), num_elements};

    std::tie<double, double>(min_vec.x, max_vec.x) = x.findMinMax();
    std::tie<double, double>(min_vec.y, max_vec.y) = y.findMinMax();
    std::tie<double, double>(min_vec.z, max_vec.z) = z.findMinMax();

    return std::pair<Vec3d, Vec3d>(min_vec, max_vec);
}

inline std::pair<Vec3d, Vec3d> findMinMaxFromThreeVectors(uint8_t* const data_buffer,
                                                          const size_t num_elements,
                                                          const size_t num_bytes_for_one_vec,
                                                          const DataType data_type)
{
    if (data_type == DataType::FLOAT)
    {
        return findMinMaxFromThreeVectorsInternal<float>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::DOUBLE)
    {
        return findMinMaxFromThreeVectorsInternal<double>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::INT8)
    {
        return findMinMaxFromThreeVectorsInternal<int8_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::INT16)
    {
        return findMinMaxFromThreeVectorsInternal<int16_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::INT32)
    {
        return findMinMaxFromThreeVectorsInternal<int32_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::INT64)
    {
        return findMinMaxFromThreeVectorsInternal<int64_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::UINT8)
    {
        return findMinMaxFromThreeVectorsInternal<uint8_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::UINT16)
    {
        return findMinMaxFromThreeVectorsInternal<uint16_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::UINT32)
    {
        return findMinMaxFromThreeVectorsInternal<uint32_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::UINT64)
    {
        return findMinMaxFromThreeVectorsInternal<uint64_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::UNKNOWN)
    {
        throw std::runtime_error("Unknown data type!");
    }
    else
    {
        throw std::runtime_error("Got default in data type switch!");
    }

    return std::pair<Vec3d, Vec3d>(Vec3d(), Vec3d());
}

template <typename T>
std::pair<Vec2d, Vec2d> findMinMaxFromTwoVectorsInternal(uint8_t* const data_buffer,
                                                         const size_t num_elements,
                                                         const size_t num_bytes_for_one_vec)
{
    VectorView<T> x{reinterpret_cast<T*>(data_buffer), num_elements},
        y{reinterpret_cast<T*>(&(data_buffer[num_bytes_for_one_vec])), num_elements};
    Vec2d min_vec, max_vec;

    std::tie<double, double>(min_vec.x, max_vec.x) = x.findMinMax();
    std::tie<double, double>(min_vec.y, max_vec.y) = y.findMinMax();

    return std::pair<Vec2d, Vec2d>(min_vec, max_vec);
}

inline std::pair<Vec2d, Vec2d> findMinMaxFromTwoVectors(uint8_t* const data_buffer,
                                                        const size_t num_elements,
                                                        const size_t num_bytes_for_one_vec,
                                                        const DataType data_type)
{
    if (data_type == DataType::FLOAT)
    {
        return findMinMaxFromTwoVectorsInternal<float>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::DOUBLE)
    {
        return findMinMaxFromTwoVectorsInternal<double>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::INT8)
    {
        return findMinMaxFromTwoVectorsInternal<int8_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::INT16)
    {
        return findMinMaxFromTwoVectorsInternal<int16_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::INT32)
    {
        return findMinMaxFromTwoVectorsInternal<int32_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::INT64)
    {
        return findMinMaxFromTwoVectorsInternal<int64_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::UINT8)
    {
        return findMinMaxFromTwoVectorsInternal<uint8_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::UINT16)
    {
        return findMinMaxFromTwoVectorsInternal<uint16_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::UINT32)
    {
        return findMinMaxFromTwoVectorsInternal<uint32_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::UINT64)
    {
        return findMinMaxFromTwoVectorsInternal<uint64_t>(data_buffer, num_elements, num_bytes_for_one_vec);
    }
    else if (data_type == DataType::UNKNOWN)
    {
        throw std::runtime_error("Unknown data type!");
    }
    else
    {
        throw std::runtime_error("Got default in data type switch!");
    }

    return std::pair<Vec2d, Vec2d>(Vec2d(), Vec2d());
}

template <typename T>
std::pair<Vec3d, Vec3d> findMinMaxFromThreeMatricesInternal(uint8_t* const data_buffer,
                                                            const size_t num_rows,
                                                            const size_t num_cols,
                                                            const size_t num_bytes_for_one_mat)
{
    MatrixView<T> x{reinterpret_cast<T*>(data_buffer), num_rows, num_cols},
        y{reinterpret_cast<T*>(&(data_buffer[num_bytes_for_one_mat])), num_rows, num_cols},
        z{reinterpret_cast<T*>(&(data_buffer[2 * num_bytes_for_one_mat])), num_rows, num_cols};
    Vec3<T> min_vec, max_vec;

    std::tie<T, T>(min_vec.x, max_vec.x) = x.findMinMax();
    std::tie<T, T>(min_vec.y, max_vec.y) = y.findMinMax();
    std::tie<T, T>(min_vec.z, max_vec.z) = z.findMinMax();

    return std::pair<Vec3d, Vec3d>(min_vec, max_vec);
}

inline std::pair<Vec3d, Vec3d> findMinMaxFromThreeMatrices(uint8_t* const data_buffer,
                                                           const size_t num_rows,
                                                           const size_t num_cols,
                                                           const size_t num_bytes_for_one_mat,
                                                           const DataType data_type)
{
    if (data_type == DataType::FLOAT)
    {
        return findMinMaxFromThreeMatricesInternal<float>(data_buffer, num_rows, num_cols, num_bytes_for_one_mat);
    }
    else if (data_type == DataType::DOUBLE)
    {
        return findMinMaxFromThreeMatricesInternal<double>(data_buffer, num_rows, num_cols, num_bytes_for_one_mat);
    }
    else if (data_type == DataType::INT8)
    {
        return findMinMaxFromThreeMatricesInternal<int8_t>(data_buffer, num_rows, num_cols, num_bytes_for_one_mat);
    }
    else if (data_type == DataType::INT16)
    {
        return findMinMaxFromThreeMatricesInternal<int16_t>(data_buffer, num_rows, num_cols, num_bytes_for_one_mat);
    }
    else if (data_type == DataType::INT32)
    {
        return findMinMaxFromThreeMatricesInternal<int32_t>(data_buffer, num_rows, num_cols, num_bytes_for_one_mat);
    }
    else if (data_type == DataType::INT64)
    {
        return findMinMaxFromThreeMatricesInternal<int64_t>(data_buffer, num_rows, num_cols, num_bytes_for_one_mat);
    }
    else if (data_type == DataType::UINT8)
    {
        return findMinMaxFromThreeMatricesInternal<uint8_t>(data_buffer, num_rows, num_cols, num_bytes_for_one_mat);
    }
    else if (data_type == DataType::UINT16)
    {
        return findMinMaxFromThreeMatricesInternal<uint16_t>(data_buffer, num_rows, num_cols, num_bytes_for_one_mat);
    }
    else if (data_type == DataType::UINT32)
    {
        return findMinMaxFromThreeMatricesInternal<uint32_t>(data_buffer, num_rows, num_cols, num_bytes_for_one_mat);
    }
    else if (data_type == DataType::UINT64)
    {
        return findMinMaxFromThreeMatricesInternal<uint64_t>(data_buffer, num_rows, num_cols, num_bytes_for_one_mat);
    }
    else if (data_type == DataType::UNKNOWN)
    {
        throw std::runtime_error("Unknown data type!");

        return std::pair<Vec3d, Vec3d>(Vec3d(), Vec3d());
    }
    else
    {
        throw std::runtime_error("Got default in data type switch!");

        return std::pair<Vec3d, Vec3d>(Vec3d(), Vec3d());
    }
}

#endif  // MAIN_APPLICATION_PLOT_OBJECTS_UTILS_H_
