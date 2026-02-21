// pba/assets/gltf_mesh.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/assets/gltf_mesh.hpp"
#include "pba/assets/mesh_data.hpp"
#include "pba/assets/model_config.hpp"
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/scene/world_types.hpp"
#include "pba/util/scope_timer.hpp"
//
#include <gsl/assert>
#include <print>
//
#include <glm/gtc/quaternion.hpp>
#include <tiny_gltf.h>

namespace ds_pba
{
namespace
{

static auto find_model_gltf_file(const std::filesystem::path& model_dir)
    -> std::optional<std::filesystem::path>
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

static auto
read_vec3_f32_strided(not_null<const std::byte*> base, usize stride, usize stride_offset) noexcept
    -> glm::vec3
{
    {
        Expects(stride >= 12zu);
    }
    const std::byte* loc{base.get() + stride_offset * stride};
    f32 x{}, y{}, z{};
    std::memcpy(&x, loc + 0, 4);
    std::memcpy(&y, loc + 4, 4);
    std::memcpy(&z, loc + 8, 4);
    return glm::vec3{x, y, z};
}

static auto face_normal(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) noexcept
    -> glm::vec3
{
    return safe_normalize(glm::cross(p1 - p0, p2 - p0));
}

template <class T>
static auto read_unaligned(not_null<const std::byte*> p) noexcept -> T
{
    T v{};
    std::memcpy(&v, p.get(), sizeof(T));
    return v;
}

static auto read_indices_u32(const tinygltf::Model& model, int accessor_index)
    -> std::optional<std::vector<u32>>
{
    if (accessor_index < 0)
    {
        return std::vector<u32>{};
    }
    if (accessor_index >= static_cast<int>(model.accessors.size()))
    {
        std::println(stderr, "Index accessor out of bounds: {}", accessor_index);
        return std::nullopt;
    }

    const tinygltf::Accessor& a{model.accessors[static_cast<usize>(accessor_index)]};
    if (a.type != TINYGLTF_TYPE_SCALAR)
    {
        std::println(stderr, "Index accessor has non-scalar type={}", a.type);
        return std::nullopt;
    }
    if (a.bufferView < 0 || a.bufferView >= static_cast<int>(model.bufferViews.size()))
    {
        std::println(stderr, "Index accessor bufferView out of bounds: {}", a.bufferView);
        return std::nullopt;
    }

    const tinygltf::BufferView& bv{model.bufferViews[static_cast<usize>(a.bufferView)]};
    if (bv.buffer < 0 || bv.buffer >= static_cast<int>(model.buffers.size()))
    {
        std::println(stderr, "Index buffer out of bounds: {}", bv.buffer);
        return std::nullopt;
    }

    const tinygltf::Buffer& buf{model.buffers[static_cast<usize>(bv.buffer)]};

    const auto base_off = static_cast<usize>(bv.byteOffset) + static_cast<usize>(a.byteOffset);
    const auto count = static_cast<usize>(a.count);

    const auto elem_size =
        static_cast<usize>(tinygltf::GetComponentSizeInBytes(static_cast<u32>(a.componentType)));

    // byteStride == 0 is allowed and just means tightly packed
    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#data-alignment
    const auto stride = (bv.byteStride != 0) ? static_cast<usize>(bv.byteStride) : elem_size;
    const auto buf_size = buf.data.size();

    if (count == 0zu)
    {
        return std::vector<u32>{};
    }

    const usize last_off{(count - 1zu) * stride};

    {  // Bounds check
        const auto base_out_of_bounds = base_off > buf_size;
        const auto last_out_of_bounds = last_off > buf_size - base_off;
        const auto elem_out_of_bounds = elem_size > buf_size - base_off - last_off;
        if (base_out_of_bounds || last_out_of_bounds || elem_out_of_bounds)
        {
            std::println(
                stderr,
                "Index accessor OOB: base_off={}, count={}, stride={}, elem_size={}, "
                "buffer_size={}",
                base_off,
                count,
                stride,
                elem_size,
                buf_size
            );
            return std::nullopt;
        }
    }

    const std::byte* base{reinterpret_cast<const std::byte*>(buf.data.data() + base_off)};

    std::vector<u32> out(count);
    switch (a.componentType)
    {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            for (usize i{0zu}; i < count; ++i)
            {
                out[i] =
                    static_cast<u32>(read_unaligned<std::uint8_t>(not_null{base + i * stride}));
            }
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            for (usize i{0zu}; i < count; ++i)
            {
                out[i] =
                    static_cast<u32>(read_unaligned<std::uint16_t>(not_null{base + i * stride}));
            }
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            for (usize i{0zu}; i < count; ++i)
            {
                out[i] = read_unaligned<std::uint32_t>(not_null{base + i * stride});
            }
            break;

        default:
            std::println(stderr, "Unsupported index componentType={}", a.componentType);
            return std::nullopt;
    }

    return out;
}

static auto get_vec3_f32_view(const tinygltf::Model& m, int accessor_index)
    -> std::optional<AccessorView>
{
    if (accessor_index < 0 || accessor_index >= static_cast<int>(m.accessors.size()))
    {
        std::println(stderr, "Accessor index OOB (vec3): {}", accessor_index);
        return std::nullopt;
    }

    const tinygltf::Accessor& a{m.accessors[static_cast<usize>(accessor_index)]};
    if (a.type != TINYGLTF_TYPE_VEC3 || a.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
    {
        std::println(
            stderr, "Unsupported vec3 accessor (type={}, componentType={})", a.type, a.componentType
        );
        return std::nullopt;
    }

    if (a.bufferView < 0 || a.bufferView >= static_cast<int>(m.bufferViews.size()))
    {
        std::println(stderr, "Accessor bufferView OOB (vec3): {}", a.bufferView);
        return std::nullopt;
    }

    const tinygltf::BufferView& bv{m.bufferViews[static_cast<usize>(a.bufferView)]};
    if (bv.buffer < 0 || bv.buffer >= static_cast<int>(m.buffers.size()))
    {
        std::println(stderr, "Accessor buffer OOB (vec3): {}", bv.buffer);
        return std::nullopt;
    }

    const tinygltf::Buffer& buf{m.buffers[static_cast<usize>(bv.buffer)]};

    constexpr auto size_elem = 3zu * sizeof(f32);
    const auto stride = (bv.byteStride != 0) ? static_cast<usize>(bv.byteStride) : size_elem;
    if (stride < size_elem)
    {
        std::println(stderr, "Invalid vec3 stride {} (< {})", stride, size_elem);
        return std::nullopt;
    }

    const auto base_off = static_cast<usize>(bv.byteOffset + a.byteOffset);
    const auto count = static_cast<usize>(a.count);

    if (base_off > buf.data.size())
    {
        std::println(stderr, "vec3 accessor base offset out of range");
        return std::nullopt;
    }

    if (count != 0zu)
    {
        const auto last = (count - 1zu) * stride;
        if (base_off + last + size_elem > buf.data.size())
        {
            std::println(stderr, "vec3 accessor OOB (count/stride)");
            return std::nullopt;
        }
    }

    return AccessorView{
        .base = reinterpret_cast<const std::byte*>(buf.data.data() + base_off),
        .stride = stride,
        .count = count,
    };
}

struct Vec2AccessorView
{
    const std::byte* base{};
    usize stride{};
    usize count{};
    int component_type{};
    bool normalized{};
};

static auto get_vec2_view(const tinygltf::Model& m, int accessor_index)
    -> std::optional<Vec2AccessorView>
{
    if (accessor_index < 0 || accessor_index >= static_cast<int>(m.accessors.size()))
    {
        std::println(stderr, "Accessor index OOB (vec2): {}", accessor_index);
        return std::nullopt;
    }

    const tinygltf::Accessor& a{m.accessors[static_cast<usize>(accessor_index)]};
    if (a.type != TINYGLTF_TYPE_VEC2)
    {
        std::println(stderr, "Unsupported vec2 accessor type={}", a.type);
        return std::nullopt;
    }

    if (a.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT
        && a.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE
        && a.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
    {
        std::println(stderr, "Unsupported vec2 componentType={}", a.componentType);
        return std::nullopt;
    }

    if (a.bufferView < 0 || a.bufferView >= static_cast<int>(m.bufferViews.size()))
    {
        std::println(stderr, "Accessor bufferView OOB (vec2): {}", a.bufferView);
        return std::nullopt;
    }

    const tinygltf::BufferView& bv{m.bufferViews[static_cast<usize>(a.bufferView)]};
    if (bv.buffer < 0 || bv.buffer >= static_cast<int>(m.buffers.size()))
    {
        std::println(stderr, "Accessor buffer OOB (vec2): {}", bv.buffer);
        return std::nullopt;
    }

    const tinygltf::Buffer& buf{m.buffers[static_cast<usize>(bv.buffer)]};

    const auto comp_size =
        static_cast<usize>(tinygltf::GetComponentSizeInBytes(static_cast<u32>(a.componentType)));

    const auto elem = 2zu * comp_size;
    const auto stride = (bv.byteStride != 0) ? static_cast<usize>(bv.byteStride) : elem;
    if (stride < elem)
    {
        std::println(stderr, "Invalid vec2 stride {} (< {})", stride, elem);
        return std::nullopt;
    }

    const auto base_off = static_cast<usize>(bv.byteOffset) + static_cast<usize>(a.byteOffset);
    const auto count = static_cast<usize>(a.count);

    if (base_off > buf.data.size())
    {
        std::println(stderr, "vec2 accessor base offset out of range");
        return std::nullopt;
    }

    if (count != 0zu)
    {
        const usize last{(count - 1zu) * stride};
        if (base_off + last + elem > buf.data.size())
        {
            std::println(stderr, "vec2 accessor OOB (count/stride)");
            return std::nullopt;
        }
    }

    return Vec2AccessorView{
        .base = reinterpret_cast<const std::byte*>(buf.data.data() + base_off),
        .stride = stride,
        .count = count,
        .component_type = a.componentType,
        .normalized = a.normalized,
    };
}

static auto read_vec2_as_f32(const Vec2AccessorView& v, usize i) -> glm::vec2
{
    const std::byte* loc = v.base + i * v.stride;

    switch (v.component_type)
    {
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            {
                const auto u = read_unaligned<f32>(loc + 0);
                const auto w = read_unaligned<f32>(loc + 4);
                return glm::vec2{u, w};
            }

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            {
                const auto u = read_unaligned<u8>(loc + 0);
                const auto w = read_unaligned<u8>(loc + 1);
                if (v.normalized)
                {
                    return glm::vec2{static_cast<f32>(u) / 255.0f, static_cast<f32>(w) / 255.0f};
                }
                return glm::vec2{static_cast<f32>(u), static_cast<f32>(w)};
            }

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            {
                const auto u = read_unaligned<u16>(loc + 0);
                const auto w = read_unaligned<u16>(loc + 2);
                if (v.normalized)
                {
                    return glm::vec2{
                        static_cast<f32>(u) / 65535.0f, static_cast<f32>(w) / 65535.0f
                    };
                }
                return glm::vec2{static_cast<f32>(u), static_cast<f32>(w)};
            }

        default:
            std::println(
                stderr,
                "v.component tpye in read_vec2_as_f32 is {} which is invalid. This should have "
                "been caught in get_vec2_view",
                v.component_type
            );
            return glm::vec2{0.0f, 0.0f};
    }
}

static auto load_tinygltf_model(const std::string& path) -> std::optional<tinygltf::Model>
{
    tinygltf::Model model{};
    std::string err{};
    std::string warn{};

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

    if (!warn.empty())
    {
        std::println("[Warning] Got warning in gltf loading:\n{}", warn);
    }

    if (!res)
    {
        if (!err.empty())
        {
            std::println(stderr, "glTF load failed '{}': {}", path, err);
        }
        else
        {
            std::println(stderr, "glTF load failed '{}' (no error string)", path);
        }
        return std::nullopt;
    }

    return model;
}

struct ParsedPrimitive
{
    tinygltf::Model model{};
    usize mesh_index{0zu};
    usize primitive_index{0zu};

    [[nodiscard]] auto primitive() const noexcept -> const tinygltf::Primitive&
    {
        return model.meshes[mesh_index].primitives[primitive_index];
    }
};

struct PrimitiveViews
{
    AccessorView positions{};
    bool has_normals{false};
    AccessorView normals{};
    bool has_uv{false};
    Vec2AccessorView uv{};
    std::vector<u32> indices{};
};

[[nodiscard]] static auto parse_first_triangle_primitive(const std::string& path)
    -> std::optional<ParsedPrimitive>
{
    auto model_opt = load_tinygltf_model(path);
    if (!model_opt)
    {
        return std::nullopt;
    }

    ParsedPrimitive parsed{
        .model = std::move(*model_opt),
        .mesh_index = 0zu,
        .primitive_index = 0zu,
    };

    if (parsed.model.meshes.empty())
    {
        std::println(stderr, "glTF '{}' has no meshes", path);
        return std::nullopt;
    }

    const tinygltf::Mesh& mesh0{parsed.model.meshes[parsed.mesh_index]};
    if (mesh0.primitives.empty())
    {
        std::println(stderr, "glTF '{}' first mesh has no primitives", path);
        return std::nullopt;
    }

    const tinygltf::Primitive& prim{mesh0.primitives[parsed.primitive_index]};
    if (prim.mode != -1 && prim.mode != TINYGLTF_MODE_TRIANGLES)
    {
        std::println(stderr, "glTF '{}' primitive mode unsupported: {}", path, prim.mode);
        return std::nullopt;
    }

    return parsed;
}

[[nodiscard]] static auto validate_primitive_views(
    const tinygltf::Model& model,
    const tinygltf::Primitive& prim,
    std::string_view path,
    bool require_uv
) -> std::optional<PrimitiveViews>
{
    PrimitiveViews views{};

    const auto it_pos = prim.attributes.find("POSITION");
    if (it_pos == prim.attributes.end())
    {
        std::println(stderr, "glTF '{}' missing POSITION attribute", path);
        return std::nullopt;
    }

    const int pos_accessor_index{it_pos->second};
    if (pos_accessor_index < 0)
    {
        std::println(stderr, "glTF '{}' POSITION accessor must be non-negative", path);
        return std::nullopt;
    }

    auto pos_view_opt = get_vec3_f32_view(model, pos_accessor_index);
    if (!pos_view_opt)
    {
        std::println(stderr, "glTF '{}' failed to read POSITION accessor", path);
        return std::nullopt;
    }
    views.positions = *pos_view_opt;

    if (auto it_n = prim.attributes.find("NORMAL"); it_n != prim.attributes.end())
    {
        const int nrm_accessor_index{it_n->second};
        if (nrm_accessor_index >= 0)
        {
            if (auto nrm_view_opt = get_vec3_f32_view(model, nrm_accessor_index))
            {
                const AccessorView nrm_view{*nrm_view_opt};
                if (nrm_view.count == views.positions.count)
                {
                    views.has_normals = true;
                    views.normals = nrm_view;
                }
            }
        }
    }

    if (auto it_uv = prim.attributes.find("TEXCOORD_0"); it_uv != prim.attributes.end())
    {
        const int uv_accessor_index{it_uv->second};
        if (uv_accessor_index < 0)
        {
            std::println(stderr, "glTF '{}' TEXCOORD_0 accessor must be non-negative", path);
            return std::nullopt;
        }

        auto uv_view_opt = get_vec2_view(model, uv_accessor_index);
        if (!uv_view_opt)
        {
            std::println(stderr, "glTF '{}' failed to read TEXCOORD_0 accessor", path);
            return std::nullopt;
        }

        const Vec2AccessorView uv_view{*uv_view_opt};
        if (uv_view.count != views.positions.count)
        {
            std::println(
                stderr,
                "glTF '{}' TEXCOORD_0 count mismatch (uv_count={}, pos_count={})",
                path,
                uv_view.count,
                views.positions.count
            );
            return std::nullopt;
        }

        views.has_uv = true;
        views.uv = uv_view;
    }
    else if (require_uv)
    {
        std::println(stderr, "glTF '{}' missing TEXCOORD_0 attribute", path);
        return std::nullopt;
    }

    auto idx_opt = read_indices_u32(model, prim.indices);
    if (!idx_opt)
    {
        std::println(stderr, "glTF '{}' failed to read indices", path);
        return std::nullopt;
    }
    views.indices = std::move(*idx_opt);

    return views;
}

template <class EmitVertexFn>
static auto convert_primitive_to_triangles(
    const PrimitiveViews& views,
    const Transform& preprocess,
    std::string_view path,
    EmitVertexFn&& emit_vertex
) -> bool
{
    const auto model_matrix = preprocess.model_matrix();
    const auto normal_matrix = model_matrix.normal_matrix();

    auto read_pos = [&](u32 i) -> Pos3
    {
        const glm::vec3 p{
            read_vec3_f32_strided(views.positions.base, views.positions.stride, static_cast<usize>(i))
        };
        return model_matrix.transform_position(p);
    };

    auto read_nrm = [&](u32 i) -> Dir3
    {
        Expects(views.has_normals);
        const glm::vec3 n{
            safe_normalize(
                read_vec3_f32_strided(views.normals.base, views.normals.stride, static_cast<usize>(i))
            )
        };
        return safe_normalize(normal_matrix.m * n);
    };

    const auto vertex_count = views.positions.count;
    if (!views.indices.empty())
    {
        if ((views.indices.size() % 3zu) != 0zu)
        {
            std::println(
                stderr, "glTF '{}' indices are not triangles (count={})", path, views.indices.size()
            );
            return false;
        }

        for (usize t{0zu}; t < views.indices.size(); t += 3zu)
        {
            const auto i0 = views.indices[t + 0zu];
            const auto i1 = views.indices[t + 1zu];
            const auto i2 = views.indices[t + 2zu];

            if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
            {
                std::println(stderr, "glTF '{}' invalid index in primitive", path);
                return false;
            }

            const Pos3 p0{read_pos(i0)};
            const Pos3 p1{read_pos(i1)};
            const Pos3 p2{read_pos(i2)};

            if (views.has_normals)
            {
                emit_vertex(i0, p0, read_nrm(i0));
                emit_vertex(i1, p1, read_nrm(i1));
                emit_vertex(i2, p2, read_nrm(i2));
            }
            else
            {
                const Dir3 fn{face_normal(p0, p1, p2)};
                emit_vertex(i0, p0, fn);
                emit_vertex(i1, p1, fn);
                emit_vertex(i2, p2, fn);
            }
        }
        return true;
    }

    if ((vertex_count % 3zu) != 0zu)
    {
        std::println(stderr, "glTF '{}' non-indexed primitive not triangles", path);
        return false;
    }

    for (u32 i = 0u; i < static_cast<u32>(vertex_count); i += 3u)
    {
        const auto i0 = i + 0u;
        const auto i1 = i + 1u;
        const auto i2 = i + 2u;

        const Pos3 p0{read_pos(i0)};
        const Pos3 p1{read_pos(i1)};
        const Pos3 p2{read_pos(i2)};

        if (views.has_normals)
        {
            emit_vertex(i0, p0, read_nrm(i0));
            emit_vertex(i1, p1, read_nrm(i1));
            emit_vertex(i2, p2, read_nrm(i2));
        }
        else
        {
            const Dir3 fn{face_normal(p0, p1, p2)};
            emit_vertex(i0, p0, fn);
            emit_vertex(i1, p1, fn);
            emit_vertex(i2, p2, fn);
        }
    }

    return true;
}

}  // namespace

auto load_gltf_mesh(const std::string& path, const Transform& preprocess)
    -> std::optional<MeshDataPN>
{
    const ScopeTimer timer{path};

    auto parsed_opt = parse_first_triangle_primitive(path);
    if (!parsed_opt)
    {
        return std::nullopt;
    }
    const auto views_opt =
        validate_primitive_views(parsed_opt->model, parsed_opt->primitive(), path, false);
    if (!views_opt)
    {
        return std::nullopt;
    }
    const PrimitiveViews& views{*views_opt};

    std::vector<MeshV_PN> vertices{};
    const auto reserve_count = views.indices.empty() ? views.positions.count : views.indices.size();
    vertices.reserve(reserve_count);

    const auto ok = convert_primitive_to_triangles(
        views,
        preprocess,
        path,
        [&](u32 i, const Pos3& p, const Dir3& n) -> void
        {
            (void) i;
            vertices.emplace_back(p.x, p.y, p.z, n.x, n.y, n.z);
        }
    );
    if (!ok)
    {
        return std::nullopt;
    }

    return MeshDataPN{.vertices = std::move(vertices)};
}

auto load_model_mesh(std::string_view model_name) -> std::optional<MeshDataPN>
{
    namespace fs = std::filesystem;
    const fs::path model_dir = fs::path{k_fp_assets} / k_fp_assets_models / model_name;

    auto cfg_opt = load_or_create_model_config(model_dir);
    if (!cfg_opt)
    {
        std::println(stderr, "Model config error for '{}'", model_name);
        return std::nullopt;
    }

    auto file_res = find_model_gltf_file(model_dir);
    if (!file_res)
    {
        std::println(stderr, "Model file error for '{}'", model_name);
        return std::nullopt;
    }

    const std::string path{file_res->string()};
    return load_gltf_mesh(path, cfg_opt->transform);
}

auto load_gltf_mesh_pnt(const std::string& path, const Transform& preprocess)
    -> std::optional<MeshDataPNT>
{
    const ScopeTimer timer{path};

    auto parsed_opt = parse_first_triangle_primitive(path);
    if (!parsed_opt)
    {
        return std::nullopt;
    }

    const auto views_opt =
        validate_primitive_views(parsed_opt->model, parsed_opt->primitive(), path, true);
    if (!views_opt)
    {
        return std::nullopt;
    }
    const PrimitiveViews& views{*views_opt};
    Expects(views.has_uv);

    std::vector<MeshV_PNT> vertices{};
    const auto reserve_count = views.indices.empty() ? views.positions.count : views.indices.size();
    vertices.reserve(reserve_count);

    const auto ok = convert_primitive_to_triangles(
        views,
        preprocess,
        path,
        [&](u32 i, const Pos3& p, const Dir3& n) -> void
        {
            const auto uv = read_vec2_as_f32(views.uv, static_cast<usize>(i));
            vertices.emplace_back(p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y);
        }
    );
    if (!ok)
    {
        return std::nullopt;
    }

    return MeshDataPNT{.vertices = std::move(vertices)};
}

auto load_model_mesh_pnt(std::string_view model_name) -> std::optional<MeshDataPNT>
{
    namespace fs = std::filesystem;
    const auto model_dir = fs::path{k_fp_assets} / k_fp_assets_models / model_name;

    auto cfg_res = load_or_create_model_config(model_dir);
    if (!cfg_res)
    {
        std::println(stderr, "Model config error for '{}'", model_name);
        return std::nullopt;
    }

    auto file_res = find_model_gltf_file(model_dir);
    if (!file_res)
    {
        std::println(stderr, "Model file error for '{}'", model_name);
        return std::nullopt;
    }

    const auto path = file_res->string();
    return load_gltf_mesh_pnt(path, cfg_res->transform);
}

}  // namespace ds_pba
