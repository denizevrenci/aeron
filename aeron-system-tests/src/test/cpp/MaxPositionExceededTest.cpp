#include <cassert>
#include <thread>
#include <type_traits>

#include <gtest/gtest.h>

#include <Aeron.h>

constexpr auto message_size = 1024u;
constexpr auto payload_size = message_size - 32u;
constexpr auto msg_count_until_max_pos = 100u;

constexpr auto max_pos_mult_pow = 31;

constexpr int64_t get_max_position(const unsigned int term_buf_len_pow) {
    return 1L << (term_buf_len_pow + max_pos_mult_pow);
}

template <typename Publication>
int64_t offer(Publication& pub, const unsigned int value) {
    uint8_t buf[payload_size];
    aeron::concurrent::AtomicBuffer transport_buf{buf, payload_size};
    transport_buf.overlayStruct<std::remove_const<decltype(value)>::type>(0) = value;
    auto result = pub.offer(transport_buf);
    while (result == aeron::BACK_PRESSURED)
        result = pub.offer(transport_buf);
    return result;
}

template <typename Publication>
int64_t claim(Publication& pub, const unsigned int value) {
    aeron::concurrent::logbuffer::BufferClaim claim;
    auto result = pub.tryClaim(payload_size, claim);
    while (result == aeron::BACK_PRESSURED)
        result = pub.tryClaim(payload_size, claim);
    if (result) {
        claim.buffer().overlayStruct<std::remove_const<decltype(value)>::type>(claim.offset()) = value;
        claim.commit();
    }
    return result;
}

template <typename publish_method_t>
void hit_max_position(
        aeron::ExclusivePublication& pub,
        aeron::Subscription& sub,
        const int64_t initial_position,
        const int64_t max_position,
        publish_method_t&& publish) noexcept {
    auto send_value = 0u;
    auto expected_rcv_value = 0u;
    auto expected_position = initial_position;
    while (send_value != msg_count_until_max_pos) {
        auto result = publish(pub, send_value++);
        expected_position += message_size;
        ASSERT_TRUE(result == expected_position);
        sub.poll([&expected_rcv_value] (
                const aeron::concurrent::AtomicBuffer& buffer,
                aeron::util::index_t offset,
                aeron::util::index_t,
                const aeron::Header&) noexcept {
            ASSERT_TRUE(buffer.template overlayStruct<decltype(expected_rcv_value)>(offset) == expected_rcv_value++);
        }, 1);
    }
    ASSERT_TRUE(expected_position == max_position);
    auto result = publish(pub, send_value);
    ASSERT_TRUE(result == aeron::MAX_POSITION_EXCEEDED);
    while (expected_rcv_value != send_value) {
        sub.poll([&expected_rcv_value] (
                const aeron::concurrent::AtomicBuffer& buffer,
                aeron::util::index_t offset,
                aeron::util::index_t,
                const aeron::Header&) noexcept {
            ASSERT_TRUE(buffer.template overlayStruct<decltype(expected_rcv_value)>(offset) == expected_rcv_value++);
        }, 1);
    }
}

template <typename publish_method_t>
void check_mitigation(
        aeron::ExclusivePublication& pub,
        aeron::Subscription& sub,
        publish_method_t&& publish) noexcept {
    auto send_value = 0u;
    auto expected_rcv_value = 0u;
    int64_t expected_position = 0;
    while (send_value != msg_count_until_max_pos) {
        auto result = publish(pub, send_value++);
        expected_position += message_size;
        ASSERT_TRUE(result == expected_position);
        sub.poll([&expected_rcv_value] (
                const aeron::concurrent::AtomicBuffer& buffer,
                aeron::util::index_t offset,
                aeron::util::index_t,
                const aeron::Header&) noexcept {
            ASSERT_TRUE(buffer.template overlayStruct<decltype(expected_rcv_value)>(offset) == expected_rcv_value++);
        }, 1);
    }
    while (expected_rcv_value != send_value) {
        sub.poll([&expected_rcv_value] (
                const aeron::concurrent::AtomicBuffer& buffer,
                aeron::util::index_t offset,
                aeron::util::index_t,
                const aeron::Header&) noexcept {
            ASSERT_TRUE(buffer.template overlayStruct<decltype(expected_rcv_value)>(offset) == expected_rcv_value++);
        }, 1);
    }
}

TEST(SystemTest, mitigateMaxPositionExceeded) {
    constexpr auto max_position = get_max_position(26);
    constexpr int64_t offset = message_size * msg_count_until_max_pos;
    constexpr auto initial_position = max_position - offset;

    aeron::Context c;
    aeron::Aeron client(c);

    auto pub_reg_id = client.addExclusivePublication("aeron:ipc?init-term-id=0|term-length=67108864|term-offset=67006464|term-id=2147483647", 0);
    auto sub_reg_id = client.addSubscription(
        "aeron:ipc",
        0,
        [] (aeron::Image& i) {std::cout << "up " << i.sessionId() << std::endl; },
        [] (aeron::Image& i) {std::cout << "down " << i.sessionId() << std::endl; });
    auto pub = client.findExclusivePublication(pub_reg_id);
    while (!pub) {
        std::this_thread::yield();
        pub = client.findExclusivePublication(pub_reg_id);
    }
    while (!pub->isConnected())
        std::this_thread::yield();
    auto sub = client.findSubscription(sub_reg_id);
    while (!sub) {
        std::this_thread::yield();
        sub = client.findSubscription(sub_reg_id);
    }
    hit_max_position(*pub, *sub, initial_position, max_position, offer<std::decay<decltype(*pub)>::type>);

    pub_reg_id = client.addExclusivePublication("aeron:ipc", 0);
    pub = client.findExclusivePublication(pub_reg_id);
    while (!pub) {
        std::this_thread::yield();
        pub = client.findExclusivePublication(pub_reg_id);
    }
    while (!pub->isConnected())
        std::this_thread::yield();
    check_mitigation(*pub, *sub, offer<std::decay<decltype(*pub)>::type>);
}
