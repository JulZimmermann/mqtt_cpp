// Copyright Takatoshi Kondo 2018
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "test_main.hpp"
#include "combi_test.hpp"
#include "checker.hpp"

#include <mqtt/optional.hpp>

#include <vector>
#include <string>

BOOST_AUTO_TEST_SUITE(test_as_buffer_async_pubsub_2)

BOOST_AUTO_TEST_CASE( pub_qos0_sub_qos2 ) {
    auto test = [](boost::asio::io_context& ioc, auto& c, auto finish, auto& /*b*/) {
        using packet_id_t = typename std::remove_reference_t<decltype(*c)>::packet_id_t;
        c->set_clean_session(true);

        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;


        checker chk = {
            // connect
            cont("h_connack"),
            // subscribe topic1 QoS2
            cont("h_suback"),
            // publish topic1 QoS0
            cont("h_publish"),
            cont("h_unsuback"),
            // disconnect
            cont("h_close"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c, &pid_sub]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_sub = c->acquire_unique_packet_id();
                    c->async_subscribe(
                        pid_sub,
                        as::buffer(*topic1),
                        MQTT_NS::qos::exactly_once,
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_puback_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_pubrec_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_pubcomp_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_suback_handler(
                [&chk, &c, &pid_sub]
                (packet_id_t packet_id, std::vector<MQTT_NS::suback_return_code> results) {
                    MQTT_CHK("h_suback");
                    BOOST_TEST(packet_id == pid_sub);
                    BOOST_TEST(results.size() == 1U);
                    BOOST_TEST(results[0] == MQTT_NS::suback_return_code::success_maximum_qos_2);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    auto contents = std::make_shared<std::string>("topic1_contents");
                    c->async_publish(
                        as::buffer(*topic1),
                        as::buffer(*contents),
                        std::make_pair(topic1, contents),
                        MQTT_NS::qos::at_most_once,
                        false
                    );
                    return true;
                });
            c->set_unsuback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id) {
                    MQTT_CHK("h_unsuback");
                    BOOST_TEST(packet_id == pid_unsub);
                    c->async_disconnect();
                    return true;
                });
            c->set_publish_handler(
                [&chk, &c, &pid_unsub]
                (bool is_dup,
                 MQTT_NS::qos qos_value,
                 bool is_retain,
                 MQTT_NS::optional<packet_id_t> packet_id,
                 MQTT_NS::buffer topic,
                 MQTT_NS::buffer contents) {
                    MQTT_CHK("h_publish");
                    BOOST_TEST(is_dup == false);
                    BOOST_TEST(qos_value == MQTT_NS::qos::at_most_once);
                    BOOST_TEST(is_retain == false);
                    BOOST_CHECK(!packet_id);
                    BOOST_TEST(topic == "topic1");
                    BOOST_TEST(contents == "topic1_contents");
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_unsub = c->acquire_unique_packet_id();
                    c->async_unsubscribe(
                        pid_unsub,
                        as::buffer(*topic1),
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c, &pid_sub]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_sub = c->acquire_unique_packet_id();
                    c->async_subscribe(
                        pid_sub,
                        as::buffer(*topic1),
                        MQTT_NS::qos::exactly_once,
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_v5_puback_handler(
                []
                (packet_id_t, MQTT_NS::v5::puback_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_pubrec_handler(
                []
                (packet_id_t, MQTT_NS::v5::pubrec_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_pubcomp_handler(
                []
                (packet_id_t, MQTT_NS::v5::pubcomp_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_suback_handler(
                [&chk, &c, &pid_sub]
                (packet_id_t packet_id, std::vector<MQTT_NS::v5::suback_reason_code> reasons, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_suback");
                    BOOST_TEST(packet_id == pid_sub);
                    BOOST_TEST(reasons.size() == 1U);
                    BOOST_TEST(reasons[0] == MQTT_NS::v5::suback_reason_code::granted_qos_2);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    auto contents = std::make_shared<std::string>("topic1_contents");
                    c->async_publish(
                        as::buffer(*topic1),
                        as::buffer(*contents),
                        std::make_pair(topic1, contents),
                        MQTT_NS::qos::at_most_once,
                        false
                    );
                    return true;
                });
            c->set_v5_unsuback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id, std::vector<MQTT_NS::v5::unsuback_reason_code> reasons, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_unsuback");
                    BOOST_TEST(packet_id == pid_unsub);
                    BOOST_TEST(reasons.size() == 1U);
                    BOOST_TEST(reasons[0] == MQTT_NS::v5::unsuback_reason_code::success);
                    c->async_disconnect();
                    return true;
                });
            c->set_v5_publish_handler(
                [&chk, &c, &pid_unsub]
                (bool is_dup,
                 MQTT_NS::qos qos_value,
                 bool is_retain,
                 MQTT_NS::optional<packet_id_t> packet_id,
                 MQTT_NS::buffer topic,
                 MQTT_NS::buffer contents,
                 MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_publish");
                    BOOST_TEST(is_dup == false);
                    BOOST_TEST(qos_value == MQTT_NS::qos::at_most_once);
                    BOOST_TEST(is_retain == false);
                    BOOST_CHECK(!packet_id);
                    BOOST_TEST(topic == "topic1");
                    BOOST_TEST(contents == "topic1_contents");
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_unsub = c->acquire_unique_packet_id();
                    c->async_unsubscribe(
                        pid_unsub,
                        as::buffer(*topic1),
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }
        c->set_close_handler(
            [&chk, &finish]
            () {
                MQTT_CHK("h_close");
                finish();
            });
        c->set_error_handler(
            []
            (MQTT_NS::error_code) {
                BOOST_CHECK(false);
            });
        c->set_pub_res_sent_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
            });
        c->async_connect();
        ioc.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_async(test);
}

BOOST_AUTO_TEST_CASE( pub_qos1_sub_qos2 ) {
    auto test = [](boost::asio::io_context& ioc, auto& c, auto finish, auto& /*b*/) {
        using packet_id_t = typename std::remove_reference_t<decltype(*c)>::packet_id_t;
        c->set_clean_session(true);

        std::uint16_t pid_pub;
        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;


        checker chk = {
            // connect
            cont("h_connack"),
            // subscribe topic1 QoS0
            cont("h_suback"),
            // publish topic1 QoS1
            cont("h_publish"),
            cont("h_pub_res_sent"),
            deps("h_puback", "h_publish"),
            cont("h_unsuback"),
            // disconnect
            cont("h_close"),
        };

        MQTT_NS::optional<packet_id_t> recv_packet_id;
        c->set_pub_res_sent_handler(
            [&chk, &recv_packet_id]
            (packet_id_t packet_id) {
                MQTT_CHK("h_pub_res_sent");
                BOOST_TEST(*recv_packet_id == packet_id);
            });

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c, &pid_sub]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_sub = c->acquire_unique_packet_id();
                    c->async_subscribe(
                        pid_sub,
                        as::buffer(*topic1),
                        MQTT_NS::qos::exactly_once,
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_puback_handler(
                [&chk, &c, &pid_pub, &pid_unsub]
                (packet_id_t packet_id) {
                    MQTT_CHK("h_puback");
                    BOOST_TEST(packet_id == pid_pub);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_unsub = c->acquire_unique_packet_id();
                    c->async_unsubscribe(
                        pid_unsub,
                        as::buffer(*topic1),
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_pubrec_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_pubcomp_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_suback_handler(
                [&chk, &c, &pid_sub, &pid_pub]
                (packet_id_t packet_id, std::vector<MQTT_NS::suback_return_code> results) {
                    MQTT_CHK("h_suback");
                    BOOST_TEST(packet_id == pid_sub);
                    BOOST_TEST(results.size() == 1U);
                    BOOST_TEST(results[0] == MQTT_NS::suback_return_code::success_maximum_qos_2);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    auto contents = std::make_shared<std::string>("topic1_contents");
                    pid_pub = c->acquire_unique_packet_id();
                    c->async_publish(
                        pid_pub,
                        as::buffer(*topic1),
                        as::buffer(*contents),
                        std::make_pair(topic1, contents),
                        MQTT_NS::qos::at_least_once
                    );
                    return true;
                });
            c->set_unsuback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id) {
                    MQTT_CHK("h_unsuback");
                    BOOST_TEST(packet_id == pid_unsub);
                    c->async_disconnect();
                    return true;
                });
            c->set_publish_handler(
                [&chk, &recv_packet_id]
                (bool is_dup,
                 MQTT_NS::qos qos_value,
                 bool is_retain,
                 MQTT_NS::optional<packet_id_t> packet_id,
                 MQTT_NS::buffer topic,
                 MQTT_NS::buffer contents) {
                    MQTT_CHK("h_publish");
                    BOOST_TEST(is_dup == false);
                    BOOST_TEST(qos_value == MQTT_NS::qos::at_least_once);
                    BOOST_TEST(is_retain == false);
                    BOOST_TEST(*packet_id != 0);
                    recv_packet_id = packet_id;
                    BOOST_TEST(topic == "topic1");
                    BOOST_TEST(contents == "topic1_contents");
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c, &pid_sub]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_sub = c->acquire_unique_packet_id();
                    c->async_subscribe(
                        pid_sub,
                        as::buffer(*topic1),
                        MQTT_NS::qos::exactly_once,
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_v5_puback_handler(
                [&chk, &c, &pid_pub, &pid_unsub]
                (packet_id_t packet_id, MQTT_NS::v5::puback_reason_code, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_puback");
                    BOOST_TEST(packet_id == pid_pub);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_unsub = c->acquire_unique_packet_id();
                    c->async_unsubscribe(
                        pid_unsub,
                        as::buffer(*topic1),
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_v5_pubrec_handler(
                []
                (packet_id_t, MQTT_NS::v5::pubrec_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_pubcomp_handler(
                []
                (packet_id_t, MQTT_NS::v5::pubcomp_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_suback_handler(
                [&chk, &c, &pid_sub, &pid_pub]
                (packet_id_t packet_id, std::vector<MQTT_NS::v5::suback_reason_code> reasons, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_suback");
                    BOOST_TEST(packet_id == pid_sub);
                    BOOST_TEST(reasons.size() == 1U);
                    BOOST_TEST(reasons[0] == MQTT_NS::v5::suback_reason_code::granted_qos_2);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    auto contents = std::make_shared<std::string>("topic1_contents");
                    pid_pub = c->acquire_unique_packet_id();
                    c->async_publish(
                        pid_pub,
                        as::buffer(*topic1),
                        as::buffer(*contents),
                        std::make_pair(topic1, contents),
                        MQTT_NS::qos::at_least_once
                    );
                    return true;
                });
            c->set_v5_unsuback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id, std::vector<MQTT_NS::v5::unsuback_reason_code> reasons, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_unsuback");
                    BOOST_TEST(packet_id == pid_unsub);
                    BOOST_TEST(reasons.size() == 1U);
                    BOOST_TEST(reasons[0] == MQTT_NS::v5::unsuback_reason_code::success);
                    c->async_disconnect();
                    return true;
                });
            c->set_v5_publish_handler(
                [&chk, &recv_packet_id]
                (bool is_dup,
                 MQTT_NS::qos qos_value,
                 bool is_retain,
                 MQTT_NS::optional<packet_id_t> packet_id,
                 MQTT_NS::buffer topic,
                 MQTT_NS::buffer contents,
                 MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_publish");
                    BOOST_TEST(is_dup == false);
                    BOOST_TEST(qos_value == MQTT_NS::qos::at_least_once);
                    BOOST_TEST(is_retain == false);
                    BOOST_TEST(*packet_id != 0);
                    recv_packet_id = packet_id;
                    BOOST_TEST(topic == "topic1");
                    BOOST_TEST(contents == "topic1_contents");
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }
        c->set_close_handler(
            [&chk, &finish]
            () {
                MQTT_CHK("h_close");
                finish();
            });
        c->set_error_handler(
            []
            (MQTT_NS::error_code) {
                BOOST_CHECK(false);
            });
        c->async_connect();
        ioc.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_async(test);
}


BOOST_AUTO_TEST_CASE( pub_qos2_sub_qos2 ) {
    auto test = [](boost::asio::io_context& ioc, auto& c, auto finish, auto& /*b*/) {
        using packet_id_t = typename std::remove_reference_t<decltype(*c)>::packet_id_t;
        c->set_clean_session(true);

        std::uint16_t pid_pub;
        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;


        checker chk = {
            // connect
            cont("h_connack"),
            // subscribe topic1 QoS0
            cont("h_suback"),
            // publish topic1 QoS2
            cont("h_publish"),
            cont("h_pubrec"),
            cont("h_pub_res_sent"),
            deps("h_pubcomp", "h_pubrec"),
            cont("h_unsuback"),
            // disconnect
            cont("h_close"),
        };

        MQTT_NS::optional<packet_id_t> recv_packet_id;
        c->set_pub_res_sent_handler(
            [&chk, &recv_packet_id]
            (packet_id_t packet_id) {
                MQTT_CHK("h_pub_res_sent");
                BOOST_TEST(*recv_packet_id == packet_id);
            });

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c, &pid_sub]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_sub = c->acquire_unique_packet_id();
                    c->async_subscribe(
                        pid_sub,
                        as::buffer(*topic1),
                        MQTT_NS::qos::exactly_once,
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_puback_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_pubrec_handler(
                [&chk, &pid_pub]
                (packet_id_t packet_id) {
                    MQTT_CHK("h_pubrec");
                    BOOST_TEST(packet_id == pid_pub);
                    return true;
                });
            c->set_pubcomp_handler(
                [&chk, &c, &pid_pub, &pid_unsub]
                (packet_id_t packet_id) {
                    MQTT_CHK("h_pubcomp");
                    BOOST_TEST(packet_id == pid_pub);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_unsub = c->acquire_unique_packet_id();
                    c->async_unsubscribe(
                        pid_unsub,
                        as::buffer(*topic1),
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_suback_handler(
                [&chk, &c, &pid_sub, &pid_pub]
                (packet_id_t packet_id, std::vector<MQTT_NS::suback_return_code> results) {
                    MQTT_CHK("h_suback");
                    BOOST_TEST(packet_id == pid_sub);
                    BOOST_TEST(results.size() == 1U);
                    BOOST_TEST(results[0] == MQTT_NS::suback_return_code::success_maximum_qos_2);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    auto contents = std::make_shared<std::string>("topic1_contents");
                    pid_pub = c->acquire_unique_packet_id();
                    c->async_publish(
                        pid_pub,
                        as::buffer(*topic1),
                        as::buffer(*contents),
                        std::make_pair(topic1, contents),
                        MQTT_NS::qos::exactly_once
                    );
                    return true;
                });
            c->set_unsuback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id) {
                    MQTT_CHK("h_unsuback");
                    BOOST_TEST(packet_id == pid_unsub);
                    c->async_disconnect();
                    return true;
                });
            c->set_publish_handler(
                [&chk, &recv_packet_id]
                (bool is_dup,
                 MQTT_NS::qos qos_value,
                 bool is_retain,
                 MQTT_NS::optional<packet_id_t> packet_id,
                 MQTT_NS::buffer topic,
                 MQTT_NS::buffer contents) {
                    MQTT_CHK("h_publish");
                    BOOST_TEST(is_dup == false);
                    BOOST_TEST(qos_value == MQTT_NS::qos::exactly_once);
                    BOOST_TEST(is_retain == false);
                    BOOST_TEST(*packet_id != 0);
                    recv_packet_id = packet_id;
                    BOOST_TEST(topic == "topic1");
                    BOOST_TEST(contents == "topic1_contents");
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c, &pid_sub]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_sub = c->acquire_unique_packet_id();
                    c->async_subscribe(
                        pid_sub,
                        as::buffer(*topic1),
                        MQTT_NS::qos::exactly_once,
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_v5_puback_handler(
                []
                (packet_id_t, MQTT_NS::v5::puback_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_pubrec_handler(
                [&chk, &pid_pub]
                (packet_id_t packet_id, MQTT_NS::v5::pubrec_reason_code, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_pubrec");
                    BOOST_TEST(packet_id == pid_pub);
                    return true;
                });
            c->set_v5_pubcomp_handler(
                [&chk, &c, &pid_pub, &pid_unsub]
                (packet_id_t packet_id, MQTT_NS::v5::pubcomp_reason_code, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_pubcomp");
                    BOOST_TEST(packet_id == pid_pub);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_unsub = c->acquire_unique_packet_id();
                    c->async_unsubscribe(
                        pid_unsub,
                        as::buffer(*topic1),
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_v5_suback_handler(
                [&chk, &c, &pid_sub, &pid_pub]
                (packet_id_t packet_id, std::vector<MQTT_NS::v5::suback_reason_code> reasons, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_suback");
                    BOOST_TEST(packet_id == pid_sub);
                    BOOST_TEST(reasons.size() == 1U);
                    BOOST_TEST(reasons[0] == MQTT_NS::v5::suback_reason_code::granted_qos_2);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    auto contents = std::make_shared<std::string>("topic1_contents");
                    pid_pub = c->acquire_unique_packet_id();
                    c->async_publish(
                        pid_pub,
                        as::buffer(*topic1),
                        as::buffer(*contents),
                        std::make_pair(topic1, contents),
                        MQTT_NS::qos::exactly_once
                    );
                    return true;
                });
            c->set_v5_unsuback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id, std::vector<MQTT_NS::v5::unsuback_reason_code> reasons, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_unsuback");
                    BOOST_TEST(packet_id == pid_unsub);
                    BOOST_TEST(reasons.size() == 1U);
                    BOOST_TEST(reasons[0] == MQTT_NS::v5::unsuback_reason_code::success);
                    c->async_disconnect();
                    return true;
                });
            c->set_v5_publish_handler(
                [&chk, &recv_packet_id]
                (bool is_dup,
                 MQTT_NS::qos qos_value,
                 bool is_retain,
                 MQTT_NS::optional<packet_id_t> packet_id,
                 MQTT_NS::buffer topic,
                 MQTT_NS::buffer contents,
                 MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_publish");
                    BOOST_TEST(is_dup == false);
                    BOOST_TEST(qos_value == MQTT_NS::qos::exactly_once);
                    BOOST_TEST(is_retain == false);
                    BOOST_TEST(*packet_id != 0);
                    recv_packet_id = packet_id;
                    BOOST_TEST(topic == "topic1");
                    BOOST_TEST(contents == "topic1_contents");
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }
        c->set_close_handler(
            [&chk, &finish]
            () {
                MQTT_CHK("h_close");
                finish();
            });
        c->set_error_handler(
            []
            (MQTT_NS::error_code) {
                BOOST_CHECK(false);
            });
        c->async_connect();
        ioc.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_async(test);
}

BOOST_AUTO_TEST_CASE( publish_function ) {
    auto test = [](boost::asio::io_context& ioc, auto& c, auto finish, auto& /*b*/) {
        using packet_id_t = typename std::remove_reference_t<decltype(*c)>::packet_id_t;
        c->set_clean_session(true);

        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;


        checker chk = {
            // connect
            cont("h_connack"),
            // subscribe topic1 QoS0
            cont("h_suback"),
            // publish topic1 QoS0
            cont("h_publish"),
            cont("h_unsuback"),
            // disconnect
            cont("h_close"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c, &pid_sub]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_sub = c->acquire_unique_packet_id();
                    c->async_subscribe(
                        pid_sub,
                        as::buffer(*topic1),
                        MQTT_NS::qos::at_most_once,
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_puback_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_pubrec_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_pubcomp_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_suback_handler(
                [&chk, &c, &pid_sub]
                (packet_id_t packet_id, std::vector<MQTT_NS::suback_return_code> results) {
                    MQTT_CHK("h_suback");
                    BOOST_TEST(packet_id == pid_sub);
                    BOOST_TEST(results.size() == 1U);
                    BOOST_TEST(results[0] == MQTT_NS::suback_return_code::success_maximum_qos_0);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    auto contents = std::make_shared<std::string>("topic1_contents");
                    c->async_publish(
                        as::buffer(*topic1),
                        as::buffer(*contents),
                        std::make_pair(topic1, contents),
                        MQTT_NS::qos::at_most_once
                    );
                    return true;
                });
            c->set_unsuback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id) {
                    MQTT_CHK("h_unsuback");
                    BOOST_TEST(packet_id == pid_unsub);
                    c->async_disconnect();
                    return true;
                });
            c->set_publish_handler(
                [&chk, &c, &pid_unsub]
                (bool is_dup,
                 MQTT_NS::qos qos_value,
                 bool is_retain,
                 MQTT_NS::optional<packet_id_t> packet_id,
                 MQTT_NS::buffer topic,
                 MQTT_NS::buffer contents) {
                    MQTT_CHK("h_publish");
                    BOOST_TEST(is_dup == false);
                    BOOST_TEST(qos_value == MQTT_NS::qos::at_most_once);
                    BOOST_TEST(is_retain == false);
                    BOOST_CHECK(!packet_id);
                    BOOST_TEST(topic == "topic1");
                    BOOST_TEST(contents == "topic1_contents");
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_unsub = c->acquire_unique_packet_id();
                    c->async_unsubscribe(
                        pid_unsub,
                        as::buffer(*topic1),
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c, &pid_sub]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_sub = c->acquire_unique_packet_id();
                    c->async_subscribe(
                        pid_sub,
                        as::buffer(*topic1),
                        MQTT_NS::qos::at_most_once,
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_v5_puback_handler(
                []
                (packet_id_t, MQTT_NS::v5::puback_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_pubrec_handler(
                []
                (packet_id_t, MQTT_NS::v5::pubrec_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_pubcomp_handler(
                []
                (packet_id_t, MQTT_NS::v5::pubcomp_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_suback_handler(
                [&chk, &c, &pid_sub]
                (packet_id_t packet_id, std::vector<MQTT_NS::v5::suback_reason_code> reasons, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_suback");
                    BOOST_TEST(packet_id == pid_sub);
                    BOOST_TEST(reasons.size() == 1U);
                    BOOST_TEST(reasons[0] == MQTT_NS::v5::suback_reason_code::granted_qos_0);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    auto contents = std::make_shared<std::string>("topic1_contents");
                    c->async_publish(
                        as::buffer(*topic1),
                        as::buffer(*contents),
                        std::make_pair(topic1, contents),
                        MQTT_NS::qos::at_most_once
                    );
                    return true;
                });
            c->set_v5_unsuback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id, std::vector<MQTT_NS::v5::unsuback_reason_code> reasons, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_unsuback");
                    BOOST_TEST(packet_id == pid_unsub);
                    BOOST_TEST(reasons.size() == 1U);
                    BOOST_TEST(reasons[0] == MQTT_NS::v5::unsuback_reason_code::success);
                    c->async_disconnect();
                    return true;
                });
            c->set_v5_publish_handler(
                [&chk, &c, &pid_unsub]
                (bool is_dup,
                 MQTT_NS::qos qos_value,
                 bool is_retain,
                 MQTT_NS::optional<packet_id_t> packet_id,
                 MQTT_NS::buffer topic,
                 MQTT_NS::buffer contents,
                 MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_publish");
                    BOOST_TEST(is_dup == false);
                    BOOST_TEST(qos_value == MQTT_NS::qos::at_most_once);
                    BOOST_TEST(is_retain == false);
                    BOOST_CHECK(!packet_id);
                    BOOST_TEST(topic == "topic1");
                    BOOST_TEST(contents == "topic1_contents");
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_unsub = c->acquire_unique_packet_id();
                    c->async_unsubscribe(
                        pid_unsub,
                        as::buffer(*topic1),
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }
        c->set_close_handler(
            [&chk, &finish]
            () {
                MQTT_CHK("h_close");
                finish();
            });
        c->set_error_handler(
            []
            (MQTT_NS::error_code) {
                BOOST_CHECK(false);
            });
        c->async_connect();
        ioc.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_async(test);
}

BOOST_AUTO_TEST_CASE( publish_function_buffer ) {
    auto test = [](boost::asio::io_context& ioc, auto& c, auto finish, auto& /*b*/) {
        using packet_id_t = typename std::remove_reference_t<decltype(*c)>::packet_id_t;
        c->set_clean_session(true);

        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;


        checker chk = {
            // connect
            cont("h_connack"),
            // subscribe topic1 QoS0
            cont("h_suback"),
            // publish topic1 QoS0
            cont("h_publish"),
            cont("h_unsuback"),
            // disconnect
            cont("h_close"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c, &pid_sub]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    auto topic1 = MQTT_NS::allocate_buffer("topic1");
                    pid_sub = c->acquire_unique_packet_id();
                    c->async_subscribe(
                        pid_sub,
                        std::move(topic1),
                        MQTT_NS::qos::at_most_once,
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_puback_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_pubrec_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_pubcomp_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_suback_handler(
                [&chk, &c, &pid_sub]
                (packet_id_t packet_id, std::vector<MQTT_NS::suback_return_code> results) {
                    MQTT_CHK("h_suback");
                    BOOST_TEST(packet_id == pid_sub);
                    BOOST_TEST(results.size() == 1U);
                    BOOST_TEST(results[0] == MQTT_NS::suback_return_code::success_maximum_qos_0);
                    auto topic1 = MQTT_NS::allocate_buffer("topic1");
                    auto contents = MQTT_NS::allocate_buffer("topic1_contents");
                    c->async_publish(
                        std::move(topic1),
                        std::move(contents),
                        MQTT_NS::qos::at_most_once
                    );
                    return true;
                });
            c->set_unsuback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id) {
                    MQTT_CHK("h_unsuback");
                    BOOST_TEST(packet_id == pid_unsub);
                    c->async_disconnect();
                    return true;
                });
            c->set_publish_handler(
                [&chk, &c, &pid_unsub]
                (bool is_dup,
                 MQTT_NS::qos qos_value,
                 bool is_retain,
                 MQTT_NS::optional<packet_id_t> packet_id,
                 MQTT_NS::buffer topic,
                 MQTT_NS::buffer contents) {
                    MQTT_CHK("h_publish");
                    BOOST_TEST(is_dup == false);
                    BOOST_TEST(qos_value == MQTT_NS::qos::at_most_once);
                    BOOST_TEST(is_retain == false);
                    BOOST_CHECK(!packet_id);
                    BOOST_TEST(topic == "topic1");
                    BOOST_TEST(contents == "topic1_contents");
                    auto topic1 = MQTT_NS::allocate_buffer("topic1");
                    pid_unsub = c->acquire_unique_packet_id();
                    c->async_unsubscribe(
                        pid_unsub,
                        std::move(topic1),
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c, &pid_sub]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    auto topic1 = MQTT_NS::allocate_buffer("topic1");
                    pid_sub = c->acquire_unique_packet_id();
                    c->async_subscribe(
                        pid_sub,
                        std::move(topic1),
                        MQTT_NS::qos::at_most_once,
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_v5_puback_handler(
                []
                (packet_id_t, MQTT_NS::v5::puback_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_pubrec_handler(
                []
                (packet_id_t, MQTT_NS::v5::pubrec_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_pubcomp_handler(
                []
                (packet_id_t, MQTT_NS::v5::pubcomp_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_suback_handler(
                [&chk, &c, &pid_sub]
                (packet_id_t packet_id, std::vector<MQTT_NS::v5::suback_reason_code> reasons, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_suback");
                    BOOST_TEST(packet_id == pid_sub);
                    BOOST_TEST(reasons.size() == 1U);
                    BOOST_TEST(reasons[0] == MQTT_NS::v5::suback_reason_code::granted_qos_0);
                    auto topic1 = MQTT_NS::allocate_buffer("topic1");
                    auto contents = MQTT_NS::allocate_buffer("topic1_contents");
                    c->async_publish(
                        std::move(topic1),
                        std::move(contents),
                        MQTT_NS::qos::at_most_once
                    );
                    return true;
                });
            c->set_v5_unsuback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id, std::vector<MQTT_NS::v5::unsuback_reason_code> reasons, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_unsuback");
                    BOOST_TEST(packet_id == pid_unsub);
                    BOOST_TEST(reasons.size() == 1U);
                    BOOST_TEST(reasons[0] == MQTT_NS::v5::unsuback_reason_code::success);
                    c->async_disconnect();
                    return true;
                });
            c->set_v5_publish_handler(
                [&chk, &c, &pid_unsub]
                (bool is_dup,
                 MQTT_NS::qos qos_value,
                 bool is_retain,
                 MQTT_NS::optional<packet_id_t> packet_id,
                 MQTT_NS::buffer topic,
                 MQTT_NS::buffer contents,
                 MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_publish");
                    BOOST_TEST(is_dup == false);
                    BOOST_TEST(qos_value == MQTT_NS::qos::at_most_once);
                    BOOST_TEST(is_retain == false);
                    BOOST_CHECK(!packet_id);
                    BOOST_TEST(topic == "topic1");
                    BOOST_TEST(contents == "topic1_contents");
                    auto topic1 = MQTT_NS::allocate_buffer("topic1");
                    pid_unsub = c->acquire_unique_packet_id();
                    c->async_unsubscribe(
                        pid_unsub,
                        std::move(topic1),
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }
        c->set_close_handler(
            [&chk, &finish]
            () {
                MQTT_CHK("h_close");
                finish();
            });
        c->set_error_handler(
            []
            (MQTT_NS::error_code) {
                BOOST_CHECK(false);
            });
        c->async_connect();
        ioc.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_async(test);
}

BOOST_AUTO_TEST_CASE( publish_dup_function ) {
    auto test = [](boost::asio::io_context& ioc, auto& c, auto finish, auto& /*b*/) {
        using packet_id_t = typename std::remove_reference_t<decltype(*c)>::packet_id_t;
        c->set_clean_session(true);

        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;


        checker chk = {
            // connect
            cont("h_connack"),
            // subscribe topic1 QoS1
            cont("h_suback"),
            // publish topic1 QoS1
            cont("h_publish"),
            cont("h_puback"),
            cont("h_unsuback"),
            // disconnect
            cont("h_close"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c, &pid_sub]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_sub = c->acquire_unique_packet_id();
                    c->async_subscribe(
                        pid_sub,
                        as::buffer(*topic1),
                        MQTT_NS::qos::at_least_once,
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_puback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id) {
                    MQTT_CHK("h_puback");
                    BOOST_TEST(packet_id == 1);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_unsub = c->acquire_unique_packet_id();
                    c->async_unsubscribe(
                        pid_unsub,
                        as::buffer(*topic1),
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_pubrec_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_pubcomp_handler(
                []
                (std::uint16_t) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_suback_handler(
                [&chk, &c, &pid_sub]
                (packet_id_t packet_id, std::vector<MQTT_NS::suback_return_code> results) {
                    MQTT_CHK("h_suback");
                    BOOST_TEST(packet_id == pid_sub);
                    BOOST_TEST(results.size() == 1U);
                    BOOST_TEST(results[0] == MQTT_NS::suback_return_code::success_maximum_qos_1);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    auto contents = std::make_shared<std::string>("topic1_contents");
                    c->register_packet_id(1);
                    c->async_publish_dup(
                        1,
                        as::buffer(*topic1),
                        as::buffer(*contents),
                        std::make_pair(topic1, contents),
                        MQTT_NS::qos::at_least_once
                    );
                    return true;
                });
            c->set_unsuback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id) {
                    MQTT_CHK("h_unsuback");
                    BOOST_TEST(packet_id == pid_unsub);
                    c->async_disconnect();
                    return true;
                });
            c->set_publish_handler(
                [&chk]
                (bool is_dup,
                 MQTT_NS::qos qos_value,
                 bool is_retain,
                 MQTT_NS::optional<packet_id_t> packet_id,
                 MQTT_NS::buffer topic,
                 MQTT_NS::buffer contents) {
                    MQTT_CHK("h_publish");
                    BOOST_TEST(is_dup == false); // not propagated
                    BOOST_TEST(qos_value == MQTT_NS::qos::at_least_once);
                    BOOST_TEST(is_retain == false);
                    BOOST_CHECK(packet_id.value() == 1);
                    BOOST_TEST(topic == "topic1");
                    BOOST_TEST(contents == "topic1_contents");
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c, &pid_sub]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_sub = c->acquire_unique_packet_id();
                    c->async_subscribe(
                        pid_sub,
                        as::buffer(*topic1),
                        MQTT_NS::qos::at_least_once,
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_v5_puback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id, MQTT_NS::v5::puback_reason_code, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_puback");
                    BOOST_TEST(packet_id == 1);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    pid_unsub = c->acquire_unique_packet_id();
                    c->async_unsubscribe(
                        pid_unsub,
                        as::buffer(*topic1),
                        [topic1](MQTT_NS::error_code) {}
                    );
                    return true;
                });
            c->set_v5_pubrec_handler(
                []
                (packet_id_t, MQTT_NS::v5::pubrec_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_pubcomp_handler(
                []
                (packet_id_t, MQTT_NS::v5::pubcomp_reason_code, MQTT_NS::v5::properties /*props*/) {
                    BOOST_CHECK(false);
                    return true;
                });
            c->set_v5_suback_handler(
                [&chk, &c, &pid_sub]
                (packet_id_t packet_id, std::vector<MQTT_NS::v5::suback_reason_code> reasons, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_suback");
                    BOOST_TEST(packet_id == pid_sub);
                    BOOST_TEST(reasons.size() == 1U);
                    BOOST_TEST(reasons[0] == MQTT_NS::v5::suback_reason_code::granted_qos_1);
                    auto topic1 = std::make_shared<std::string>("topic1");
                    auto contents = std::make_shared<std::string>("topic1_contents");
                    c->register_packet_id(1);
                    c->async_publish_dup(
                        1,
                        as::buffer(*topic1),
                        as::buffer(*contents),
                        std::make_pair(topic1, contents),
                        MQTT_NS::qos::at_least_once
                    );
                    return true;
                });
            c->set_v5_unsuback_handler(
                [&chk, &c, &pid_unsub]
                (packet_id_t packet_id, std::vector<MQTT_NS::v5::unsuback_reason_code> reasons, MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_unsuback");
                    BOOST_TEST(packet_id == pid_unsub);
                    BOOST_TEST(reasons.size() == 1U);
                    BOOST_TEST(reasons[0] == MQTT_NS::v5::unsuback_reason_code::success);
                    c->async_disconnect();
                    return true;
                });
            c->set_v5_publish_handler(
                [&chk]
                (bool is_dup,
                 MQTT_NS::qos qos_value,
                 bool is_retain,
                 MQTT_NS::optional<packet_id_t> packet_id,
                 MQTT_NS::buffer topic,
                 MQTT_NS::buffer contents,
                 MQTT_NS::v5::properties /*props*/) {
                    MQTT_CHK("h_publish");
                    BOOST_TEST(is_dup == false); // not propagated
                    BOOST_TEST(qos_value == MQTT_NS::qos::at_least_once);
                    BOOST_TEST(is_retain == false);
                    BOOST_CHECK(packet_id.value() == 1);
                    BOOST_TEST(topic == "topic1");
                    BOOST_TEST(contents == "topic1_contents");
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }
        c->set_close_handler(
            [&chk, &finish]
            () {
                MQTT_CHK("h_close");
                finish();
            });
        c->set_error_handler(
            []
            (MQTT_NS::error_code) {
                BOOST_CHECK(false);
            });
        c->async_connect();
        ioc.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_async(test);
}

BOOST_AUTO_TEST_SUITE_END()
