// pba/gfx/gfx_capture.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gfx_context.hpp"
//
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/gfx/video_recorder.hpp"
#include "pba/ui/ui.hpp"
//
#include <imgui.h>
//
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <json.hpp>

namespace ds_pba
{

bool ds_pba::GfxContext::capture_viewport_rgba8(std::vector<u8>& out) const
{
    if (!loaded_glad)
    {
        return false;
    }
    if (!viewport_fb_rect_valid)
    {
        return false;
    }
    if (viewport_fbo.fbo == 0 || viewport_fbo.color_tex == 0)
    {
        return false;
    }

    const int w{viewport_fbo.width};
    const int h{viewport_fbo.height};
    if (w <= 0 || h <= 0)
    {
        return false;
    }

    const usize bytes = static_cast<usize>(w) * static_cast<usize>(h) * 4zu;
    out.resize(bytes);

    GLint prev_fbo{0};
    GLint prev_read_buffer{0};
    GLint prev_pack_alignment{0};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    glGetIntegerv(GL_READ_BUFFER, &prev_read_buffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &prev_pack_alignment);

    glBindFramebuffer(GL_FRAMEBUFFER, viewport_fbo.fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, out.data());

    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prev_fbo));
    glReadBuffer(static_cast<GLenum>(prev_read_buffer));
    glPixelStorei(GL_PACK_ALIGNMENT, prev_pack_alignment);

    const GLenum err = glGetError();
    return err == GL_NO_ERROR;
}
void ds_pba::GfxContext::start_recording()
{
    if (recorder.is_recording())
    {
        return;
    }

    if (!viewport_fb_rect_valid)
    {
        ui_log("Recording start failed: viewport is not valid");
        return;
    }

    const int w{viewport_fbo.width};
    const int h{viewport_fbo.height};
    if (w <= 0 || h <= 0)
    {
        ui_log("Recording start failed: invalid viewport size");
        return;
    }

    std::filesystem::path out_dir = capture_output_dir;
    if (out_dir.empty())
    {
        out_dir = "renders";
    }

    std::filesystem::path out_path{};
    try
    {
        std::filesystem::create_directories(out_dir);
        const std::string file = std::format("capture_{:04}.mp4", capture_take_index++);
        out_path = out_dir / file;
    }
    catch (const std::exception& e)
    {
        ui_log(std::format("Recording start failed: {}", e.what()));
        return;
    }

    recorder.width = w;
    recorder.height = h;
    recorder.fps = capture_fps;
    if (!recorder.start(out_path))
    {
        ui_log("Recording start failed: could not start ffmpeg");
        return;
    }
    ui_log(
        std::format("Recording started: {} ({}x{} @ {} fps)", out_path.string(), w, h, capture_fps)
    );
}
void ds_pba::GfxContext::stop_recording()
{
    if (!recorder.is_recording())
    {
        return;
    }

    const std::string out = recorder.output_path.string();
    recorder.stop();
    ui_log(std::format("Recording stopped: {} ({} frames)", out, recorder.frames_written));
}

void ds_pba::GfxContext::toggle_recording()
{
    if (recorder.is_recording())
    {
        stop_recording();
    }
    else
    {
        start_recording();
    }
}
}  // namespace ds_pba
