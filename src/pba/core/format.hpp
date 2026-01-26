// pba/core/format.hpp
#pragma once

#include "pba/engine/scene_types.hpp"
#include "pba/gfx/raycast.hpp"

#include <format>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace std
{
template <>
struct formatter<glm::vec3>
{
    formatter<float> float_fmt;

    constexpr auto parse(format_parse_context& ctx)
    {
        return float_fmt.parse(ctx);
    }

    template <class FormatContext>
    auto format(const glm::vec3& v, FormatContext& ctx) const
    {
        auto out = ctx.out();
        *out++ = '(';
        ctx.advance_to(out);
        out = float_fmt.format(v.x, ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        out = float_fmt.format(v.y, ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        out = float_fmt.format(v.z, ctx);
        ctx.advance_to(out);
        *out++ = ')';
        return out;
    }
};

template <>
struct formatter<glm::quat>
{
    formatter<float> float_fmt;

    constexpr auto parse(format_parse_context& ctx)
    {
        return float_fmt.parse(ctx);
    }

    template <class FormatContext>
    auto format(const glm::quat& q, FormatContext& ctx) const
    {
        auto out = ctx.out();
        *out++ = '(';
        ctx.advance_to(out);

        out = float_fmt.format(q.w, ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        out = float_fmt.format(q.x, ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        out = float_fmt.format(q.y, ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        out = float_fmt.format(q.z, ctx);
        ctx.advance_to(out);
        *out++ = ')';
        return out;
    }
};

template <>
struct formatter<glm::mat4>
{
    formatter<float> float_fmt;

    constexpr auto parse(format_parse_context& ctx)
    {
        return float_fmt.parse(ctx);
    }

    template <class FormatContext>
    auto format(const glm::mat4& m, FormatContext& ctx) const
    {
        auto out = ctx.out();
        *out++ = '[';

        for (int row = 0; row < 4; ++row)
        {
            if (row != 0)
            {
                *out++ = '\n';
                *out++ = ' ';
            }

            for (int col = 0; col < 4; ++col)
            {
                ctx.advance_to(out);
                out = float_fmt.format(m[col][row], ctx);
                ctx.advance_to(out);
                if (col != 3)
                {
                    *out++ = ' ';
                }
            }
        }

        *out++ = ']';
        return out;
    }
};

template <>
struct formatter<ds_pba::Transform>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}')
        {
            throw format_error("Invalid format specifier for ds_pba::Transform");
        }
        return it;
    }

    template <class FormatContext>
    auto format(const ds_pba::Transform& t, FormatContext& ctx) const
    {
        return std::format_to(
            ctx.out(),
            "Transform{{position={}, scale={}, orientation={}}}",
            t.position,
            t.scale,
            t.orientation
        );
    }
};

template <>
struct formatter<ds_pba::Ray>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}')
        {
            throw format_error("Invalid format specifier for ds_pba::Ray");
        }
        return it;
    }

    template <class FormatContext>
    auto format(const ds_pba::Ray& r, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "Ray{{origin={}, dir={}}}", r.origin, r.dir);
    }
};

template <>
struct formatter<ds_pba::Raycast>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}')
        {
            throw format_error("Invalid format specifier for ds_pba::Raycast");
        }
        return it;
    }

    template <class FormatContext>
    auto format(const ds_pba::Raycast& rc, FormatContext& ctx) const
    {
        const char* type_str{""};
        switch (rc.object_type)
        {
            case ds_pba::ObjectType::Cube:
                type_str = "Cube";
                break;
            case ds_pba::ObjectType::Sphere:
                type_str = "Sphere";
                break;
            case ds_pba::ObjectType::Hitmarker:
                type_str = "Hitmarker";
                break;
            case ds_pba::ObjectType::MarbleBust:
                type_str = "MarbleBust";
                break;
        }

        return std::format_to(
            ctx.out(),
            "Raycast{{ray={}, hit={}, t={}, object_id={}, object_type={}}}",
            rc.ray,
            rc.hit,
            rc.t,
            rc.object_id,
            type_str
        );
    }
};
}  // namespace std
