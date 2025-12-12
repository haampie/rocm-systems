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

#include "core/agent.hpp"
#include "core/categories.hpp"
#include "core/trace_cache/buffer_storage.hpp"
#include "core/trace_cache/cacheable.hpp"

#if ROCPROFSYS_USE_ROCM > 0
#    include <rocprofiler-sdk/callback_tracing.h>
#    include <rocprofiler-sdk/cxx/name_info.hpp>
#endif

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <map>
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

enum class metadata_identifier_t : uint8_t
{
    process            = 0x00,
    pmc                = 0x01,
    thread             = 0x02,
    track              = 0x03,
    queue              = 0x04,
    stream             = 0x05,
    string             = 0x06,
    code_object        = 0x07,
    kernel_symbol      = 0x08,
    agent              = 0x09,
    process_start_time = 0x0A,
    process_end_time   = 0x0B,
    fragmented_space   = 0xFF,
};

inline size_t
hash_combine(size_t seed, size_t value)
{
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

template <typename T>
inline size_t
make_type_hash(metadata_identifier_t type_id, T&& value)
{
    size_t seed = static_cast<size_t>(type_id) * 0x9e3779b97f4a7c15ULL;
    return hash_combine(seed, std::hash<std::decay_t<T>>{}(std::forward<T>(value)));
}

struct process : cacheable_t
{
    static constexpr metadata_identifier_t type_identifier =
        metadata_identifier_t::process;

    process() = default;
    process(int32_t _pid, int32_t _ppid, std::string _command)
    : pid(_pid)
    , ppid(_ppid)
    , command(std::move(_command))
    {}

    int32_t     pid;
    int32_t     ppid;
    std::string command;

    size_t hash() const noexcept { return make_type_hash(type_identifier, pid); }
};

struct process_start_time : cacheable_t
{
    static constexpr metadata_identifier_t type_identifier =
        metadata_identifier_t::process_start_time;

    process_start_time() = default;
    process_start_time(int64_t _start)
    : start(_start)
    {}

    int64_t start;

    size_t hash() const noexcept { return make_type_hash(type_identifier, start); }
};

struct process_end_time : cacheable_t
{
    static constexpr metadata_identifier_t type_identifier =
        metadata_identifier_t::process_end_time;

    process_end_time() = default;
    process_end_time(int64_t _end)
    : end(_end)
    {}

    int64_t end;

    size_t hash() const noexcept { return make_type_hash(type_identifier, end); }
};

struct pmc : cacheable_t
{
    static constexpr metadata_identifier_t type_identifier = metadata_identifier_t::pmc;

    pmc() = default;

    pmc(agent_type _type, size_t _agent_type_index, std::string _target_arch,
        size_t _event_code, size_t _instance_id, std::string _name, std::string _symbol,
        std::string _description, std::string _long_description, std::string _component,
        std::string _units, std::string _value_type, std::string _block,
        std::string _expression, uint32_t _is_constant, uint32_t _is_derived,
        std::string _extdata = "{}")
    : type(_type)
    , agent_type_index(_agent_type_index)
    , target_arch(std::move(_target_arch))
    , event_code(_event_code)
    , instance_id(_instance_id)
    , name(std::move(_name))
    , symbol(std::move(_symbol))
    , description(std::move(_description))
    , long_description(std::move(_long_description))
    , component(std::move(_component))
    , units(std::move(_units))
    , value_type(std::move(_value_type))
    , block(std::move(_block))
    , expression(std::move(_expression))
    , is_constant(_is_constant)
    , is_derived(_is_derived)
    , extdata(std::move(_extdata))
    {}

    agent_type  type;
    size_t      agent_type_index;
    std::string target_arch;
    size_t      event_code;
    size_t      instance_id;
    std::string name;
    std::string symbol;
    std::string description;
    std::string long_description;
    std::string component;
    std::string units;
    std::string value_type;
    std::string block;
    std::string expression;
    uint32_t    is_constant;
    uint32_t    is_derived;
    std::string extdata;

    size_t hash() const noexcept
    {
        size_t seed = static_cast<size_t>(type_identifier) * 0x9e3779b97f4a7c15ULL;
        seed        = hash_combine(seed, static_cast<size_t>(type));
        seed        = hash_combine(seed, agent_type_index);
        seed        = hash_combine(seed, std::hash<std::string>{}(name));
        return seed;
    }
};

struct thread : cacheable_t
{
    static constexpr metadata_identifier_t type_identifier =
        metadata_identifier_t::thread;

    thread() = default;
    thread(int32_t _parent_process_id, int32_t _process_id, uint64_t _thread_id,
           uint32_t _start, uint32_t _end, std::string _extdata)
    : parent_process_id(_parent_process_id)
    , process_id(_process_id)
    , thread_id(_thread_id)
    , start(_start)
    , end(_end)
    , extdata(std::move(_extdata))
    {}

    int32_t     parent_process_id;
    int32_t     process_id;
    uint64_t    thread_id;
    uint32_t    start;
    uint32_t    end;
    std::string extdata;

    size_t hash() const noexcept { return make_type_hash(type_identifier, thread_id); }
};

struct track : cacheable_t
{
    static constexpr metadata_identifier_t type_identifier = metadata_identifier_t::track;

    track() = default;
    track(std::string _track_name, std::optional<size_t> _thread_id, std::string _extdata)
    : track_name(std::move(_track_name))
    , thread_id(_thread_id)
    , extdata(std::move(_extdata))
    {}

    std::string           track_name;
    std::optional<size_t> thread_id;
    std::string           extdata;

    size_t hash() const noexcept { return make_type_hash(type_identifier, track_name); }
};

struct queue : cacheable_t
{
    static constexpr metadata_identifier_t type_identifier = metadata_identifier_t::queue;

    queue() = default;
    queue(uint64_t _handle)
    : handle(_handle)
    {}

    uint64_t handle;

    size_t hash() const noexcept { return make_type_hash(type_identifier, handle); }
};

struct stream : cacheable_t
{
    static constexpr metadata_identifier_t type_identifier =
        metadata_identifier_t::stream;

    stream() = default;
    stream(uint64_t _handle)
    : handle(_handle)
    {}

    uint64_t handle;

    size_t hash() const noexcept { return make_type_hash(type_identifier, handle); }
};

struct string_entry : cacheable_t
{
    static constexpr metadata_identifier_t type_identifier =
        metadata_identifier_t::string;

    string_entry() = default;
    string_entry(std::string_view _value)
    : value(_value)
    {}

    std::string_view value;

    size_t hash() const noexcept { return make_type_hash(type_identifier, value); }
};

struct code_object : cacheable_t
{
    static constexpr metadata_identifier_t type_identifier =
        metadata_identifier_t::code_object;

    code_object() = default;
    code_object(uint64_t _code_object_id, std::string _uri, uint64_t _load_base,
                uint64_t _load_size, int64_t _load_delta, int32_t _storage_type,
                uint64_t _agent_id_handle)
    : code_object_id(_code_object_id)
    , uri(std::move(_uri))
    , load_base(_load_base)
    , load_size(_load_size)
    , load_delta(_load_delta)
    , storage_type(_storage_type)
    , agent_id_handle(_agent_id_handle)
    {}

    uint64_t    code_object_id;
    std::string uri;
    uint64_t    load_base;
    uint64_t    load_size;
    int64_t     load_delta;
    int32_t     storage_type;
    uint64_t    agent_id_handle;

    size_t hash() const noexcept
    {
        return make_type_hash(type_identifier, code_object_id);
    }
};

struct kernel_symbol : cacheable_t
{
    static constexpr metadata_identifier_t type_identifier =
        metadata_identifier_t::kernel_symbol;

    kernel_symbol() = default;
    kernel_symbol(uint64_t _kernel_id, uint64_t _code_object_id, std::string _kernel_name,
                  uint64_t _kernel_object, uint32_t _kernarg_segment_size,
                  uint32_t _kernarg_segment_alignment, uint32_t _group_segment_size,
                  uint32_t _private_segment_size, uint32_t _sgpr_count,
                  uint32_t _arch_vgpr_count, uint32_t _accum_vgpr_count)
    : kernel_id(_kernel_id)
    , code_object_id(_code_object_id)
    , kernel_name(std::move(_kernel_name))
    , kernel_object(_kernel_object)
    , kernarg_segment_size(_kernarg_segment_size)
    , kernarg_segment_alignment(_kernarg_segment_alignment)
    , group_segment_size(_group_segment_size)
    , private_segment_size(_private_segment_size)
    , sgpr_count(_sgpr_count)
    , arch_vgpr_count(_arch_vgpr_count)
    , accum_vgpr_count(_accum_vgpr_count)
    {}

    uint64_t    kernel_id;
    uint64_t    code_object_id;
    std::string kernel_name;
    uint64_t    kernel_object;
    uint32_t    kernarg_segment_size;
    uint32_t    kernarg_segment_alignment;
    uint32_t    group_segment_size;
    uint32_t    private_segment_size;
    uint32_t    sgpr_count;
    uint32_t    arch_vgpr_count;
    uint32_t    accum_vgpr_count;

    size_t hash() const noexcept { return make_type_hash(type_identifier, kernel_id); }
};

struct agent_t : cacheable_t
{
    static constexpr metadata_identifier_t type_identifier = metadata_identifier_t::agent;

    agent_t() = default;
    agent_t(const std::shared_ptr<struct rocprofsys::agent>& _agent)
    : agent_ptr(_agent)
    {}

    std::shared_ptr<struct rocprofsys::agent> agent_ptr;

    size_t hash() const noexcept
    {
        size_t seed = make_type_hash(type_identifier, agent_ptr->device_id);
        seed = hash_combine(seed, static_cast<size_t>(agent_ptr->device_type_index));
        seed = hash_combine(seed, static_cast<size_t>(agent_ptr->type));
        return seed;
    }
};

}  // namespace info

template <>
inline void
serialize(uint8_t* buffer, const info::process& item)
{
    utility::store_value(buffer, item.pid, item.ppid, std::string_view(item.command));
}

template <>
inline info::process
deserialize(uint8_t*& buffer)
{
    info::process    item;
    std::string_view command_view;
    utility::parse_value(buffer, item.pid, item.ppid, command_view);
    item.command = std::string(command_view);
    return item;
}

template <>
size_t inline get_size(const info::process& item)
{
    return utility::get_size(item.pid, item.ppid, std::string_view(item.command));
}

template <>
void inline serialize(uint8_t* buffer, const info::process_start_time& item)
{
    utility::store_value(buffer, item.start);
}

template <>
inline info::process_start_time
deserialize(uint8_t*& buffer)
{
    info::process_start_time item;
    utility::parse_value(buffer, item.start);
    return item;
}

template <>
size_t inline get_size(const info::process_start_time& item)
{
    return utility::get_size(item.start);
}

template <>
void inline serialize(uint8_t* buffer, const info::process_end_time& item)
{
    utility::store_value(buffer, item.end);
}

template <>
inline info::process_end_time
deserialize(uint8_t*& buffer)
{
    info::process_end_time item;
    utility::parse_value(buffer, item.end);
    return item;
}

template <>
size_t inline get_size(const info::process_end_time& item)
{
    return utility::get_size(item.end);
}

template <>
inline void
serialize(uint8_t* buffer, const info::pmc& item)
{
    utility::store_value(
        buffer, static_cast<uint8_t>(item.type), item.agent_type_index,
        std::string_view(item.target_arch), item.event_code, item.instance_id,
        std::string_view(item.name), std::string_view(item.symbol),
        std::string_view(item.description), std::string_view(item.long_description),
        std::string_view(item.component), std::string_view(item.units),
        std::string_view(item.value_type), std::string_view(item.block),
        std::string_view(item.expression), item.is_constant, item.is_derived,
        std::string_view(item.extdata));
}

template <>
info::pmc inline deserialize(uint8_t*& buffer)
{
    info::pmc        item;
    std::string_view target_arch_view, name_view, symbol_view, description_view,
        long_description_view, component_view, units_view, value_type_view, block_view,
        expression_view, extdata_view;
    uint8_t type_val;
    utility::parse_value(buffer, type_val, item.agent_type_index, target_arch_view,
                         item.event_code, item.instance_id, name_view, symbol_view,
                         description_view, long_description_view, component_view,
                         units_view, value_type_view, block_view, expression_view,
                         item.is_constant, item.is_derived, extdata_view);

    item.type             = static_cast<agent_type>(type_val);
    item.target_arch      = std::string(target_arch_view);
    item.name             = std::string(name_view);
    item.symbol           = std::string(symbol_view);
    item.description      = std::string(description_view);
    item.long_description = std::string(long_description_view);
    item.component        = std::string(component_view);
    item.units            = std::string(units_view);
    item.value_type       = std::string(value_type_view);
    item.block            = std::string(block_view);
    item.expression       = std::string(expression_view);
    item.extdata          = std::string(extdata_view);
    return item;
}

template <>
size_t inline get_size(const info::pmc& item)
{
    return utility::get_size(
        static_cast<uint8_t>(item.type), item.agent_type_index,
        std::string_view(item.target_arch), item.event_code, item.instance_id,
        std::string_view(item.name), std::string_view(item.symbol),
        std::string_view(item.description), std::string_view(item.long_description),
        std::string_view(item.component), std::string_view(item.units),
        std::string_view(item.value_type), std::string_view(item.block),
        std::string_view(item.expression), item.is_constant, item.is_derived,
        std::string_view(item.extdata));
}

template <>
void inline serialize(uint8_t* buffer, const info::thread& item)
{
    utility::store_value(buffer, item.parent_process_id, item.process_id, item.thread_id,
                         item.start, item.end, std::string_view(item.extdata));
}

template <>
info::thread inline deserialize(uint8_t*& buffer)
{
    info::thread     item;
    std::string_view extdata_view;
    utility::parse_value(buffer, item.parent_process_id, item.process_id, item.thread_id,
                         item.start, item.end, extdata_view);
    item.extdata = std::string(extdata_view);
    return item;
}

template <>
size_t inline get_size(const info::thread& item)
{
    return utility::get_size(item.parent_process_id, item.process_id, item.thread_id,
                             item.start, item.end, std::string_view(item.extdata));
}

template <>
void inline serialize(uint8_t* buffer, const info::track& item)
{
    const size_t thread_id_val = item.thread_id.has_value()
                                     ? static_cast<uint64_t>(item.thread_id.value())
                                     : std::numeric_limits<uint64_t>::max();

    utility::store_value(buffer, std::string_view(item.track_name), thread_id_val,
                         std::string_view(item.extdata));
}

template <>
info::track inline deserialize(uint8_t*& buffer)
{
    info::track      item;
    std::string_view track_name_view, extdata_view;
    size_t           thread_id_val;

    utility::parse_value(buffer, track_name_view, thread_id_val, extdata_view);

    item.track_name = std::string(track_name_view);
    item.thread_id  = thread_id_val != std::numeric_limits<size_t>::max()
                          ? std::make_optional(thread_id_val)
                          : std::nullopt;

    item.extdata = std::string(extdata_view);

    return item;
}

template <>
size_t inline get_size(const info::track& item)
{
    const size_t thread_id_val = item.thread_id.has_value()
                                     ? item.thread_id.value()
                                     : std::numeric_limits<size_t>::max();

    return utility::get_size(std::string_view(item.track_name), thread_id_val,
                             std::string_view(item.extdata));
}

template <>
void inline serialize(uint8_t* buffer, const info::queue& item)
{
    utility::store_value(buffer, item.handle);
}

template <>
info::queue inline deserialize(uint8_t*& buffer)
{
    info::queue item;
    utility::parse_value(buffer, item.handle);
    return item;
}

template <>
size_t inline get_size(const info::queue& item)
{
    return utility::get_size(item.handle);
}

template <>
void inline serialize(uint8_t* buffer, const info::stream& item)
{
    utility::store_value(buffer, item.handle);
}

template <>
info::stream inline deserialize(uint8_t*& buffer)
{
    info::stream item;
    utility::parse_value(buffer, item.handle);
    return item;
}

template <>
size_t inline get_size(const info::stream& item)
{
    return utility::get_size(item.handle);
}

template <>
void inline serialize(uint8_t* buffer, const info::string_entry& item)
{
    utility::store_value(buffer, item.value);
}

template <>
info::string_entry inline deserialize(uint8_t*& buffer)
{
    info::string_entry item;
    std::string_view   value_view;
    utility::parse_value(buffer, value_view);
    item.value = std::string(value_view);
    return item;
}

template <>
size_t inline get_size(const info::string_entry& item)
{
    return utility::get_size(std::string_view(item.value));
}

#if ROCPROFSYS_USE_ROCM > 0
template <>
void inline serialize(uint8_t* buffer, const info::code_object& item)
{
    utility::store_value(buffer, item.code_object_id, std::string_view(item.uri),
                         item.load_base, item.load_size, item.load_delta,
                         item.storage_type, item.agent_id_handle);
}

template <>
info::code_object inline deserialize(uint8_t*& buffer)
{
    info::code_object item;
    std::string_view  uri_view;
    utility::parse_value(buffer, item.code_object_id, uri_view, item.load_base,
                         item.load_size, item.load_delta, item.storage_type,
                         item.agent_id_handle);
    item.uri = std::string(uri_view);
    return item;
}

template <>
size_t inline get_size(const info::code_object& item)
{
    return utility::get_size(item.code_object_id, std::string_view(item.uri),
                             item.load_base, item.load_size, item.load_delta,
                             item.storage_type, item.agent_id_handle);
}

template <>
void inline serialize(uint8_t* buffer, const info::kernel_symbol& item)
{
    utility::store_value(buffer, item.kernel_id, item.code_object_id,
                         std::string_view(item.kernel_name), item.kernel_object,
                         item.kernarg_segment_size, item.kernarg_segment_alignment,
                         item.group_segment_size, item.private_segment_size,
                         item.sgpr_count, item.arch_vgpr_count, item.accum_vgpr_count);
}

template <>
info::kernel_symbol inline deserialize(uint8_t*& buffer)
{
    info::kernel_symbol item;
    std::string_view    kernel_name_view;
    utility::parse_value(buffer, item.kernel_id, item.code_object_id, kernel_name_view,
                         item.kernel_object, item.kernarg_segment_size,
                         item.kernarg_segment_alignment, item.group_segment_size,
                         item.private_segment_size, item.sgpr_count, item.arch_vgpr_count,
                         item.accum_vgpr_count);
    item.kernel_name = std::string(kernel_name_view);
    return item;
}

template <>
size_t inline get_size(const info::kernel_symbol& item)
{
    return utility::get_size(
        item.kernel_id, item.code_object_id, std::string_view(item.kernel_name),
        item.kernel_object, item.kernarg_segment_size, item.kernarg_segment_alignment,
        item.group_segment_size, item.private_segment_size, item.sgpr_count,
        item.arch_vgpr_count, item.accum_vgpr_count);
}
#endif

template <>
void inline serialize(uint8_t* buffer, const info::agent_t& item)
{
    utility::store_value(
        buffer, static_cast<uint8_t>(item.agent_ptr->type), item.agent_ptr->handle,
        item.agent_ptr->device_id, item.agent_ptr->node_id,
        item.agent_ptr->logical_node_id, item.agent_ptr->logical_node_type_id,
        std::string_view(item.agent_ptr->name),
        std::string_view(item.agent_ptr->model_name),
        std::string_view(item.agent_ptr->vendor_name),
        std::string_view(item.agent_ptr->product_name), item.agent_ptr->device_type_index,
        item.agent_ptr->base_id, std::string_view(item.agent_ptr->agent_info));
}

template <>
info::agent_t inline deserialize(uint8_t*& buffer)
{
    info::agent_t item;
    item.agent_ptr = std::make_shared<rocprofsys::agent>();
    std::string_view name_view, model_name_view, vendor_name_view, product_name_view,
        agent_info_view;
    uint8_t type_val;
    utility::parse_value(
        buffer, type_val, item.agent_ptr->handle, item.agent_ptr->device_id,
        item.agent_ptr->node_id, item.agent_ptr->logical_node_id,
        item.agent_ptr->logical_node_type_id, name_view, model_name_view,
        vendor_name_view, product_name_view, item.agent_ptr->device_type_index,
        item.agent_ptr->base_id, agent_info_view);
    item.agent_ptr->type         = static_cast<agent_type>(type_val);
    item.agent_ptr->name         = std::string(name_view);
    item.agent_ptr->model_name   = std::string(model_name_view);
    item.agent_ptr->vendor_name  = std::string(vendor_name_view);
    item.agent_ptr->product_name = std::string(product_name_view);
    item.agent_ptr->agent_info   = std::string(agent_info_view);
    return item;
}

template <>
size_t inline get_size(const info::agent_t& item)
{
    return utility::get_size(
        static_cast<uint8_t>(item.agent_ptr->type), item.agent_ptr->handle,
        item.agent_ptr->device_id, item.agent_ptr->node_id,
        item.agent_ptr->logical_node_id, item.agent_ptr->logical_node_type_id,
        std::string_view(item.agent_ptr->name),
        std::string_view(item.agent_ptr->model_name),
        std::string_view(item.agent_ptr->vendor_name),
        std::string_view(item.agent_ptr->product_name), item.agent_ptr->device_type_index,
        item.agent_ptr->base_id, std::string_view(item.agent_ptr->agent_info));
}

// ============================================================================================

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

    void start(const int32_t& current_pid) { m_metadata_buffer.start(current_pid); }
    void shutdown() { m_metadata_buffer.shutdown(); }

#if ROCPROFSYS_USE_ROCM > 0

    void add_code_object(
        const rocprofiler_callback_tracing_code_object_load_data_t& code_object);
    void add_kernel_symbol(
        const rocprofiler_callback_tracing_code_object_kernel_symbol_register_data_t&
            kernel_symbol);
    std::vector<rocprofiler_callback_tracing_code_object_load_data_t>
    get_code_object_list() const;
    std::vector<rocprofiler_callback_tracing_code_object_kernel_symbol_register_data_t>
    get_kernel_symbol_list() const;
    std::optional<rocprofiler_callback_tracing_code_object_load_data_t> get_code_object(
        uint64_t code_object_id) const;
    std::optional<rocprofiler_callback_tracing_code_object_kernel_symbol_register_data_t>
    get_kernel_symbol(uint64_t kernel_id) const;

#endif

private:
    template <typename T>
    bool try_store_unique(const T& item);

    buffer_storage<flush_worker_factory_t, info::metadata_identifier_t,
                   metadata_buffer_size, metadata_flush_threshold>
        m_metadata_buffer;

    common::synchronized<std::unordered_set<size_t>> m_unique_objects;
};

struct metadata_storage_t
{
    metadata_storage_t();
    metadata_storage_t(const metadata_storage_t&)            = delete;
    metadata_storage_t& operator=(const metadata_storage_t&) = delete;
    metadata_storage_t(metadata_storage_t&&)                 = delete;
    metadata_storage_t& operator=(metadata_storage_t&&)      = delete;

    info::process            get_process_info() const { return m_process; }
    info::process_start_time get_process_start_time() const
    {
        return m_process_start_time;
    }
    info::process_end_time get_process_end_time() const { return m_process_end_time; }

    std::vector<info::pmc>         get_pmc_info_list() const { return m_pmcs; }
    std::vector<info::thread>      get_thread_info_list() const { return m_threads; }
    std::vector<info::track>       get_track_info_list() const { return m_tracks; }
    std::vector<info::queue>       get_queue_list() const { return m_queues; }
    std::vector<info::stream>      get_stream_list() const { return m_streams; }
    std::vector<std::string>       get_string_list() const { return m_string_entries; }
    std::vector<info::code_object> get_code_object_list() const { return m_code_objects; }
    std::vector<info::kernel_symbol> get_kernel_symbol_list() const
    {
        return m_kernel_symbols;
    }
    std::vector<info::agent_t> get_agents() const { return m_agents; }

    std::optional<info::code_object> get_code_object(uint64_t code_object_id) const
    {
        auto it = std::find_if(m_code_objects.begin(), m_code_objects.end(),
                               [&](const info::code_object& co) {
                                   return co.code_object_id == code_object_id;
                               });
        return it != m_code_objects.end() ? std::optional<info::code_object>(*it)
                                          : std::nullopt;
    }
    std::optional<info::kernel_symbol> get_kernel_symbol(uint64_t kernel_id) const
    {
        auto it = std::find_if(
            m_kernel_symbols.begin(), m_kernel_symbols.end(),
            [&](const info::kernel_symbol& ks) { return ks.kernel_id == kernel_id; });
        return it != m_kernel_symbols.end() ? std::optional<info::kernel_symbol>(*it)
                                            : std::nullopt;
    }

    info::process            m_process;
    info::process_start_time m_process_start_time;
    info::process_end_time   m_process_end_time;

    std::vector<info::pmc>           m_pmcs;
    std::vector<info::thread>        m_threads;
    std::vector<info::track>         m_tracks;
    std::vector<info::queue>         m_queues;
    std::vector<info::stream>        m_streams;
    std::vector<std::string>         m_string_entries;
    std::vector<info::code_object>   m_code_objects;
    std::vector<info::kernel_symbol> m_kernel_symbols;
    std::vector<info::agent_t>       m_agents;

#if ROCPROFSYS_USE_ROCM > 0
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
#endif
};

struct metadata_parser_handler_t
{
    metadata_parser_handler_t(const std::shared_ptr<metadata_storage_t>& metadata_storage,
                              std::vector<std::shared_ptr<agent>>&       agents)
    : m_metadata_storage(metadata_storage)
    , m_agents(agents)
    {}

    void execute_sample_processing(info::metadata_identifier_t type_identifier,
                                   const cacheable_t&          value)
    {
        switch(type_identifier)
        {
            case info::metadata_identifier_t::process:
            {
                m_metadata_storage->m_process = static_cast<const info::process&>(value);
                break;
            }
            case info::metadata_identifier_t::process_start_time:
            {
                m_metadata_storage->m_process_start_time =
                    static_cast<const info::process_start_time&>(value);
                break;
            }
            case info::metadata_identifier_t::process_end_time:
            {
                m_metadata_storage->m_process_end_time =
                    static_cast<const info::process_end_time&>(value);
                break;
            }
            case info::metadata_identifier_t::pmc:
            {
                m_metadata_storage->m_pmcs.push_back(
                    static_cast<const info::pmc&>(value));
                break;
            }
            case info::metadata_identifier_t::thread:
            {
                m_metadata_storage->m_threads.push_back(
                    static_cast<const info::thread&>(value));
                break;
            }
            case info::metadata_identifier_t::track:
            {
                m_metadata_storage->m_tracks.push_back(
                    static_cast<const info::track&>(value));
                break;
            }
            case info::metadata_identifier_t::queue:
            {
                m_metadata_storage->m_queues.push_back(
                    static_cast<const info::queue&>(value));
                break;
            }
            case info::metadata_identifier_t::stream:
            {
                m_metadata_storage->m_streams.push_back(
                    static_cast<const info::stream&>(value));
                break;
            }
            case info::metadata_identifier_t::string:
            {
                m_metadata_storage->m_string_entries.emplace_back(
                    static_cast<const info::string_entry&>(value).value);
                break;
            }
            case info::metadata_identifier_t::code_object:
            {
                m_metadata_storage->m_code_objects.push_back(
                    static_cast<const info::code_object&>(value));
                break;
            }
            case info::metadata_identifier_t::kernel_symbol:
            {
                m_metadata_storage->m_kernel_symbols.push_back(
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

private:
    std::shared_ptr<metadata_storage_t>  m_metadata_storage;
    std::vector<std::shared_ptr<agent>>& m_agents;
};

}  // namespace trace_cache
}  // namespace rocprofsys
