// pba/gltf_mesh.cpp
#include "pba/gltf_mesh.hpp"

#include "pba/core_types.hpp"
#include "pba/gl_types.hpp"
#include "pba/pch.hpp"  // IWYU pragma: keep
#include "pba/util/scope_timer.hpp"

#include <print>
#include <tiny_gltf.h>

namespace ds_pba
{
namespace
{
struct V
{
    f32 px, py, pz;  // position
    f32 nx, ny, nz;  // normals
};

class ScopedBufferBinding
{
  public:
    explicit ScopedBufferBinding(GLMesh& mesh)
    {
        mesh.vao.bind();
        mesh.vbo.bind();
    }
    ~ScopedBufferBinding()
    {
        VBO::unbind();
        VAO::unbind();
    }

    ScopedBufferBinding(const ScopedBufferBinding&) = delete;
    ScopedBufferBinding& operator=(const ScopedBufferBinding&) = delete;

    ScopedBufferBinding(ScopedBufferBinding&&) = delete;
    ScopedBufferBinding& operator=(ScopedBufferBinding&&) = delete;
};

static const std::byte* accessor_data_begin(
    const tinygltf::Model& model,
    const tinygltf::Accessor& accessor,
    const tinygltf::BufferView& view
)
{
    const tinygltf::Buffer& buf = model.buffers[static_cast<usize>(view.buffer)];
    const usize off = static_cast<usize>(view.byteOffset + accessor.byteOffset);
    return reinterpret_cast<const std::byte*>(buf.data.data() + off);
}

static usize
accessor_stride_bytes(const tinygltf::Accessor& accessor, const tinygltf::BufferView& view)
{
    const usize default_stride =
        static_cast<usize>(
            tinygltf::GetComponentSizeInBytes(static_cast<u32>(accessor.componentType))
        )
        * static_cast<usize>(tinygltf::GetNumComponentsInType(static_cast<u32>(accessor.type)));

    return (view.byteStride != 0) ? static_cast<usize>(view.byteStride) : default_stride;
}

static glm::vec3 read_vec3_f32_strided(const std::byte* base, usize stride, usize stride_offset)
{
    const std::byte* loc = base + stride_offset * stride;
    f32 x{}, y{}, z{};
    std::memcpy(&x, loc + 0, 4);
    std::memcpy(&y, loc + 4, 4);
    std::memcpy(&z, loc + 8, 4);
    return glm::vec3{x, y, z};
}

static glm::vec3 safe_normalize(glm::vec3 v) noexcept
{
    const f32 len = glm::length(v);
    if (len <= 1e-12f)
    {
        return glm::vec3{0.0f, 0.0f, 1.0f};
    }
    return v / len;
}

static glm::vec3 face_normal(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) noexcept
{
    return safe_normalize(glm::cross(p1 - p0, p2 - p0));
}

static std::expected<std::vector<u32>, GltfLoadError>
read_indices_u32(const tinygltf::Model& model, int accessor_index)
{
    if (accessor_index < 0)
    {
        return std::vector<u32>{};
    }
    if (accessor_index >= static_cast<int>(model.accessors.size()))
    {
        return std::unexpected(GltfLoadError::UnsupportedAccessorType);
    }

    const tinygltf::Accessor& a = model.accessors[static_cast<usize>(accessor_index)];
    if (a.type != TINYGLTF_TYPE_SCALAR)
    {
        return std::unexpected(GltfLoadError::UnsupportedAccessorType);
    }
    if (a.bufferView < 0 || a.bufferView >= static_cast<int>(model.bufferViews.size()))
    {
        std::println(stderr, "Buffer View OOB");
        return std::unexpected(GltfLoadError::ParseError);
    }

    const tinygltf::BufferView& bv = model.bufferViews[static_cast<usize>(a.bufferView)];
    const std::byte* base = accessor_data_begin(model, a, bv);

    const usize count = static_cast<usize>(a.count);
    std::vector<u32> out;
    out.resize(count);

    switch (a.componentType)
    {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            for (usize i = 0; i < count; ++i)
            {
                out[i] = static_cast<u32>(reinterpret_cast<const std::uint8_t*>(base)[i]);
            }
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            for (usize i = 0; i < count; ++i)
            {
                out[i] = static_cast<u32>(reinterpret_cast<const std::uint16_t*>(base)[i]);
            }
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            for (usize i = 0; i < count; ++i)
            {
                out[i] = static_cast<u32>(reinterpret_cast<const std::uint32_t*>(base)[i]);
            }
            break;
        default:
            return std::unexpected(GltfLoadError::UnsupportedAccessorType);
    }

    return out;
}

static bool validate_vec3_f32(const tinygltf::Accessor& a)
{
    return (a.type == TINYGLTF_TYPE_VEC3 && a.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
}

static glm::mat3 axis_fix_matrix(AxisFix fix) noexcept
{
    using AF = AxisFix;
    using glm::mat3;
    using glm::vec3;

    const f32 c0 = 0.0f;
    const f32 c1 = 1.0f;
    const f32 cn = -1.0f;

    const auto Rx90 = [&]() -> mat3
    { return mat3(vec3(c1, c0, c0), vec3(c0, c0, c1), vec3(c0, cn, c0)); };
    const auto Rx180 = [&]() -> mat3
    { return mat3(vec3(c1, c0, c0), vec3(c0, cn, c0), vec3(c0, c0, cn)); };
    const auto Rx270 = [&]() -> mat3
    { return mat3(vec3(c1, c0, c0), vec3(c0, c0, cn), vec3(c0, c1, c0)); };

    const auto Ry90 = [&]() -> mat3
    { return mat3(vec3(c0, c0, cn), vec3(c0, c1, c0), vec3(c1, c0, c0)); };
    const auto Ry180 = [&]() -> mat3
    { return mat3(vec3(cn, c0, c0), vec3(c0, c1, c0), vec3(c0, c0, cn)); };
    const auto Ry270 = [&]() -> mat3
    { return mat3(vec3(c0, c0, c1), vec3(c0, c1, c0), vec3(cn, c0, c0)); };

    const auto Rz90 = [&]() -> mat3
    { return mat3(vec3(c0, c1, c0), vec3(cn, c0, c0), vec3(c0, c0, c1)); };
    const auto Rz180 = [&]() -> mat3
    { return mat3(vec3(cn, c0, c0), vec3(c0, cn, c0), vec3(c0, c0, c1)); };
    const auto Rz270 = [&]() -> mat3
    { return mat3(vec3(c0, cn, c0), vec3(c1, c0, c0), vec3(c0, c0, c1)); };

    // clang-format off
    switch (fix)
    {
        case AF::None:        return mat3(1.0f);

        case AF::RotX90:      return Rx90();
        case AF::RotX180:     return Rx180();
        case AF::RotX270:     return Rx270();

        case AF::RotY90:      return Ry90();
        case AF::RotY180:     return Ry180();
        case AF::RotY270:     return Ry270();

        case AF::RotZ90:      return Rz90();
        case AF::RotZ180:     return Rz180();
        case AF::RotZ270:     return Rz270();

        case AF::RotX90_Z90:   return Rz90()  * Rx90();
        case AF::RotX90_Z180:  return Rz180() * Rx90();
        case AF::RotX90_Z270:  return Rz270() * Rx90();

        case AF::RotX180_Z90:  return Rz90()  * Rx180();
        case AF::RotX180_Z270: return Rz270() * Rx180();

        case AF::RotX270_Z90:  return Rz90()  * Rx270();
        case AF::RotX270_Z180: return Rz180() * Rx270();
        case AF::RotX270_Z270: return Rz270() * Rx270();

        case AF::RotY90_Z180:  return Rz180() * Ry90();
        case AF::RotY270_Z180: return Rz180() * Ry270();

        case AF::RotZ90_X90:   return Rx90()  * Rz90();
        case AF::RotZ90_X270:  return Rx270() * Rz90();
        case AF::RotZ270_X90:  return Rx90()  * Rz270();
        case AF::RotZ270_X270: return Rx270() * Rz270();

        case AF::FlipX: return mat3(vec3(cn, c0, c0), vec3(c0, c1, c0), vec3(c0, c0, c1));
        case AF::FlipY: return mat3(vec3(c1, c0, c0), vec3(c0, cn, c0), vec3(c0, c0, c1));
        case AF::FlipZ: return mat3(vec3(c1, c0, c0), vec3(c0, c1, c0), vec3(c0, c0, cn));
    }
    // clang-format on
}

}  // namespace

std::expected<GLMesh, GltfLoadError> load_gltf_mesh(const std::string& path, AxisFix fix)
{
    util::ScopeTimer timer{path};
    tinygltf::Model model;
    std::string err;
    std::string warn;

    {  // Load the file
        tinygltf::TinyGLTF loader;
        bool res{};
        if (path.ends_with(".glb"))
        {
            res = loader.LoadBinaryFromFile(&model, &err, &warn, path);
        }
        else
        {
            if (!path.ends_with(".gltf"))
            {
                std::println("[Warning] Loading file which does not end on glb of gltf: {}", path);
            }
            res = loader.LoadASCIIFromFile(&model, &err, &warn, path);
        }
        assert(!res == !err.empty() && "Got no result but also no error!");
        if (!res)
        {
            std::println(stderr, "Got uncaught error {}", err);
            return std::unexpected(GltfLoadError::ParseError);
        }
        if (!warn.empty())
        {
            std::println("[Warning] Got warning in gltf loading:\n{}", warn);
        }
    }

    if (model.meshes.empty())
    {
        std::println(stderr, "Mesh Empty");
        return std::unexpected(GltfLoadError::ParseError);
    }

    const tinygltf::Mesh& mesh0 = model.meshes[0];
    if (mesh0.primitives.empty())
    {
        std::println(stderr, "First Primitive Empty");
        return std::unexpected(GltfLoadError::ParseError);
    }

    const tinygltf::Primitive& prim = mesh0.primitives[0];

    if (prim.mode != -1 && prim.mode != TINYGLTF_MODE_TRIANGLES)
    {
        return std::unexpected(GltfLoadError::UnsupportedPrimitive);
    }

    const auto it_pos = prim.attributes.find("POSITION");
    if (it_pos == prim.attributes.end())
    {
        return std::unexpected(GltfLoadError::MissingPosition);
    }

    const int pos_idx_i = it_pos->second;
    if (pos_idx_i < 0)
    {
        std::println(stderr, "pos accessor must be positive");
        return std::unexpected(GltfLoadError::ParseError);
    }
    const usize pos_idx = static_cast<usize>(pos_idx_i);
    if (pos_idx >= model.accessors.size())
    {
        std::println(
            stderr,
            "Malformed Position Accessor, is {} but there are only {} accessors",
            pos_idx,
            model.accessors.size()
        );
        return std::unexpected(GltfLoadError::ParseError);
    }

    const tinygltf::Accessor& pos_accessor = model.accessors[pos_idx];
    if (!validate_vec3_f32(pos_accessor))
    {
        return std::unexpected(GltfLoadError::UnsupportedAccessorType);
    }
    int buffer_view_i{pos_accessor.bufferView};
    if (buffer_view_i < 0)
    {
        std::println(stderr, "bufferView must not be negative");
        return std::unexpected(GltfLoadError::ParseError);
    }
    auto buffer_view = static_cast<usize>(buffer_view_i);
    if (buffer_view >= model.bufferViews.size())
    {
        std::println(
            stderr,
            "bufferView OOB, has index {} but there are only {} bufferViews",
            buffer_view,
            model.bufferViews.size()
        );
        return std::unexpected(GltfLoadError::ParseError);
    }

    const tinygltf::BufferView& pos_bv = model.bufferViews[buffer_view];
    const std::byte* pos_base = accessor_data_begin(model, pos_accessor, pos_bv);
    const usize pos_stride = accessor_stride_bytes(pos_accessor, pos_bv);

    const usize vertex_count = pos_accessor.count;

    bool has_normals{false};
    const std::byte* nrm_base = nullptr;
    usize nrm_stride = 0;

    if (auto it_n = prim.attributes.find("NORMAL"); it_n != prim.attributes.end())
    {
        const int normal_idx_i = it_n->second;
        if (normal_idx_i < 0)
        {
            std::println(stderr, "Malformed Normal Accessor, shouldn't be negative");
            return std::unexpected(GltfLoadError::ParseError);
        }
        const auto normal_idx = static_cast<usize>(normal_idx_i);
        if (normal_idx >= model.accessors.size())
        {
            std::println(
                stderr,
                "Malformed Normal Accessor, is {} but there are only {} accessors",
                normal_idx,
                model.accessors.size()
            );
            return std::unexpected(GltfLoadError::ParseError);
        }
        const tinygltf::Accessor& normal_accessor = model.accessors[normal_idx];
        if (validate_vec3_f32(normal_accessor))
        {
            if (normal_accessor.count == pos_accessor.count && normal_accessor.bufferView >= 0
                && normal_accessor.bufferView < static_cast<int>(model.bufferViews.size()))
            {
                const tinygltf::BufferView& nrm_bv_local =
                    model.bufferViews[static_cast<usize>(normal_accessor.bufferView)];
                nrm_base = accessor_data_begin(model, normal_accessor, nrm_bv_local);
                nrm_stride = accessor_stride_bytes(normal_accessor, nrm_bv_local);
                has_normals = true;
            }
        }
    }

    auto idx_res = read_indices_u32(model, prim.indices);
    if (!idx_res)
    {
        return std::unexpected(idx_res.error());
    }
    const std::vector<u32>& idx = *idx_res;

    const glm::mat3 preprocessing = axis_fix_matrix(fix);

    auto read_pos = [&](u32 i) -> glm::vec3
    {
        const glm::vec3 p = read_vec3_f32_strided(pos_base, pos_stride, static_cast<usize>(i));
        return preprocessing * p;
    };

    auto read_nrm = [&](u32 i) -> glm::vec3
    {
        const glm::vec3 n =
            safe_normalize(read_vec3_f32_strided(nrm_base, nrm_stride, static_cast<usize>(i)));
        return safe_normalize(preprocessing * n);
    };

    std::vector<V> verts;

    if (!idx.empty())
    {
        if ((idx.size() % 3u) != 0u)
        {
            return std::unexpected(GltfLoadError::UnsupportedPrimitive);
        }

        verts.reserve(idx.size());

        for (usize t = 0; t < idx.size(); t += 3)
        {
            const u32 i0 = idx[t + 0];
            const u32 i1 = idx[t + 1];
            const u32 i2 = idx[t + 2];

            if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
            {
                std::println("Invalid ij counts");
                return std::unexpected(GltfLoadError::ParseError);
            }

            const glm::vec3 p0{read_pos(i0)};
            const glm::vec3 p1{read_pos(i1)};
            const glm::vec3 p2{read_pos(i2)};

            if (has_normals)
            {
                auto emit = [&](u32 i, const glm::vec3& p)
                {
                    const glm::vec3 n{read_nrm(i)};
                    verts.emplace_back(p.x, p.y, p.z, n.x, n.y, n.z);
                };
                emit(i0, p0);
                emit(i1, p1);
                emit(i2, p2);
            }
            else
            {
                verts.emplace_back(p0.x, p0.y, p0.z);
                verts.emplace_back(p1.x, p1.y, p1.z);
                verts.emplace_back(p2.x, p2.y, p2.z);
            }
        }
    }
    else
    {
        if ((vertex_count % 3u) != 0u)
        {
            return std::unexpected(GltfLoadError::UnsupportedPrimitive);
        }

        verts.reserve(vertex_count);

        u32 vertex_idx{0};
        while (vertex_idx < vertex_count)
        {
            const u32 i0{vertex_idx++};
            const u32 i1{vertex_idx++};
            const u32 i2{vertex_idx++};

            const glm::vec3 p0 = read_pos(i0);
            const glm::vec3 p1 = read_pos(i1);
            const glm::vec3 p2 = read_pos(i2);

            const glm::vec3 fn = has_normals ? glm::vec3{} : face_normal(p0, p1, p2);

            auto emit = [&](u32 i, const glm::vec3& p)
            {
                const glm::vec3 n = has_normals ? read_nrm(i) : fn;
                verts.push_back(V{p.x, p.y, p.z, n.x, n.y, n.z});
            };

            emit(i0, p0);
            emit(i1, p1);
            emit(i2, p2);
        }
    }

    GLMesh out{};
    glGenVertexArrays(1, out.vao.ptr());
    glGenBuffers(1, out.vbo.ptr());
    out.vertex_count = static_cast<GLsizei>(verts.size());

    {
        ScopedBufferBinding bind{out};

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(verts.size() * sizeof(V)),
            verts.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset0());

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset(3 * sizeof(f32)));
    }

    return out;
}
}  // namespace ds_pba
