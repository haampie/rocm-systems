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

#pragma once

#include "core/trace_cache/metadata_types.hpp"

namespace rocprofsys
{
namespace trace_cache
{

struct metadata_parser_output_t
{
    metadata_parser_output_t()                                           = default;
    metadata_parser_output_t(const metadata_parser_output_t&)            = delete;
    metadata_parser_output_t& operator=(const metadata_parser_output_t&) = delete;
    metadata_parser_output_t(metadata_parser_output_t&&)                 = delete;
    metadata_parser_output_t& operator=(metadata_parser_output_t&&)      = delete;

    info::process            get_process_info() const;
    info::process_start_time get_process_start_time() const;
    info::process_end_time   get_process_end_time() const;

    std::vector<info::pmc>           get_pmc_info_list() const;
    std::vector<info::thread>        get_thread_info_list() const;
    std::vector<info::track>         get_track_info_list() const;
    std::vector<info::queue>         get_queue_list() const;
    std::vector<info::stream>        get_stream_list() const;
    std::vector<std::string>         get_string_list() const;
    std::vector<info::code_object>   get_code_object_list() const;
    std::vector<info::kernel_symbol> get_kernel_symbol_list() const;
    std::vector<info::agent_t>       get_agents() const;

    std::optional<info::code_object>   get_code_object(uint64_t code_object_id) const;
    std::optional<info::kernel_symbol> get_kernel_symbol(uint64_t kernel_id) const;

    void set_process(const info::process& process);
    void set_process_start_time(const info::process_start_time& process_start_time);
    void set_process_end_time(const info::process_end_time& process_end_time);
    void add_pmc_info(const info::pmc& pmc_info);
    void add_thread_info(const info::thread& thread_info);
    void add_track(const info::track& track_info);
    void add_queue(const info::queue& queue_info);
    void add_stream(const info::stream& stream_info);
    void add_string(const info::string_entry& string_entry);
    void add_agent_info(const info::agent_t& agent_info);
    void add_code_object(const info::code_object& code_object);
    void add_kernel_symbol(const info::kernel_symbol& kernel_symbol);

private:
    info::process            m_process{};
    info::process_start_time m_process_start_time{};
    info::process_end_time   m_process_end_time{};

    std::vector<info::pmc>           m_pmc_info_list{};
    std::vector<info::thread>        m_thread_info_list{};
    std::vector<info::track>         m_track_info_list{};
    std::vector<info::queue>         m_queue_list{};
    std::vector<info::stream>        m_stream_list{};
    std::vector<std::string>         m_string_entry_list{};
    std::vector<info::code_object>   m_code_object_info_list{};
    std::vector<info::kernel_symbol> m_kernel_symbol_info_list{};
    std::vector<info::agent_t>       m_agent_list{};
};

struct metadata_parser_handler_t
{
    metadata_parser_handler_t(
        const std::shared_ptr<metadata_parser_output_t>& metadata_storage,
        std::vector<std::shared_ptr<agent>>&             agents);

    void execute_sample_processing(info::metadata_identifier_t type_identifier,
                                   const cacheable_t&          value);

private:
    std::shared_ptr<metadata_parser_output_t> m_metadata_storage;
    std::vector<std::shared_ptr<agent>>&      m_agents;
};

}  // namespace trace_cache
}  // namespace rocprofsys
