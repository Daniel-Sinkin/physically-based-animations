// pba/assets/gltf_mesh.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/assets/gltf_mesh.hpp"
#include "pba/assets/mesh_data.hpp"
#include "pba/assets/model_config.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/util/scope_timer.hpp"

#include <glm/gtc/quaternion.hpp>
#include <print>
#include <tiny_gltf.h>

namespace ds_pba
{
namespace
{

static std::optional<std::filesystem::path>
find_model_gltf_file(const std::filesystem::path& model_dir)
{
    namespace fs = std::filesystem;
    if (!fs::exists(model_dir) || !fs::is_directory(model_dir))
    {
        return std::nullopt;
    }

    std::vector<fs::path> glb{};
    std::vector<fs::path> gltf{};

    for (const auto& e : fs::directory_iterator(model_dir))
    {
        if (!e.is_regular_file())
        {
            continue;
        }

        const auto p = e.path();
        const std::string ext{p.extension().string()};
        if (ext == ".glb")
        {
            glb.push_back(p);
        }
        else if (ext == ".gltf")
        {
            gltf.push_back(p);
        }
    }

    auto pick = [](std::vector<fs::path>& v) -> std::optional<fs::path>
    {
        if (v.empty())
        {
            return std::nullopt;
        }
        std::sort(v.begin(), v.end());
        return v.front();
    };

    if (auto p = pick(glb))
    {
        return p;
    }
    return pick(gltf);
}

static glm::vec3 read_vec3_f32_strided(const std::byte* base, usize stride, usize stride_offset)
{
    assert(stride >= 12zu);
    const std::byte* loc{base + stride_offset * stride};
    f32 x{}, y{}, z{};
    std::memcpy(&x, loc + 0, 4);
    std::memcpy(&y, loc + 4, 4);
    std::memcpy(&z, loc + 8, 4);
    return glm::vec3{x, y, z};
}

static glm::vec3 safe_normalize(glm::vec3 v) noexcept
{
    const f32 len{glm::length(v)};
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

template <class T>
[[nodiscard]] static T read_unaligned(const std::byte* p) noexcept
{
    T v{};
    std::memcpy(&v, p, sizeof(T));
    return v;
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

    const tinygltf::Accessor& a{model.accessors[static_cast<usize>(accessor_index)]};
    if (a.type != TINYGLTF_TYPE_SCALAR)
    {
        return std::unexpected(GltfLoadError::UnsupportedAccessorType);
    }
    if (a.bufferView < 0 || a.bufferView >= static_cast<int>(model.bufferViews.size()))
    {
        std::println(stderr, "Buffer View OOB");
        return std::unexpected(GltfLoadError::ParseError);
    }

    const tinygltf::BufferView& bv{model.bufferViews[static_cast<usize>(a.bufferView)]};
    if (bv.buffer < 0 || bv.buffer >= static_cast<int>(model.buffers.size()))
    {
        std::println(stderr, "Buffer OOB");
        return std::unexpected(GltfLoadError::ParseError);
    }

    const tinygltf::Buffer& buf{model.buffers[static_cast<usize>(bv.buffer)]};

    const usize base_off{static_cast<usize>(bv.byteOffset) + static_cast<usize>(a.byteOffset)};
    const usize count{static_cast<usize>(a.count)};

    const usize elem_size{
        static_cast<usize>(tinygltf::GetComponentSizeInBytes(static_cast<u32>(a.componentType)))
    };

    // byteStride == 0 is allowed and just means tightly packed
    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#data-alignment
    const usize stride{(bv.byteStride != 0) ? static_cast<usize>(bv.byteStride) : elem_size};
    const usize buf_size{buf.data.size()};
    if (count == 0zu)
    {
        return std::vector<u32>{};
    }

    const usize last_off{(count - 1zu) * stride};

    if (base_off > buf_size || last_off > buf_size - base_off
        || elem_size > buf_size - base_off - last_off)
    {
        std::println(
            stderr,
            "Index accessor OOB: base_off={}, count={}, stride={}, elem_size={}, buffer_size={}",
            base_off,
            count,
            stride,
            elem_size,
            buf_size
        );
        return std::unexpected(GltfLoadError::ParseError);
    }
    const std::byte* base{reinterpret_cast<const std::byte*>(buf.data.data() + base_off)};

    std::vector<u32> out(count);

    switch (a.componentType)
    {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            for (usize i = 0; i < count; ++i)
            {
                out[i] = static_cast<u32>(read_unaligned<std::uint8_t>(base + i * stride));
            }
            break;

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            for (usize i = 0; i < count; ++i)
            {
                out[i] = static_cast<u32>(read_unaligned<std::uint16_t>(base + i * stride));
            }
            break;

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            for (usize i = 0; i < count; ++i)
            {
                out[i] = read_unaligned<std::uint32_t>(base + i * stride);
            }
            break;

        default:
            return std::unexpected(GltfLoadError::UnsupportedAccessorType);
    }

    return out;
}

static glm::mat4 preprocess_matrix(const Transform& t) noexcept
{
    glm::mat4 M(1.0f);
    M = glm::translate(M, t.position);
    M *= glm::mat4_cast(t.orientation);
    M = glm::scale(M, t.scale);
    return M;
}

static std::expected<AccessorView, GltfLoadError>
get_vec3_f32_view(const tinygltf::Model& m, int accessor_index)
{
    if (accessor_index < 0 || accessor_index >= static_cast<int>(m.accessors.size()))
    {
        return std::unexpected(GltfLoadError::ParseError);
    }

    const tinygltf::Accessor& a{m.accessors[static_cast<usize>(accessor_index)]};
    if (a.type != TINYGLTF_TYPE_VEC3 || a.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
    {
        return std::unexpected(GltfLoadError::UnsupportedAccessorType);
    }

    if (a.bufferView < 0 || a.bufferView >= static_cast<int>(m.bufferViews.size()))
    {
        return std::unexpected(GltfLoadError::ParseError);
    }

    const tinygltf::BufferView& bv{m.bufferViews[static_cast<usize>(a.bufferView)]};
    if (bv.buffer < 0 || bv.buffer >= static_cast<int>(m.buffers.size()))
    {
        return std::unexpected(GltfLoadError::ParseError);
    }

    const tinygltf::Buffer& buf{m.buffers[static_cast<usize>(bv.buffer)]};

    constexpr usize elem{12zu};  // vec3<f32>
    const usize stride{(bv.byteStride != 0) ? static_cast<usize>(bv.byteStride) : elem};
    if (stride < elem)
    {
        return std::unexpected(GltfLoadError::ParseError);
    }

    const usize base_off{static_cast<usize>(bv.byteOffset) + static_cast<usize>(a.byteOffset)};
    const usize count{static_cast<usize>(a.count)};

    if (base_off > buf.data.size())
    {
        return std::unexpected(GltfLoadError::ParseError);
    }

    if (count != 0zu)
    {
        const usize last{(count - 1zu) * stride};
        if (base_off + last + elem > buf.data.size())
        {
            return std::unexpected(GltfLoadError::ParseError);
        }
    }

    return AccessorView{
        .base = reinterpret_cast<const std::byte*>(buf.data.data() + base_off),
        .stride = stride,
        .count = count,
    };
}

}  // namespace

std::expected<MeshDataPN, GltfLoadError>
load_gltf_mesh(const std::string& path, const Transform& preprocess)
{
    const util::ScopeTimer timer{path};

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
                std::println("[Warning] Loading file which does not end on glb or gltf: {}", path);
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

    const tinygltf::Mesh& mesh0{model.meshes[0]};
    if (mesh0.primitives.empty())
    {
        std::println(stderr, "First Primitive Empty");
        return std::unexpected(GltfLoadError::ParseError);
    }

    const tinygltf::Primitive& prim{mesh0.primitives[0]};

    if (prim.mode != -1 && prim.mode != TINYGLTF_MODE_TRIANGLES)
    {
        return std::unexpected(GltfLoadError::UnsupportedPrimitive);
    }

    const auto it_pos = prim.attributes.find("POSITION");
    if (it_pos == prim.attributes.end())
    {
        return std::unexpected(GltfLoadError::MissingPosition);
    }

    const int pos_accessor_index{it_pos->second};
    if (pos_accessor_index < 0)
    {
        std::println(stderr, "pos accessor must be non-negative");
        return std::unexpected(GltfLoadError::ParseError);
    }

    auto pos_view_res = get_vec3_f32_view(model, pos_accessor_index);
    if (!pos_view_res)
    {
        return std::unexpected(pos_view_res.error());
    }
    const AccessorView pos_view{*pos_view_res};

    const std::byte* pos_base{pos_view.base};
    const usize pos_stride{pos_view.stride};
    const usize vertex_count{pos_view.count};

    bool has_normals{false};
    const std::byte* nrm_base{};
    usize nrm_stride{0};

    if (auto it_n = prim.attributes.find("NORMAL"); it_n != prim.attributes.end())
    {
        const int nrm_accessor_index{it_n->second};
        if (nrm_accessor_index >= 0)
        {
            auto nrm_view_res = get_vec3_f32_view(model, nrm_accessor_index);
            if (nrm_view_res)
            {
                const AccessorView nrm_view{*nrm_view_res};
                if (nrm_view.count == vertex_count)
                {
                    has_normals = true;
                    nrm_base = nrm_view.base;
                    nrm_stride = nrm_view.stride;
                }
            }
        }
    }

    auto idx_res = read_indices_u32(model, prim.indices);
    if (!idx_res)
    {
        return std::unexpected(idx_res.error());
    }
    const std::vector<u32>& idx{*idx_res};

    const glm::mat4 P{preprocess_matrix(preprocess)};
    const glm::mat3 N{glm::transpose(glm::inverse(glm::mat3(P)))};

    auto apply_pos = [&](const glm::vec3& p) -> glm::vec3
    { return glm::vec3(P * glm::vec4(p, 1.0f)); };

    auto normalized = [&](const glm::vec3& n) -> glm::vec3 { return safe_normalize(N * n); };

    auto read_pos = [&](u32 i) -> glm::vec3
    {
        const glm::vec3 p{read_vec3_f32_strided(pos_base, pos_stride, static_cast<usize>(i))};
        return apply_pos(p);
    };

    auto read_nrm = [&](u32 i) -> glm::vec3
    {
        assert(has_normals);
        const glm::vec3 n{
            safe_normalize(read_vec3_f32_strided(nrm_base, nrm_stride, static_cast<usize>(i)))
        };
        return normalized(n);
    };

    std::vector<MeshV_PN> verts;
    if (!idx.empty())
    {
        if ((idx.size() % 3u) != 0u)
        {
            return std::unexpected(GltfLoadError::UnsupportedPrimitive);
        }

        verts.reserve(idx.size());

        for (usize t{0zu}; t < idx.size(); t += 3)
        {
            const u32 i0{idx[t + 0]};
            const u32 i1{idx[t + 1]};
            const u32 i2{idx[t + 2]};

            if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
            {
                std::println(stderr, "Invalid index in glTF primitive");
                return std::unexpected(GltfLoadError::ParseError);
            }

            const Position3 p0{read_pos(i0)};
            const Position3 p1{read_pos(i1)};
            const Position3 p2{read_pos(i2)};

            if (has_normals)
            {
                auto emit = [&](u32 i, const glm::vec3& p) -> void
                {
                    const Direction3 n{read_nrm(i)};
                    verts.emplace_back(p.x, p.y, p.z, n.x, n.y, n.z);
                };
                emit(i0, p0);
                emit(i1, p1);
                emit(i2, p2);
            }
            else
            {
                const Direction3 fn{face_normal(p0, p1, p2)};
                auto emit = [&](const glm::vec3& p, const glm::vec3& n) -> void
                { verts.emplace_back(p.x, p.y, p.z, n.x, n.y, n.z); };

                emit(p0, fn);
                emit(p1, fn);
                emit(p2, fn);
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

            const glm::vec3 p0{read_pos(i0)};
            const glm::vec3 p1{read_pos(i1)};
            const glm::vec3 p2{read_pos(i2)};

            const Direction3 fn{has_normals ? glm::vec3{} : face_normal(p0, p1, p2)};

            auto emit = [&](u32 i, const glm::vec3& p) -> void
            {
                const Direction3 n{has_normals ? read_nrm(i) : fn};
                verts.push_back(MeshV_PN{p.x, p.y, p.z, n.x, n.y, n.z});
            };

            emit(i0, p0);
            emit(i1, p1);
            emit(i2, p2);
        }
    }

    return MeshDataPN{.vertices = verts};
}

std::expected<MeshDataPN, GltfLoadError> load_model_mesh(std::string_view model_name)
{
    namespace fs = std::filesystem;
    const fs::path model_dir = fs::path{k_fp_assets} / k_fp_assets_models / model_name;

    auto cfg_res = load_or_create_model_config(model_dir);
    if (!cfg_res)
    {
        std::println(
            stderr,
            "Model config error for '{}': {}",
            model_name,
            std::to_underlying(cfg_res.error())
        );
        return std::unexpected(GltfLoadError::ParseError);
    }

    auto file_res = find_model_gltf_file(model_dir);
    if (!file_res)
    {
        std::println(stderr, "Model file error for '{}'", model_name);
        return std::unexpected(GltfLoadError::ParseError);
    }

    const std::string path{file_res->string()};
    return load_gltf_mesh(path, cfg_res->transform);
}

}  // namespace ds_pba
