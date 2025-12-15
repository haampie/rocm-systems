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

#include "core/sdk_tracing_names.hpp"

#include "core/debug.hpp"

namespace rocprofsys
{
namespace rocprofiler_sdk
{
// As the underlying implementation of callback_name_info_t resizes the category
// storage during emplace, this special method is required
void
tracing_names_registry_t::overwrite_callback_names(
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
tracing_names_registry_t::get_buffer_name_info() const
{
    return m_buffered_tracing_info;
}

rocprofiler::sdk::callback_name_info_t<const char*>
tracing_names_registry_t::get_callback_tracing_info() const
{
    return m_callback_tracing_info;
}

tracing_names_registry_t::tracing_names_registry_t()
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

}  // namespace rocprofiler_sdk
}  // namespace rocprofsys
