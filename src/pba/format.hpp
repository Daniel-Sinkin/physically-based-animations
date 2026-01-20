#pragma once

#include <format>
#include <glm/glm.hpp>
#include <string_view>

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
        out = float_fmt.format(v.x, ctx);
        *out++ = ',';
        *out++ = ' ';
        out = float_fmt.format(v.y, ctx);
        *out++ = ',';
        *out++ = ' ';
        out = float_fmt.format(v.z, ctx);
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
                out = float_fmt.format(m[col][row], ctx);
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

}  // namespace std
