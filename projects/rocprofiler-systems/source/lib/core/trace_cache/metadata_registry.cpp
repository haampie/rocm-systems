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
#include "core/debug.hpp"
#include <cstdint>

namespace rocprofsys
{
namespace trace_cache
{
template <typename T>
bool
metadata_registry_t::try_store_unique(const T& item)
{
    auto _hash_value = item.hash();
    bool _inserted   = false;

    m_unique_objects.wlock([&_hash_value, &_inserted](std::unordered_set<size_t>& _data) {
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
    try_store_unique(process);
}

void
metadata_registry_t::set_process_start_time(
    const info::process_start_time& process_start_time)
{
    try_store_unique(process_start_time);
}

void
metadata_registry_t::set_process_end_time(const info::process_end_time& process_end_time)
{
    try_store_unique(process_end_time);
}

void
metadata_registry_t::add_pmc_info(const info::pmc& pmc_info)
{
    try_store_unique(pmc_info);
}

void
metadata_registry_t::add_thread_info(const info::thread& thread_info)
{
    try_store_unique(thread_info);
}

void
metadata_registry_t::add_track(const info::track& track_info)
{
    try_store_unique(track_info);
}

void
metadata_registry_t::add_queue(const uint64_t& queue_handle)
{
    info::queue queue_info;
    queue_info.handle = queue_handle;
    try_store_unique(queue_info);
}

void
metadata_registry_t::add_stream(const uint64_t& stream_handle)
{
    info::stream stream_info;
    stream_info.handle = stream_handle;
    try_store_unique(stream_info);
}

void
metadata_registry_t::add_string(const std::string_view& string_value)
{
    info::string_entry string_info;
    string_info.value = string_value;
    try_store_unique(string_info);
}

void
metadata_registry_t::add_agent_info(const info::agent_t& agent_info)
{
    try_store_unique(agent_info);
}

#if ROCPROFSYS_USE_ROCM > 0

void
metadata_registry_t::add_code_object(
    const rocprofiler_callback_tracing_code_object_load_data_t& code_object)
{
    info::code_object co_info;
    co_info.code_object_id = code_object.code_object_id;
    co_info.uri            = code_object.uri ? std::string(code_object.uri) : "";
    co_info.load_base      = code_object.load_base;
    co_info.load_size      = code_object.load_size;
    co_info.load_delta     = code_object.load_delta;
    co_info.storage_type   = static_cast<int32_t>(code_object.storage_type);
#    if(ROCPROFILER_VERSION >= 600)
    co_info.agent_id_handle = code_object.agent_id.handle;
#    else
    co_info.agent_id_handle = code_object.rocp_agent.handle;
#    endif
    try_store_unique(co_info);
}

void
metadata_registry_t::add_kernel_symbol(
    const rocprofiler_callback_tracing_code_object_kernel_symbol_register_data_t&
        kernel_symbol)
{
    info::kernel_symbol ks_info;
    ks_info.kernel_id      = kernel_symbol.kernel_id;
    ks_info.code_object_id = kernel_symbol.code_object_id;
    ks_info.kernel_name =
        kernel_symbol.kernel_name ? std::string(kernel_symbol.kernel_name) : "";
    ks_info.kernel_object             = kernel_symbol.kernel_object;
    ks_info.kernarg_segment_size      = kernel_symbol.kernarg_segment_size;
    ks_info.kernarg_segment_alignment = kernel_symbol.kernarg_segment_alignment;
    ks_info.group_segment_size        = kernel_symbol.group_segment_size;
    try_store_unique(ks_info);
}

// As the underlying implementation of callback_name_info_t resizes the category
// storage during emplace, this special method is required
void
metadata_storage_t::overwrite_callback_names(
    std::initializer_list<
        std::pair<rocprofiler_callback_tracing_kind_t, callback_rename_map_t>>
        rename_table)
{
    if(rename_table.size() == 0) return;

    using callback_kind_t   = rocprofiler_callback_tracing_kind_t;
    using operation_names_t = std::vector<std::string_view>;

    auto category_names = std::vector<std::string_view>{};
    auto modified_ops   = std::map<callback_kind_t, operation_names_t>{};

    auto extract_operations = [&](callback_kind_t cat) -> operation_names_t {
        auto        items           = m_callback_tracing_info.items();
        const auto* target_category = items[static_cast<size_t>(cat)];

        auto              operations_data = target_category->items();
        operation_names_t operation_names;
        operation_names.reserve(operations_data.size());

        for(const auto& [op_idx, op_name] : operations_data)
            operation_names.push_back(*op_name);

        return operation_names;
    };

    // Store category names
    category_names.resize(ROCPROFILER_CALLBACK_TRACING_LAST);
    for(callback_kind_t i = ROCPROFILER_CALLBACK_TRACING_NONE;
        i < ROCPROFILER_CALLBACK_TRACING_LAST;
        i = static_cast<callback_kind_t>(static_cast<int>(i) + 1))
    {
        category_names[i] = m_callback_tracing_info.at(i);
    }

    // Process list
    for(const auto& category_info : rename_table)
    {
        auto callback_kind = category_info.first;
        // Store operations of all following categories
        //  as they will be deleted
        for(callback_kind_t i =
                static_cast<callback_kind_t>(static_cast<int>(callback_kind) + 1);
            i < ROCPROFILER_CALLBACK_TRACING_LAST;
            i = static_cast<callback_kind_t>(static_cast<int>(i) + 1))
        {
            if(modified_ops.find(i) != modified_ops.end()) break;
            modified_ops[i] = extract_operations(i);
        }

        ROCPROFSYS_CI_THROW(modified_ops.find(callback_kind) != modified_ops.end(),
                            "Overwriting a previously overwritten entry is forbidden");

        ROCPROFSYS_CI_THROW(!modified_ops.empty() &&
                                callback_kind >= modified_ops.begin()->first,
                            "Category must have a larger enum value than all previously "
                            "modified_ops categories");

        // Overwrite desired category
        auto operation_names = extract_operations(callback_kind);
        for(const auto& [index, new_value] : category_info.second)
        {
            ROCPROFSYS_CI_THROW(index < 0 ||
                                    static_cast<size_t>(index) >= operation_names.size(),
                                "Index is invalid");
            operation_names[index] = new_value;
        }
        modified_ops[callback_kind] = std::move(operation_names);
    }
    if(modified_ops.empty()) return;

    // Emplace the changed category operations
    for(callback_kind_t i = modified_ops.begin()->first;
        i < ROCPROFILER_CALLBACK_TRACING_LAST;
        i = static_cast<callback_kind_t>(static_cast<int>(i) + 1))
    {
        auto renaming_entry = modified_ops.find(i);

        ROCPROFSYS_CI_THROW(renaming_entry == modified_ops.end(),
                            "A category that needs to be emplaced is missing");

        const auto& operations_vec = renaming_entry->second;
        m_callback_tracing_info.emplace(i, category_names.at(i).data());
        for(size_t op_idx = 0; op_idx < operations_vec.size(); ++op_idx)
        {
            m_callback_tracing_info.emplace(
                i, static_cast<rocprofiler_tracing_operation_t>(op_idx),
                operations_vec[op_idx].data());
        }
    }
}

rocprofiler::sdk::buffer_name_info_t<const char*>
metadata_storage_t::get_buffer_name_info() const
{
    return m_buffered_tracing_info;
}

rocprofiler::sdk::callback_name_info_t<const char*>
metadata_storage_t::get_callback_tracing_info() const
{
    return m_callback_tracing_info;
}

#endif

metadata_storage_t::metadata_storage_t()
{
#if ROCPROFSYS_USE_ROCM > 0
    overwrite_callback_names({
#    if(ROCPROFILER_VERSION >= 600)
        { ROCPROFILER_CALLBACK_TRACING_OMPT,
          { { ROCPROFILER_OMPT_ID_parallel_begin, "omp_parallel" },
            { ROCPROFILER_OMPT_ID_parallel_end, "omp_parallel" } } }
#    endif
    });
#endif
}

}  // namespace trace_cache
}  // namespace rocprofsys
