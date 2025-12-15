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

#include "core/trace_cache/cacheable.hpp"

#include "common/defines.h"

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace rocprofsys
{
namespace trace_cache
{

struct worker_synchronization_t
{
    std::condition_variable is_running_condition;
    std::atomic_bool        is_running{ false };

    std::condition_variable exit_finished_condition;
    std::atomic_bool        exit_finished{ false };

    std::mutex              data_ready_mutex;
    std::condition_variable data_ready_condition;
    std::atomic_bool        data_ready{ false };

    pid_t origin_pid{ 0 };

    void notify_data_ready()
    {
        data_ready.store(true, std::memory_order_release);
        data_ready_condition.notify_one();
    }
};
using worker_synchronization_ptr_t = std::shared_ptr<worker_synchronization_t>;

class mmap_flush_worker_t
{
public:
    using flush_callback_t = std::function<void(uint8_t* dest, size_t max_bytes,
                                                size_t& bytes_written, bool force)>;

    static constexpr size_t initial_mmap_size = 64 * 1024 * 1024;
    static constexpr size_t growth_factor     = 2;

    mmap_flush_worker_t(flush_callback_t             flush_callback,
                        worker_synchronization_ptr_t sync, std::string filepath,
                        size_t max_file_size)
    : m_flush_callback(std::move(flush_callback))
    , m_sync(std::move(sync))
    , m_filepath(std::move(filepath))
    , m_max_file_size(max_file_size)
    {}

    ~mmap_flush_worker_t() { stop(getpid()); }

    void start(const pid_t& current_pid)
    {
        if(m_sync->is_running.load()) return;

        m_fd = ::open(m_filepath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
        if(m_fd == -1)
        {
            throw std::runtime_error("mmap_flush_worker: failed to open file: " +
                                     m_filepath);
        }

        m_current_size = std::min(initial_mmap_size, m_max_file_size);

        if(::ftruncate(m_fd, static_cast<off_t>(m_current_size)) == -1)
        {
            ::close(m_fd);
            throw std::runtime_error("mmap_flush_worker: ftruncate failed");
        }

        m_mmap_ptr = static_cast<uint8_t*>(
            ::mmap(nullptr, m_current_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0));

        if(m_mmap_ptr == MAP_FAILED)
        {
            ::close(m_fd);
            throw std::runtime_error("mmap_flush_worker: mmap failed");
        }

        // We will write sequentially to the mmap, so we can advise the kernel to prefetch
        // the data.
        ::madvise(m_mmap_ptr, m_current_size, MADV_SEQUENTIAL);

        m_write_offset        = 0;
        m_sync->origin_pid    = current_pid;
        m_sync->is_running    = true;
        m_sync->exit_finished = false;

        m_thread = std::make_unique<std::thread>([this]() { worker_loop(); });
    }

    void stop(const pid_t& current_pid)
    {
        if(!m_sync || !m_sync->is_running.load()) return;

        m_sync->is_running = false;
        m_sync->notify_data_ready();

        if(current_pid != m_sync->origin_pid) return;

        if(m_thread && m_thread->joinable())
        {
            m_thread->join();
            m_thread.reset();
        }

        cleanup_mmap();
    }

private:
    void worker_loop()
    {
        while(m_sync->is_running.load())
        {
            {
                std::unique_lock lock{ m_sync->data_ready_mutex };
                m_sync->data_ready_condition.wait(lock, [this]() {
                    return m_sync->data_ready.load(std::memory_order_acquire) ||
                           !m_sync->is_running.load();
                });
                m_sync->data_ready.store(false, std::memory_order_release);
            }

            if(m_sync->is_running.load())
            {
                do_flush(false);
            }
        }

        do_flush(true);

        m_sync->exit_finished = true;
        m_sync->exit_finished_condition.notify_one();
    }

    bool ensure_capacity(size_t required_bytes)
    {
        if(m_write_offset + required_bytes <= m_current_size) return true;

        size_t new_size = m_current_size;
        while(new_size < m_write_offset + required_bytes && new_size < m_max_file_size)
        {
            new_size = std::min(new_size * growth_factor, m_max_file_size);
        }

        if(new_size <= m_current_size || new_size > m_max_file_size) return false;

        if(::ftruncate(m_fd, static_cast<off_t>(new_size)) == -1) return false;

        void* new_ptr = ::mremap(m_mmap_ptr, m_current_size, new_size, MREMAP_MAYMOVE);
        if(new_ptr == MAP_FAILED) return false;

        m_mmap_ptr = static_cast<uint8_t*>(new_ptr);

        ::madvise(m_mmap_ptr + m_current_size, new_size - m_current_size,
                  MADV_SEQUENTIAL);

        m_current_size = new_size;
        return true;
    }

    void do_flush(bool force)
    {
        if(m_mmap_ptr == nullptr || m_mmap_ptr == MAP_FAILED) return;

        size_t available     = m_current_size - m_write_offset;
        size_t bytes_written = 0;

        m_flush_callback(m_mmap_ptr + m_write_offset, available, bytes_written, force);

        if(bytes_written > 0)
        {
            m_write_offset += bytes_written;

            if(m_write_offset > m_current_size * 3 / 4)
            {
                ensure_capacity(m_current_size);
            }
        }
    }

    void cleanup_mmap()
    {
        if(m_mmap_ptr != nullptr && m_mmap_ptr != MAP_FAILED)
        {
            ::msync(m_mmap_ptr, m_write_offset, MS_SYNC);
            ::munmap(m_mmap_ptr, m_current_size);
            m_mmap_ptr = nullptr;
        }

        if(m_fd != -1)
        {
            ::ftruncate(m_fd, static_cast<off_t>(m_write_offset));
            ::fsync(m_fd);
            ::close(m_fd);
            m_fd = -1;
        }
    }

    flush_callback_t             m_flush_callback;
    worker_synchronization_ptr_t m_sync;
    std::string                  m_filepath;
    size_t                       m_max_file_size;
    int                          m_fd{ -1 };
    uint8_t*                     m_mmap_ptr{ nullptr };
    size_t                       m_current_size{ 0 };
    size_t                       m_write_offset{ 0 };
    std::unique_ptr<std::thread> m_thread;
};

template <size_t MaxFileSize = 4ULL * 1024 * 1024 * 1024>
struct mmap_flush_worker_factory_t
{
    using worker_t = mmap_flush_worker_t;

    mmap_flush_worker_factory_t()                                         = delete;
    mmap_flush_worker_factory_t(mmap_flush_worker_factory_t&)             = delete;
    mmap_flush_worker_factory_t& operator=(mmap_flush_worker_factory_t&)  = delete;
    mmap_flush_worker_factory_t(mmap_flush_worker_factory_t&&)            = delete;
    mmap_flush_worker_factory_t& operator=(mmap_flush_worker_factory_t&&) = delete;

    static std::shared_ptr<worker_t> get_worker(
        typename worker_t::flush_callback_t flush_callback,
        const worker_synchronization_ptr_t& sync, std::string filepath)
    {
        return std::make_shared<worker_t>(std::move(flush_callback), sync,
                                          std::move(filepath), MaxFileSize);
    }
};

using ofs_t             = std::basic_ostream<char>;
using worker_function_t = std::function<void(ofs_t& ofs, bool force)>;

struct flush_worker_t
{
    explicit flush_worker_t(worker_function_t            worker_function,
                            worker_synchronization_ptr_t worker_synchronization_ptr,
                            std::string                  filepath);

    void start(const pid_t& current_pid);

    void stop(const pid_t& current_pid);

private:
    worker_function_t            m_worker_function;
    worker_synchronization_ptr_t m_worker_synchronization;
    std::string                  m_filepath;
    std::ofstream                m_ofs;
    std::unique_ptr<std::thread> m_flushing_thread;
};

struct flush_worker_factory_t
{
    using worker_t = flush_worker_t;

    flush_worker_factory_t()                                    = delete;
    flush_worker_factory_t(flush_worker_factory_t&)             = delete;
    flush_worker_factory_t& operator=(flush_worker_factory_t&)  = delete;
    flush_worker_factory_t(flush_worker_factory_t&&)            = delete;
    flush_worker_factory_t& operator=(flush_worker_factory_t&&) = delete;

    static std::shared_ptr<worker_t> get_worker(
        worker_function_t                   worker_function,
        const worker_synchronization_ptr_t& worker_synchronization_ptr,
        std::string                         filepath)
    {
        return std::make_shared<worker_t>(worker_function, worker_synchronization_ptr,
                                          std::move(filepath));
    }
};

template <typename WorkerFactory, typename TypeIdentifierEnum,
          size_t BufferSize = buffer_size, size_t FlushThreshold = flush_threshold>
class buffer_storage
{
    static_assert(type_traits::is_enum_class_v<TypeIdentifierEnum>,
                  "TypeIdentifierEnum must be an enum class");

    static constexpr bool is_mmap_worker =
        std::is_same_v<typename WorkerFactory::worker_t, mmap_flush_worker_t>;

public:
    explicit buffer_storage(std::string filepath)
    : m_worker{ create_worker(std::move(filepath)) }
    {}

    ~buffer_storage() { shutdown(); }

    void start(const pid_t& current_pid = getpid())
    {
        if(m_worker == nullptr)
        {
            throw std::runtime_error(
                "Worker is null - unable to start buffered storage.");
        }

        if(is_running())
        {
            return;
        }

        m_worker->start(current_pid);
    }

    void shutdown(const pid_t& current_pid = getpid())
    {
        if(m_worker == nullptr)
        {
            throw std::runtime_error(
                "Worker is null - unable to shutdown buffered storage.");
        }

        if(!is_running())
        {
            return;
        }

        m_worker->stop(current_pid);
    }

    template <typename Type>
    auto store(const Type& value)
    {
        if(m_worker == nullptr || !is_running())
        {
            throw std::runtime_error(
                "Trying to use buffered storage while it is not running");
        }

        type_traits::check_type<Type, TypeIdentifierEnum>();

        using TypeIdentifierEnumUderlayingType =
            std::underlying_type_t<TypeIdentifierEnum>;

        size_t sample_size      = get_size(value);
        size_t bytes_to_reserve = header_size<TypeIdentifierEnum> + sample_size;

        uint8_t* dest = reserve_memory_space(bytes_to_reserve);

        {
            size_t position = 0;
            auto   type_identifier_value =
                static_cast<TypeIdentifierEnumUderlayingType>(Type::type_identifier);
            utility::store_value(type_identifier_value, dest, position);
            utility::store_value(sample_size, dest, position);
            serialize(dest + position, value);
        }

        commit_write();
    }

    ROCPROFSYS_INLINE bool is_running() const
    {
        return m_worker_synchronization != nullptr &&
               m_worker_synchronization->is_running;
    }

private:
    auto create_worker(std::string filepath)
    {
        if constexpr(is_mmap_worker)
        {
            return WorkerFactory::get_worker(
                [this](uint8_t* dest, size_t max_bytes, size_t& bytes_written,
                       bool force) {
                    flush_to_mmap(dest, max_bytes, bytes_written, force);
                },
                m_worker_synchronization, std::move(filepath));
        }
        else
        {
            return WorkerFactory::get_worker(
                [this](ofs_t& ofs, bool force) { flush_to_stream(ofs, force); },
                m_worker_synchronization, std::move(filepath));
        }
    }

    void flush_to_mmap(uint8_t* dest, size_t max_bytes, size_t& bytes_written, bool)
    {
        size_t _committed, _tail, _new_tail;
        size_t total_to_flush;
        {
            std::lock_guard guard{ m_mutex };
            _committed = m_committed;
            _tail      = m_tail;

            if(_committed == _tail)
            {
                bytes_written = 0;
                return;
            }

            total_to_flush = _committed > _tail ? (_committed - _tail)
                                                : (BufferSize - _tail + _committed);

            if(total_to_flush > max_bytes)
            {
                total_to_flush = max_bytes;
                _new_tail      = (_tail + total_to_flush) % BufferSize;
            }
            else
            {
                _new_tail = _committed;
            }
            m_tail = _new_tail;
        }

        if(_tail + total_to_flush <= BufferSize)
        {
            std::memcpy(dest, m_buffer->data() + _tail, total_to_flush);
        }
        else
        {
            size_t len1 = BufferSize - _tail;
            size_t len2 = total_to_flush - len1;
            std::memcpy(dest, m_buffer->data() + _tail, len1);
            std::memcpy(dest + len1, m_buffer->data(), len2);
        }
        bytes_written = total_to_flush;
    }

    void flush_to_stream(ofs_t& ofs, bool force)
    {
        size_t _committed, _tail;
        {
            std::lock_guard guard{ m_mutex };
            _committed = m_committed;
            _tail      = m_tail;

            if(_committed == _tail)
            {
                return;
            }

            auto used_space = _committed > _tail ? (_committed - _tail)
                                                 : (BufferSize - _tail + _committed);
            if(!force && used_space < FlushThreshold)
            {
                return;
            }
            m_tail = _committed;
        }

        if(_committed > _tail)
        {
            ofs.write(reinterpret_cast<const char*>(m_buffer->data() + _tail),
                      _committed - _tail);
        }
        else
        {
            ofs.write(reinterpret_cast<const char*>(m_buffer->data() + _tail),
                      BufferSize - _tail);
            ofs.write(reinterpret_cast<const char*>(m_buffer->data()), _committed);
        }
        if(ofs.fail())
        {
            throw std::runtime_error(
                std::string("Error flushing buffered storage to file for pid: ") +
                std::to_string(m_worker_synchronization->origin_pid) + "\n");
        }
    }

    void fragment_memory()
    {
        auto* _data = m_buffer->data();
        memset(_data + m_head, std::numeric_limits<uint8_t>::max(), BufferSize - m_head);
        *reinterpret_cast<TypeIdentifierEnum*>(_data + m_head) =
            TypeIdentifierEnum::fragmented_space;

        size_t remaining_bytes = BufferSize - m_head - header_size<TypeIdentifierEnum>;
        *reinterpret_cast<size_t*>(_data + m_head + sizeof(TypeIdentifierEnum)) =
            remaining_bytes;
        m_head = 0;
    }

    ROCPROFSYS_INLINE uint8_t* reserve_memory_space(const size_t& number_of_bytes)
    {
        std::lock_guard scope{ m_mutex };

        if(__builtin_expect((m_head + number_of_bytes + header_size<TypeIdentifierEnum>) >
                                BufferSize,
                            0))
        {
            fragment_memory();
        }

        uint8_t* dest = m_buffer->data() + m_head;
        m_head += number_of_bytes;

        return dest;
    }

    ROCPROFSYS_INLINE void commit_write()
    {
        {
            std::lock_guard scope{ m_mutex };
            m_committed = m_head;
        }

        if constexpr(is_mmap_worker)
        {
            m_worker_synchronization->notify_data_ready();
        }
    }

private:
    worker_synchronization_ptr_t m_worker_synchronization{
        std::make_shared<worker_synchronization_t>()
    };

    std::shared_ptr<typename WorkerFactory::worker_t> m_worker;

    std::mutex m_mutex;
    size_t     m_head{ 0 };
    size_t     m_committed{ 0 };
    size_t     m_tail{ 0 };

    std::unique_ptr<buffer_array_t<BufferSize>> m_buffer{
        std::make_unique<buffer_array_t<BufferSize>>()
    };
};

}  // namespace trace_cache
}  // namespace rocprofsys
