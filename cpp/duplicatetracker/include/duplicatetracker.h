/****************************************************************************
**                                MIT License
**
** Copyright (C) 2021-2023 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz
*<marc.mutz@kdab.com>
**
** This file is part of KDToolBox (https://github.com/KDAB/KDToolBox).
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, ** and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice (including the next paragraph)
** shall be included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF ** CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
****************************************************************************/

#pragma once

#include <algorithm> // for std::max
#include <cstddef>   // for std::max_align_t
#include <unordered_set>
#ifdef __has_include
#if __has_include(<memory_resource>) && __cplusplus > 201402L
#include <memory_resource>
#endif
#endif

namespace KDToolBox
{

namespace detail
{
template<std::size_t N>
struct alignas(std::max_align_t) Storage
{
    char m_buffer[N];
};

template<typename T, typename Hash, typename Equal>
class DuplicateTrackerBaseBase
{
protected:
#ifdef __cpp_lib_memory_resource
    std::pmr::monotonic_buffer_resource m_res;
    std::pmr::unordered_set<T, Hash, Equal> m_set;
#else
    std::unordered_set<T, Hash, Equal> m_set;
#endif
#ifdef __cpp_lib_memory_resource
    explicit DuplicateTrackerBaseBase(char *buffer, std::size_t bufferSize, std::size_t N, const Hash &h,
                                      const Equal &e)
        : m_res(buffer, bufferSize)
        , m_set(N, h, e, &m_res)
    {
    }
#else
    explicit DuplicateTrackerBaseBase(std::size_t N, const Hash &h, const Equal &e)
        : m_set(N, h, e)
    {
    }
#endif
    DuplicateTrackerBaseBase(const DuplicateTrackerBaseBase &) = delete;
    DuplicateTrackerBaseBase(DuplicateTrackerBaseBase &&) = delete;
    DuplicateTrackerBaseBase &operator=(const DuplicateTrackerBaseBase &) = delete;
    DuplicateTrackerBaseBase &operator=(DuplicateTrackerBaseBase &&) = delete;
    ~DuplicateTrackerBaseBase() = default;

    void reserve(std::size_t n) { m_set.reserve(n); }

    bool hasSeen(const T &t) { return !m_set.insert(t).second; }
    bool hasSeen(T &&t) { return !m_set.insert(std::move(t)).second; }

    bool contains(const T &t) const { return m_set.find(t) != m_set.end(); }

    const decltype(m_set) &set() const noexcept { return m_set; }
    decltype(m_set) &set() noexcept { return m_set; }
};
template<typename T>
struct node_guesstimate
{
    void *next;
    std::size_t hash;
    T value;
};
template<typename T>
constexpr std::size_t calc_memory(std::size_t numNodes) noexcept
{
    return sizeof(node_guesstimate<T>) * numNodes // nodes
           + sizeof(void *) * numNodes;           // bucket list
}

template<typename T, std::size_t Prealloc, typename Hash, typename Equal>
class DuplicateTrackerBase :
#ifdef __cpp_lib_memory_resource
    Storage<calc_memory<T>(Prealloc)>,
#endif
    DuplicateTrackerBaseBase<T, Hash, Equal>
{
protected:
    using Base = detail::DuplicateTrackerBaseBase<T, Hash, Equal>;

    explicit DuplicateTrackerBase(std::size_t numBuckets, const Hash &h, const Equal &e)
#ifdef __cpp_lib_memory_resource
        : Base(this->m_buffer, sizeof(this->m_buffer), std::max(numBuckets, Prealloc), h, e){}
#else
        : Base(std::max(numBuckets, Prealloc), h, e)
    {
    }
#endif
        ~DuplicateTrackerBase() = default;

    using Base::contains;
    using Base::hasSeen;
    using Base::reserve;
    using Base::set;
};
} // namespace detail

template<typename T, std::size_t Prealloc = 64, typename Hash = std::hash<T>, typename Equal = std::equal_to<T>>
class DuplicateTracker : detail::DuplicateTrackerBase<T, Prealloc, Hash, Equal>
{
    using Base = detail::DuplicateTrackerBase<T, Prealloc, Hash, Equal>;

public:
    DuplicateTracker()
        : DuplicateTracker(Prealloc, Hash(), Equal())
    {
    }
    explicit DuplicateTracker(std::size_t numBuckets, const Hash &h = Hash(), const Equal &e = Equal())
        : Base(numBuckets, h, e)
    {
    }

    using Base::contains;
    using Base::hasSeen;
    using Base::reserve;
    using Base::set;
};

} // namespace KDToolBox
