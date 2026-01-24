// pba/util/shutdown.hpp
#pragma once

#include <atomic>
#include <csignal>

namespace ds_pba
{
// For development and showcases I use the watcher.sh script which uses
// watchexec to (gently) restart the program whenever some file gets saved.
//
// That is done by sending a SIGTERM signal which is an ignorable terminal
// signal. Indirectly when the watcher gets closed (for example by <CTRL+C>)
// then it sends a SIGINT signal which also gets processed by the program.
//
// Earlier iterations used SIGKILL and didn't need to explicitly handle
// the signal but that is unstable when handling IO requests.
//
// As per the C++ standard calling arbitrary functions (e.g. GLFW, OpenGL library
// functions) is UB, see https://en.cppreference.com/w/cpp/utility/program/signal
//
// So I use a standard workaround by setting a signal-safe atomic flag
// (g_request_close_sig) and read that, storing its value into g_request_close
// which then invokes the actual shutdown. (Probably overkill for our safety reqs.)
inline std::atomic_bool g_request_close{false};
inline volatile std::sig_atomic_t g_request_close_sig{0};
}  // namespace ds_pba
