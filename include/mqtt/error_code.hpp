// Copyright Takatoshi Kondo 2019
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_ERROR_CODE_HPP)
#define MQTT_ERROR_CODE_HPP

#include <mqtt/namespace.hpp>

#if ASIO_STANDALONE
#include <system_error>
#else
#include <boost/system/error_code.hpp>
#endif // ASIO_STANDALONE

namespace MQTT_NS {

#if ASIO_STANDALONE
using error_code = std::error_code;
using system_error = std::system_error;
auto message_size_errc = std::errc::message_size;
auto protocol_error_errc = std::errc::protocol_error;
auto generic_category = std::generic_category;
#else
using error_code = boost::system::error_code;
using system_error = boost::system::system_error;
auto message_size_errc = boost::system::errc::message_size;
auto protocol_error_errc = boost::system::errc::protocol_error;
auto generic_category = boost::system::generic_category;
#endif // ASIO_STANDALONE

} // namespace MQTT_NS

#endif // MQTT_ERROR_CODE_HPP
