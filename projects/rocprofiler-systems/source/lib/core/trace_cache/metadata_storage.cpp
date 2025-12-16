// MIT License
//
// Copyright (c) 2022 Advanced Micro Devices, Inc. All Rights Reserved.
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

#include "core/trace_cache/metadata_storage.hpp"

#include <stdexcept>

namespace rocprofsys
{
namespace trace_cache
{

//

info::process
metadata_parser_output_t::get_process_info() const
{
    return m_process;
}

info::process_start_time
metadata_parser_output_t::get_process_start_time() const
{
    return m_process_start_time;
}

info::process_end_time
metadata_parser_output_t::get_process_end_time() const
{
    return m_process_end_time;
}

std::vector<info::pmc>
metadata_parser_output_t::get_pmc_info_list() const
{
    return m_pmc_info_list;
}

std::vector<info::thread>
metadata_parser_output_t::get_thread_info_list() const
{
    return m_thread_info_list;
}

std::vector<info::track>
metadata_parser_output_t::get_track_info_list() const
{
    return m_track_info_list;
}

std::vector<info::queue>
metadata_parser_output_t::get_queue_list() const
{
    return m_queue_list;
}

std::vector<info::stream>
metadata_parser_output_t::get_stream_list() const
{
    return m_stream_list;
}

std::vector<std::string>
metadata_parser_output_t::get_string_list() const
{
    return m_string_entry_list;
}

std::vector<info::code_object>
metadata_parser_output_t::get_code_object_list() const
{
    return m_code_object_info_list;
}

std::vector<info::kernel_symbol>
metadata_parser_output_t::get_kernel_symbol_list() const
{
    return m_kernel_symbol_info_list;
}

std::vector<info::agent_t>
metadata_parser_output_t::get_agents() const
{
    return m_agent_list;
}

std::optional<info::code_object>
metadata_parser_output_t::get_code_object(uint64_t code_object_id) const
{
    auto it = std::find_if(
        m_code_object_info_list.begin(), m_code_object_info_list.end(),
        [&](const info::code_object& co) { return co.code_object_id == code_object_id; });
    return it != m_code_object_info_list.end() ? std::optional<info::code_object>(*it)
                                               : std::nullopt;
}

std::optional<info::kernel_symbol>
metadata_parser_output_t::get_kernel_symbol(uint64_t kernel_id) const
{
    auto it = std::find_if(
        m_kernel_symbol_info_list.begin(), m_kernel_symbol_info_list.end(),
        [&](const info::kernel_symbol& ks) { return ks.kernel_id == kernel_id; });
    return it != m_kernel_symbol_info_list.end() ? std::optional<info::kernel_symbol>(*it)
                                                 : std::nullopt;
}

void
metadata_parser_output_t::set_process(const info::process& process)
{
    m_process = process;
}
void
metadata_parser_output_t::set_process_start_time(
    const info::process_start_time& process_start_time)
{
    m_process_start_time = process_start_time;
}
void
metadata_parser_output_t::set_process_end_time(
    const info::process_end_time& process_end_time)
{
    m_process_end_time = process_end_time;
}

void
metadata_parser_output_t::add_pmc_info(const info::pmc& pmc_info)
{
    m_pmc_info_list.push_back(pmc_info);
}

void
metadata_parser_output_t::add_thread_info(const info::thread& thread_info)
{
    m_thread_info_list.push_back(thread_info);
}

void
metadata_parser_output_t::add_track(const info::track& track_info)
{
    m_track_info_list.push_back(track_info);
}

void
metadata_parser_output_t::add_queue(const info::queue& queue_info)
{
    m_queue_list.push_back(queue_info);
}

void
metadata_parser_output_t::add_stream(const info::stream& stream_info)
{
    m_stream_list.push_back(stream_info);
}

void
metadata_parser_output_t::add_string(const info::string_entry& string_entry)
{
    m_string_entry_list.emplace_back(string_entry.value);
}

void
metadata_parser_output_t::add_agent_info(const info::agent_t& agent_info)
{
    m_agent_list.push_back(agent_info);
}

void
metadata_parser_output_t::add_code_object(const info::code_object& code_object)
{
    m_code_object_info_list.push_back(code_object);
}

void
metadata_parser_output_t::add_kernel_symbol(const info::kernel_symbol& kernel_symbol)
{
    m_kernel_symbol_info_list.push_back(kernel_symbol);
}

metadata_parser_handler_t::metadata_parser_handler_t(
    const std::shared_ptr<metadata_parser_output_t>& metadata_storage,
    std::vector<std::shared_ptr<agent>>&             agents)
: m_metadata_storage(metadata_storage)
, m_agents(agents)
{}

void
metadata_parser_handler_t::execute_sample_processing(
    info::metadata_identifier_t type_identifier, const cacheable_t& value)
{
    switch(type_identifier)
    {
        case info::metadata_identifier_t::process:
        {
            m_metadata_storage->set_process(static_cast<const info::process&>(value));
            break;
        }
        case info::metadata_identifier_t::process_start_time:
        {
            m_metadata_storage->set_process_start_time(
                static_cast<const info::process_start_time&>(value));
            break;
        }
        case info::metadata_identifier_t::process_end_time:
        {
            m_metadata_storage->set_process_end_time(
                static_cast<const info::process_end_time&>(value));
            break;
        }
        case info::metadata_identifier_t::pmc:
        {
            m_metadata_storage->add_pmc_info(static_cast<const info::pmc&>(value));
            break;
        }
        case info::metadata_identifier_t::thread:
        {
            m_metadata_storage->add_thread_info(static_cast<const info::thread&>(value));
            break;
        }
        case info::metadata_identifier_t::track:
        {
            m_metadata_storage->add_track(static_cast<const info::track&>(value));
            break;
        }
        case info::metadata_identifier_t::queue:
        {
            m_metadata_storage->add_queue(static_cast<const info::queue&>(value));
            break;
        }
        case info::metadata_identifier_t::stream:
        {
            m_metadata_storage->add_stream(static_cast<const info::stream&>(value));
            break;
        }
        case info::metadata_identifier_t::string:
        {
            m_metadata_storage->add_string(static_cast<const info::string_entry&>(value));
            break;
        }
        case info::metadata_identifier_t::code_object:
        {
            m_metadata_storage->add_code_object(
                static_cast<const info::code_object&>(value));
            break;
        }
        case info::metadata_identifier_t::kernel_symbol:
        {
            m_metadata_storage->add_kernel_symbol(
                static_cast<const info::kernel_symbol&>(value));
            break;
        }
        case info::metadata_identifier_t::agent:
        {
            m_agents.push_back(static_cast<const info::agent_t&>(value).agent_ptr);
            break;
        }
        default: throw std::runtime_error("Unsupported metadata type");
    }
}

}  // namespace trace_cache
}  // namespace rocprofsys
