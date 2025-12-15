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

#if ROCPROFSYS_USE_ROCM > 0
#    include <map>
#    include <rocprofiler-sdk/callback_tracing.h>
#    include <rocprofiler-sdk/cxx/name_info.hpp>

namespace rocprofsys
{

namespace rocprofiler_sdk
{

struct tracing_names_registry_t
{
    tracing_names_registry_t();
    tracing_names_registry_t(const tracing_names_registry_t&) = delete;

    rocprofiler::sdk::buffer_name_info_t<const char*>   get_buffer_name_info() const;
    rocprofiler::sdk::callback_name_info_t<const char*> get_callback_tracing_info() const;

private:
    rocprofiler::sdk::buffer_name_info_t<const char*> m_buffered_tracing_info{
        rocprofiler::sdk::get_buffer_tracing_names<const char*>()
    };
    rocprofiler::sdk::callback_name_info_t<const char*> m_callback_tracing_info{
        rocprofiler::sdk::get_callback_tracing_names<const char*>()
    };
    using callback_rename_map_t =
        std::map<rocprofiler_tracing_operation_t, std::string_view>;

    void overwrite_callback_names(
        std::initializer_list<
            std::pair<rocprofiler_callback_tracing_kind_t, callback_rename_map_t>>
            rename_table);
};

inline tracing_names_registry_t&
get_tracing_names_registry()
{
    static tracing_names_registry_t instance;
    return instance;
}

#endif

}  // namespace rocprofiler_sdk

}  // namespace rocprofsys
