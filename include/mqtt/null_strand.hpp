// Copyright Takatoshi Kondo 2016
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_NULL_STRAND_HPP)
#define MQTT_NULL_STRAND_HPP

#include <utility>

#if ASIO_STANDALONE
#include <asio.hpp>
#else
#include <boost/asio.hpp>
#endif // ASIO_STANDALONE

#include <mqtt/namespace.hpp>

namespace MQTT_NS {

#if ASIO_STANDALONE
namespace as = asio;
#else
namespace as = boost::asio;
#endif // ASIO_STANDALONE

struct null_strand {
    null_strand(as::io_context& ioc) noexcept : ioc_(ioc) {}
    template <typename Func, typename Allocator>
    void post(Func&& f, Allocator) const {
        as::post(
            ioc_,
            [f = std::forward<Func>(f)] () mutable {
                std::move(f)();
            }
        );
    }
    template <typename Func, typename Allocator>
    void defer(Func&& f, Allocator) const {
        as::defer(
            ioc_,
            [f = std::forward<Func>(f)] () mutable {
                std::move(f)();
            }
        );
    }
    template <typename Func, typename Allocator>
    void dispatch(Func&& f, Allocator) const {
        std::forward<Func>(f)();
    }
    void on_work_started() const noexcept {}
    void on_work_finished() const noexcept {}
    as::io_context& context() noexcept{ return ioc_; }
    as::io_context const& context() const noexcept { return ioc_; }
private:
    as::io_context& ioc_;
};

inline bool operator==(null_strand const& lhs, null_strand const& rhs) {
    return std::addressof(lhs) == std::addressof(rhs);
}

inline bool operator!=(null_strand const& lhs, null_strand const& rhs) {
    return !(lhs == rhs);
}

} // namespace MQTT_NS

#if ASIO_STANDALONE
namespace asio {
#else
namespace boost {
namespace asio {
#endif

template<>
struct is_executor<MQTT_NS::null_strand> : std::true_type {
};

#if ASIO_STANDALONE
} // namespace asio
#else
} // namespace asio
} // namespace boost
#endif

#endif // MQTT_NULL_STRAND_HPP
