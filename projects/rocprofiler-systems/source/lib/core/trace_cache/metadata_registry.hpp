// MIT License
//
// Copyright (c) 2025 Advanced Micro Devices, Inc. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "common/synchronized.hpp"
#include "core/categories.hpp"
#include "core/trace_cache/buffer_storage.hpp"
#include "core/trace_cache/metadata_types.hpp"

#include <cstdint>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>

namespace rocprofsys
{
namespace trace_cache
{
namespace info
{

template <typename Category>
inline std::string
annotate_with_device_id(uint32_t           device_id,
                        std::optional<int> first_section  = std::nullopt,
                        std::optional<int> second_section = std::nullopt)
{
    std::stringstream ss;
    ss << std::string(tim::trait::name<Category>::value) + " [" +
              std::to_string(device_id) + "]";
    if(first_section) ss << "_" << std::to_string(*first_section);
    if(second_section) ss << "_" << std::to_string(*second_section);
    return ss.str();
}

template <typename Category>
inline std::string
annotate_category(std::optional<int> first_section  = std::nullopt,
                  std::optional<int> second_section = std::nullopt)
{
    std::stringstream ss;
    ss << std::string(tim::trait::name<Category>::value);
    if(first_section) ss << "_" << std::to_string(*first_section);
    if(second_section) ss << "_" << std::to_string(*second_section);
    return ss.str();
}

}  // namespace info

struct metadata_registry_t
{
    metadata_registry_t(std::string metadata_filename);
    metadata_registry_t(const metadata_registry_t&)            = delete;
    metadata_registry_t& operator=(const metadata_registry_t&) = delete;
    metadata_registry_t(metadata_registry_t&&)                 = delete;
    metadata_registry_t& operator=(metadata_registry_t&&)      = delete;

    void set_process(const info::process& process);
    void set_process_start_time(const info::process_start_time& process_start_time);
    void set_process_end_time(const info::process_end_time& process_end_time);
    void add_pmc_info(const info::pmc& pmc_info);
    void add_thread_info(const info::thread& thread_info);
    void add_track(const info::track& track_info);
    void add_queue(const uint64_t& queue_handle);
    void add_stream(const uint64_t& stream_handle);
    void add_string(const std::string_view& string_value);
    void add_agent_info(const info::agent_t& agent_info);

    void add_code_object(const info::code_object& code_object);
    void add_kernel_symbol(const info::kernel_symbol& kernel_symbol);

    void start(const int32_t& current_pid) { m_metadata_buffer.start(current_pid); }
    void shutdown() { m_metadata_buffer.shutdown(); }

private:
    template <typename T>
    bool try_store_unique(common::synchronized<std::unordered_set<size_t>>& hash_list,
                          const T&                                          item);

    buffer_storage<mmap_flush_worker_factory_t<>, info::metadata_identifier_t,
                   metadata_buffer_size, metadata_flush_threshold>
        m_metadata_buffer;

    common::synchronized<size_t> m_process_info{};
    common::synchronized<size_t> m_process_start_time_info{};
    common::synchronized<size_t> m_process_end_time_info{};

    common::synchronized<std::unordered_set<size_t>> m_pmc_info_hash_list{};
    common::synchronized<std::unordered_set<size_t>> m_thread_info_hash_list{};
    common::synchronized<std::unordered_set<size_t>> m_track_info_hash_list{};
    common::synchronized<std::unordered_set<size_t>> m_queue_info_hash_list{};
    common::synchronized<std::unordered_set<size_t>> m_stream_info_hash_list{};
    common::synchronized<std::unordered_set<size_t>> m_string_info_hash_list{};
    common::synchronized<std::unordered_set<size_t>> m_code_object_info_hash_list{};
    common::synchronized<std::unordered_set<size_t>> m_kernel_symbol_info_hash_list{};
    common::synchronized<std::unordered_set<size_t>> m_agent_info_hash_list{};
};

}  // namespace trace_cache
}  // namespace rocprofsys
