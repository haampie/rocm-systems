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

#include "metadata_registry.hpp"

namespace rocprofsys
{
namespace trace_cache
{

template <typename T>
bool
metadata_registry_t::try_store_unique(
    common::synchronized<std::unordered_set<size_t>>& hash_list, const T& item)
{
    auto _hash_value = item.hash();
    bool _inserted   = false;

    hash_list.wlock([&_hash_value, &_inserted](std::unordered_set<size_t>& _data) {
        if(_data.find(_hash_value) != _data.end())
        {
            _inserted = false;
            return;
        }
        _data.emplace(_hash_value);
        _inserted = true;
    });

    if(_inserted && m_metadata_buffer.is_running())
    {
        m_metadata_buffer.store(item);
    }

    return _inserted;
}

metadata_registry_t::metadata_registry_t(std::string metadata_filename)
: m_metadata_buffer(std::move(metadata_filename))
{}

void
metadata_registry_t::set_process(const info::process& process)
{
    m_process_info.wlock([&](size_t& _process_info) { _process_info = process.hash(); });
}

void
metadata_registry_t::set_process_start_time(
    const info::process_start_time& process_start_time)
{
    m_process_start_time_info.wlock([&](size_t& _process_start_time_info) {
        _process_start_time_info = process_start_time.hash();
    });
}

void
metadata_registry_t::set_process_end_time(const info::process_end_time& process_end_time)
{
    m_process_end_time_info.wlock([&](size_t& _process_end_time_info) {
        _process_end_time_info = process_end_time.hash();
    });
}

void
metadata_registry_t::add_pmc_info(const info::pmc& pmc_info)
{
    try_store_unique(m_pmc_info_hash_list, pmc_info);
}

void
metadata_registry_t::add_thread_info(const info::thread& thread_info)
{
    try_store_unique(m_thread_info_hash_list, thread_info);
}

void
metadata_registry_t::add_track(const info::track& track_info)
{
    try_store_unique(m_track_info_hash_list, track_info);
}

void
metadata_registry_t::add_queue(const uint64_t& queue_handle)
{
    info::queue queue_info;
    queue_info.handle = queue_handle;
    try_store_unique(m_queue_info_hash_list, queue_info);
}

void
metadata_registry_t::add_stream(const uint64_t& stream_handle)
{
    info::stream stream_info;
    stream_info.handle = stream_handle;
    try_store_unique(m_stream_info_hash_list, stream_info);
}

void
metadata_registry_t::add_string(const std::string_view& string_value)
{
    info::string_entry string_info;
    string_info.value = string_value;
    try_store_unique(m_string_info_hash_list, string_info);
}

void
metadata_registry_t::add_agent_info(const info::agent_t& agent_info)
{
    try_store_unique(m_agent_info_hash_list, agent_info);
}

void
metadata_registry_t::add_code_object(const info::code_object& code_object)
{
    try_store_unique(m_code_object_info_hash_list, code_object);
}

void
metadata_registry_t::add_kernel_symbol(const info::kernel_symbol& kernel_symbol)
{
    try_store_unique(m_kernel_symbol_info_hash_list, kernel_symbol);
}

}  // namespace trace_cache
}  // namespace rocprofsys
