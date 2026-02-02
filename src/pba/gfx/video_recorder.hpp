// pba/gfx/video_recorder.hpp
#pragma once

#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
//
#include <cstdio>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace ds_pba
{
// Disabled on windows
struct VideoRecorder
{
    VideoRecorder() = default;
    ~VideoRecorder();

    VideoRecorder(const VideoRecorder&) = delete;
    VideoRecorder& operator=(const VideoRecorder&) = delete;
    VideoRecorder(VideoRecorder&&) = delete;
    VideoRecorder& operator=(VideoRecorder&&) = delete;

    std::filesystem::path output_path{};
    int width{-1};
    int height{-1};
    int fps{k_video_recorder_fps};
    int frames_written{};

    FILE* pipe{nullptr};

    void stop() noexcept;
    [[nodiscard]] auto start(std::filesystem::path output_path) -> bool;
    [[nodiscard]] auto write_frame(std::span<const u8> rgba) -> bool;
    [[nodiscard]] auto is_recording() const noexcept -> bool;
    [[nodiscard]] static auto quote_arg(std::string_view s) -> std::string;
    [[nodiscard]] static auto open_pipe_write(const std::string& cmd) -> FILE*;
    [[nodiscard]] static auto close_pipe(FILE* pipe) noexcept -> int;
};

}  // namespace ds_pba
