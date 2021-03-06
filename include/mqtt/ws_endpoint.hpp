// Copyright Takatoshi Kondo 2016
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_WS_ENDPOINT_HPP)
#define MQTT_WS_ENDPOINT_HPP

#if defined(MQTT_USE_TLS)
#include <boost/beast/websocket/ssl.hpp>
#endif // defined(MQTT_USE_TLS)

#include <boost/beast/websocket.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#if ASIO_STANDALONE
#include <asio/bind_executor.hpp>
#else
#include <boost/asio/bind_executor.hpp>
#endif // ASIO_STANDALONE

#include <mqtt/namespace.hpp>
#include <mqtt/string_view.hpp>
#include <mqtt/error_code.hpp>

namespace MQTT_NS {

#if ASIO_STANDALONE
namespace as = asio;
#else
namespace as = boost::asio;
#endif // ASIO_STANDALONE

template <typename Socket, typename Strand>
class ws_endpoint {
public:
    template <typename... Args>
    ws_endpoint(as::io_context& ioc, Args&&... args)
        :ws_(ioc, std::forward<Args>(args)...),
         strand_(ioc) {
        ws_.binary(true);
    }

    void close(error_code& ec) {
        ws_.close(boost::beast::websocket::close_code::normal, ec);
        if (ec) return;
        do {
            boost::beast::flat_buffer buffer;
            ws_.read(buffer, ec);
        } while (!ec);
        if (ec != boost::beast::websocket::error::closed) return;
        ec = boost::system::errc::make_error_code(boost::system::errc::success);
    }

    auto get_executor() {
        return lowest_layer().get_executor();
    }

#if BOOST_VERSION >= 107000

    auto& lowest_layer() {
        return boost::beast::get_lowest_layer(ws_);
    }

#else  // BOOST_VERSION >= 107000

    typename boost::beast::websocket::stream<Socket>::lowest_layer_type& lowest_layer() {
        return ws_.lowest_layer();
    }

#endif // BOOST_VERSION >= 107000

    typename boost::beast::websocket::stream<Socket>::next_layer_type& next_layer() {
        return ws_.next_layer();
    }

    auto native_handle() {
        return next_layer().native_handle();
    }

    template <typename T>
    void set_option(T&& t) {
        ws_.set_option(std::forward<T>(t));
    }

    template <typename ConstBufferSequence, typename AcceptHandler>
    void async_accept(
        ConstBufferSequence const& buffers,
        AcceptHandler&& handler) {
        ws_.async_accept(buffers, std::forward<AcceptHandler>(handler));
    }

    template<typename ConstBufferSequence, typename ResponseDecorator, typename AcceptHandler>
    void async_accept_ex(
        ConstBufferSequence const& buffers,
        ResponseDecorator const& decorator,
        AcceptHandler&& handler) {
        ws_.async_accept_ex(buffers, decorator, std::forward<AcceptHandler>(handler));
    }

    template <typename... Args>
    void async_handshake(Args&& ... args) {
        ws_.async_handshake(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void handshake(Args&& ... args) {
        ws_.handshake(std::forward<Args>(args)...);
    }

    template <typename MutableBufferSequence, typename ReadHandler>
    void async_read(
        MutableBufferSequence const& buffers,
        ReadHandler&& handler) {
        auto req_size = as::buffer_size(buffers);

        using beast_read_handler_t =
            std::function<void(error_code ec, std::shared_ptr<void>)>;

        std::shared_ptr<beast_read_handler_t> beast_read_handler;
        if (req_size <= buffer_.size()) {
            as::buffer_copy(buffers, buffer_.data(), req_size);
            buffer_.consume(req_size);
            handler(boost::system::errc::make_error_code(boost::system::errc::success), req_size);
            return;
        }

        beast_read_handler.reset(
            new beast_read_handler_t(
                [this, req_size, buffers, handler = std::forward<ReadHandler>(handler)]
                (error_code ec, std::shared_ptr<void> const& v) mutable {
                    if (ec) {
                        std::forward<ReadHandler>(handler)(ec, 0);
                        return;
                    }
                    if (!ws_.got_binary()) {
                        buffer_.consume(buffer_.size());
                        std::forward<ReadHandler>(handler)
                            (boost::system::errc::make_error_code(boost::system::errc::bad_message), 0);
                        return;
                    }
                    if (req_size > buffer_.size()) {
                        auto beast_read_handler = std::static_pointer_cast<beast_read_handler_t>(v);
                        ws_.async_read(
                            buffer_,
                            as::bind_executor(
                                strand_,
                                [beast_read_handler]
                                (error_code ec, std::size_t) {
                                    (*beast_read_handler)(ec, beast_read_handler);
                                }
                            )
                        );
                        return;
                    }
                    as::buffer_copy(buffers, buffer_.data(), req_size);
                    buffer_.consume(req_size);
                    std::forward<ReadHandler>(handler)(boost::system::errc::make_error_code(boost::system::errc::success), req_size);
                }
            )
        );
        ws_.async_read(
            buffer_,
            as::bind_executor(
                strand_,
                [beast_read_handler]
                (error_code ec, std::size_t) {
                    (*beast_read_handler)(ec, beast_read_handler);
                }
            )
        );
    }

    template <typename ConstBufferSequence>
    std::size_t write(
        ConstBufferSequence const& buffers) {
        ws_.write(buffers);
        return as::buffer_size(buffers);
    }

    template <typename ConstBufferSequence>
    std::size_t write(
        ConstBufferSequence const& buffers,
        error_code& ec) {
        ws_.write(buffers, ec);
        return as::buffer_size(buffers);
    }

    template <typename ConstBufferSequence, typename WriteHandler>
    void async_write(
        ConstBufferSequence const& buffers,
        WriteHandler&& handler) {
        ws_.async_write(
            buffers,
            as::bind_executor(
                strand_,
                std::forward<WriteHandler>(handler)
            )
        );
    }

    template <typename PostHandler>
    void post(PostHandler&& handler) {
        as::post(
            strand_,
            std::forward<PostHandler>(handler)
        );
    }

private:
    boost::beast::websocket::stream<Socket> ws_;
    boost::beast::flat_buffer buffer_;
    Strand strand_;
};

template <typename Socket, typename Strand, typename MutableBufferSequence, typename ReadHandler>
inline void async_read(
    ws_endpoint<Socket, Strand>& ep,
    MutableBufferSequence const& buffers,
    ReadHandler&& handler) {
    ep.async_read(buffers, std::forward<ReadHandler>(handler));
}

template <typename Socket, typename Strand, typename ConstBufferSequence>
inline std::size_t write(
    ws_endpoint<Socket, Strand>& ep,
    ConstBufferSequence const& buffers) {
    return ep.write(buffers);
}

template <typename Socket, typename Strand, typename ConstBufferSequence>
inline std::size_t write(
    ws_endpoint<Socket, Strand>& ep,
    ConstBufferSequence const& buffers,
    error_code& ec) {
    return ep.write(buffers, ec);
}

template <typename Socket, typename Strand, typename ConstBufferSequence, typename WriteHandler>
inline void async_write(
    ws_endpoint<Socket, Strand>& ep,
    ConstBufferSequence const& buffers,
    WriteHandler&& handler) {
    ep.async_write(buffers, std::forward<WriteHandler>(handler));
}

} // namespace MQTT_NS

#endif // MQTT_WS_ENDPOINT_HPP
