// pba/gfx/video_recorder.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/video_recorder.hpp"
//
#include <print>

#ifndef DS_PBA_FFMPEG_EXECUTABLE
#    define DS_PBA_FFMPEG_EXECUTABLE "ffmpeg"
#endif

namespace ds_pba
{
namespace
{
[[nodiscard]] constexpr usize rgba_frame_bytes(int w, int h) noexcept
{
    if (w <= 0 || h <= 0)
    {
        return 0zu;
    }
    return static_cast<usize>(w) * static_cast<usize>(h) * 4zu;
}
}  // namespace

VideoRecorder::~VideoRecorder()
{
    stop();
}

[[nodiscard]] bool VideoRecorder::is_recording() const noexcept
{
    return pipe != nullptr;
}

std::string VideoRecorder::quote_arg(std::string_view s)
{
    // Escapes quotes
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('"');
    for (const char c : s)
    {
        if (c == '"')
        {
            out += "\\\"";
        }
        else
        {
            out.push_back(c);
        }
    }
    out.push_back('"');
    return out;
}

[[nodiscard]] FILE* VideoRecorder::open_pipe_write(const std::string& cmd)
{
    return popen(cmd.c_str(), "w");
}

[[nodiscard]] int VideoRecorder::close_pipe(FILE* pipe) noexcept
{
    return pclose(pipe);
}

[[nodiscard]] bool VideoRecorder::start(std::filesystem::path path)
{
    {
        Expects((width > -1 && height > -1) && "Need to init width and height before recording");
    }

    stop();

    bool ok{true};

    if (width <= 0 || height <= 0 || fps <= 0)
    {
        std::println(
            "[Warning] Trying to start recording with (width={},height={},fps={}). Skipping.",
            width,
            height,
            fps
        );
        ok = false;
    }

    if (ok)
    {
        output_path = std::move(path);
        frames_written = 0;

        if (output_path.has_parent_path())
        {
            std::error_code ec{};
            std::filesystem::create_directories(output_path.parent_path(), ec);

            if (ec)
            {
                std::println(
                    "[Warning] Failed to create directories for output path '{}': {}",
                    output_path.string(),
                    ec.message()
                );
                assert(!pipe);
                ok = false;
            }
        }
    }

    if (ok)
    {
        const std::string ffmpeg{quote_arg(std::string_view{DS_PBA_FFMPEG_EXECUTABLE})};
        const std::string out{quote_arg(output_path.string())};
        assert(!out.empty());

        const std::string cmd = std::format(
            "{} -y -hide_banner -loglevel error "
            "-f rawvideo -pixel_format rgba -video_size {}x{} -framerate {} -i - "
            "-vf vflip "
            "-an "
            "-c:v libx264 -preset veryfast -crf 18 -pix_fmt yuv420p -movflags +faststart "
            "{}",
            ffmpeg,
            width,
            height,
            fps,
            out
        );

        pipe = open_pipe_write(cmd);
        if (!pipe)
        {
            std::println("[Warning] Failed to open pipe for command '{}'", cmd);
            ok = false;
        }
    }

    {
        Ensures(ok == (pipe != nullptr));
        return ok;
    }
}

[[nodiscard]] bool VideoRecorder::write_frame(std::span<const u8> rgba)
{
    if (!pipe)
    {
        return false;
    }

    const usize expected = rgba_frame_bytes(width, height);
    if (expected == 0zu || rgba.size() != expected)
    {
        return false;
    }

    const auto written = std::fwrite(rgba.data(), 1, static_cast<size_t>(expected), pipe);
    if (written != static_cast<size_t>(expected))
    {
        stop();
        return false;
    }
    ++frames_written;
    return true;
}

void VideoRecorder::stop() noexcept
{
    if (!pipe)
    {
        return;
    }
    std::fflush(pipe);
    (void) close_pipe(pipe);
    pipe = nullptr;
}

}  // namespace ds_pba
