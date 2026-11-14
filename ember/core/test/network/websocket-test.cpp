/*
 * ------------------------------------------------------------------------
 * 名称：websocket-test.cpp
 * 说明：WebSocket 纯逻辑的测试：SHA-1 与 Base64 的已知值、RFC 6455 的握手示例、握手请求的解析与检查（Host、Origin、版本、密钥）、帧的编码与解码（长度档位、掩码、分片、控制帧、保留位、超长、非法 UTF-8）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "platform/network/websocket.hpp"

namespace {

namespace ws = ember::platform::websocket;

std::string hex(const std::array<std::uint8_t, 20>& digest) {
  static constexpr char DIGITS[] = "0123456789abcdef";
  std::string text;
  for (const auto byte : digest) {
    text.push_back(DIGITS[byte >> 4]);
    text.push_back(DIGITS[byte & 15]);
  }
  return text;
}

std::vector<std::uint8_t> bytes(const std::string& text) { return {text.begin(), text.end()}; }

constexpr std::array<std::uint8_t, 4> MASK{0x37, 0xFA, 0x21, 0x3D};

/** 客户端帧：带掩码。 */
std::vector<std::uint8_t> clientFrame(ws::Opcode opcode, const std::vector<std::uint8_t>& payload) { return ws::encodeFrame(opcode, payload, MASK); }

const std::string VALID_REQUEST =
    "GET / HTTP/1.1\r\nHost: 127.0.0.1:5688\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";

ws::HandshakeCheck check(const std::string& request, std::uint16_t port = 5688) {
  const auto parsed = ws::parseHandshake(request);
  EXPECT_EQ(parsed.status, ws::HandshakeParse::Status::Complete);
  return ws::checkHandshake(parsed.request, {port, "/"});
}

TEST(WebSocketHash, Sha1MatchesKnownVectors) {
  EXPECT_EQ(hex(ws::sha1("")), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
  EXPECT_EQ(hex(ws::sha1("abc")), "a9993e364706816aba3e25717850c26c9cd0d89d");
  EXPECT_EQ(hex(ws::sha1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")), "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
  EXPECT_EQ(hex(ws::sha1(std::string(1000, 'a'))), "291e9a6c66994949b57ba5e650361e98fc36b1ba");
}

TEST(WebSocketHash, Base64PadsCorrectly) {
  const auto encode = [](const std::string& text) { return ws::base64Encode(bytes(text)); };
  EXPECT_EQ(encode(""), "");
  EXPECT_EQ(encode("f"), "Zg==");
  EXPECT_EQ(encode("fo"), "Zm8=");
  EXPECT_EQ(encode("foo"), "Zm9v");
  EXPECT_EQ(encode("foobar"), "Zm9vYmFy");
}

TEST(WebSocketHandshake, AcceptKeyMatchesTheRfcExample) { EXPECT_EQ(ws::acceptKey("dGhlIHNhbXBsZSBub25jZQ=="), "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="); }

TEST(WebSocketHandshake, ParsesAHeaderWithoutLosingTheBytesAfterIt) {
  const auto parsed = ws::parseHandshake(VALID_REQUEST + "\x81\x80");
  ASSERT_EQ(parsed.status, ws::HandshakeParse::Status::Complete);
  EXPECT_EQ(parsed.consumed, VALID_REQUEST.size());
  EXPECT_EQ(parsed.request.method, "GET");
  EXPECT_EQ(parsed.request.target, "/");
  EXPECT_EQ(parsed.request.headers.at("host"), "127.0.0.1:5688");
  EXPECT_EQ(parsed.request.headers.at("sec-websocket-version"), "13");
}

TEST(WebSocketHandshake, WaitsForTheRestOfAnIncompleteHeader) {
  EXPECT_EQ(ws::parseHandshake("GET / HTTP/1.1\r\nHost: 127.0.0.1\r\n").status, ws::HandshakeParse::Status::Incomplete);
  EXPECT_EQ(ws::parseHandshake("").status, ws::HandshakeParse::Status::Incomplete);
}

TEST(WebSocketHandshake, RejectsMalformedAndOversizedHeaders) {
  EXPECT_EQ(ws::parseHandshake("garbage\r\n\r\n").status, ws::HandshakeParse::Status::Invalid);
  EXPECT_EQ(ws::parseHandshake("GET / HTTP/1.0\r\n\r\n").status, ws::HandshakeParse::Status::Invalid);
  EXPECT_EQ(ws::parseHandshake("GET / HTTP/1.1\r\nno colon here\r\n\r\n").status, ws::HandshakeParse::Status::Invalid);
  EXPECT_EQ(ws::parseHandshake(std::string(ws::MAX_HANDSHAKE_BYTES + 10, 'a')).status, ws::HandshakeParse::Status::Invalid);
}

TEST(WebSocketHandshake, AcceptsAValidUpgradeAndEchoesTheSubprotocol) {
  auto result = check(VALID_REQUEST);
  EXPECT_TRUE(result.ok);
  EXPECT_EQ(result.accept, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
  EXPECT_FALSE(result.subprotocol);

  const std::string withProtocol = "GET / HTTP/1.1\r\nHost: localhost:5688\r\nUpgrade: WebSocket\r\nConnection: keep-alive, Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Protocol: other, ember-debug.v1\r\n\r\n";
  result = check(withProtocol);
  EXPECT_TRUE(result.ok);
  EXPECT_TRUE(result.subprotocol);
  const auto response = ws::buildHandshakeResponse(result);
  EXPECT_NE(response.find("HTTP/1.1 101 Switching Protocols\r\n"), std::string::npos);
  EXPECT_NE(response.find("Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n"), std::string::npos);
  EXPECT_NE(response.find("Sec-WebSocket-Protocol: ember-debug.v1\r\n"), std::string::npos);
  EXPECT_TRUE(response.ends_with("\r\n\r\n"));
}

TEST(WebSocketHandshake, OnlyLoopbackHostsWithTheServicePortAreAllowed) {
  const auto withHost = [](const std::string& host) {
    return "GET / HTTP/1.1\r\nHost: " + host + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
  };
  EXPECT_TRUE(check(withHost("127.0.0.1:5688")).ok);
  EXPECT_TRUE(check(withHost("LocalHost:5688")).ok);
  EXPECT_TRUE(check(withHost("[::1]:5688")).ok);
  for (const std::string host : {"example.com:5688", "127.0.0.1:1234", "127.0.0.1", "192.168.1.5:5688", "evil.127.0.0.1:5688"}) {
    const auto result = check(withHost(host));
    EXPECT_FALSE(result.ok) << host;
    EXPECT_EQ(result.status, 403) << host;
  }
}

TEST(WebSocketHandshake, ABrowserPageOnAnotherSiteCannotConnect) {
  const auto withOrigin = [](const std::string& origin) {
    return "GET / HTTP/1.1\r\nHost: 127.0.0.1:5688\r\nOrigin: " + origin + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
  };
  for (const std::string origin : {"http://127.0.0.1:3000", "https://localhost", "http://[::1]:8080"}) EXPECT_TRUE(check(withOrigin(origin)).ok) << origin;
  for (const std::string origin : {"https://evil.example", "http://127.0.0.1.evil.example", "http://localhost.evil.example:80", "null", "file://", "ftp://localhost", "http://localhost/path"}) {
    const auto result = check(withOrigin(origin));
    EXPECT_FALSE(result.ok) << origin;
    EXPECT_EQ(result.status, 403) << origin;
  }
}

TEST(WebSocketHandshake, RejectsWrongMethodPathVersionAndKey) {
  const auto replace = [](std::string text, const std::string& from, const std::string& to) {
    text.replace(text.find(from), from.size(), to);
    return text;
  };
  EXPECT_EQ(check(replace(VALID_REQUEST, "GET /", "POST /")).status, 405);
  EXPECT_EQ(check(replace(VALID_REQUEST, "GET /", "GET /other")).status, 404);
  EXPECT_EQ(check(replace(VALID_REQUEST, "Version: 13", "Version: 8")).status, 426);
  EXPECT_EQ(check(replace(VALID_REQUEST, "Upgrade: websocket\r\n", "")).status, 426);
  EXPECT_EQ(check(replace(VALID_REQUEST, "Connection: Upgrade\r\n", "")).status, 426);
  EXPECT_EQ(check(replace(VALID_REQUEST, "dGhlIHNhbXBsZSBub25jZQ==", "short")).status, 400);
  EXPECT_NE(ws::buildHttpError(426, "x").find("Sec-WebSocket-Version: 13"), std::string::npos);
  EXPECT_TRUE(ws::buildHttpError(403, "禁止").starts_with("HTTP/1.1 403 Forbidden\r\n"));
}

TEST(WebSocketFrames, EncodesTheThreeLengthForms) {
  const std::vector<std::uint8_t> small(5, 'a');
  auto frame = ws::encodeFrame(ws::Opcode::Binary, small);
  EXPECT_EQ(frame[0], 0x82);
  EXPECT_EQ(frame[1], 5);
  EXPECT_EQ(frame.size(), 7u);

  const std::vector<std::uint8_t> medium(300, 'a');
  frame = ws::encodeFrame(ws::Opcode::Text, medium);
  EXPECT_EQ(frame[1], 126);
  EXPECT_EQ((frame[2] << 8) | frame[3], 300);
  EXPECT_EQ(frame.size(), 304u);

  const std::vector<std::uint8_t> large(70000, 'a');
  frame = ws::encodeFrame(ws::Opcode::Text, large);
  EXPECT_EQ(frame[1], 127);
  EXPECT_EQ(frame.size(), 70010u);
  EXPECT_EQ(frame[2], 0);
  EXPECT_EQ((frame[7] << 16) | (frame[8] << 8) | frame[9], 70000);
}

TEST(WebSocketFrames, DecodesMaskedFramesOfEveryLengthForm) {
  for (const std::size_t size : {std::size_t{0}, std::size_t{5}, std::size_t{125}, std::size_t{126}, std::size_t{300}, std::size_t{65535}, std::size_t{65536}, std::size_t{100000}}) {
    ws::FrameDecoder decoder(1 << 20);
    std::vector<std::uint8_t> payload(size);
    for (std::size_t i = 0; i < size; ++i) payload[i] = static_cast<std::uint8_t>('a' + i % 26);
    decoder.feed(clientFrame(ws::Opcode::Binary, payload));
    const auto message = decoder.next();
    ASSERT_TRUE(message.has_value()) << size;
    EXPECT_EQ(message->opcode, ws::Opcode::Binary);
    EXPECT_EQ(message->payload, payload) << size;
    EXPECT_FALSE(decoder.next().has_value());
  }
}

TEST(WebSocketFrames, ReassemblesFramesThatArriveByteByByte) {
  ws::FrameDecoder decoder(1024);
  const auto frame = clientFrame(ws::Opcode::Text, bytes("你好，世界"));
  for (std::size_t i = 0; i + 1 < frame.size(); ++i) {
    decoder.feed(std::span(&frame[i], 1));
    EXPECT_FALSE(decoder.next().has_value());
  }
  decoder.feed(std::span(&frame.back(), 1));
  const auto message = decoder.next();
  ASSERT_TRUE(message.has_value());
  EXPECT_EQ(std::string(message->payload.begin(), message->payload.end()), "你好，世界");
}

TEST(WebSocketFrames, DecodesSeveralFramesInOneChunk) {
  ws::FrameDecoder decoder(1024);
  auto chunk = clientFrame(ws::Opcode::Text, bytes("one"));
  const auto second = clientFrame(ws::Opcode::Text, bytes("two"));
  chunk.insert(chunk.end(), second.begin(), second.end());
  decoder.feed(chunk);
  EXPECT_EQ(decoder.next()->payload, bytes("one"));
  EXPECT_EQ(decoder.next()->payload, bytes("two"));
  EXPECT_FALSE(decoder.next().has_value());
  EXPECT_EQ(decoder.status(), ws::FrameDecoder::Status::Ok);
}

/** 手工构造一个带掩码的帧：可以控制 FIN 与保留位。 */
std::vector<std::uint8_t> rawFrame(std::uint8_t first, const std::vector<std::uint8_t>& payload, bool masked = true) {
  std::vector<std::uint8_t> frame{first, static_cast<std::uint8_t>((masked ? 0x80 : 0) | payload.size())};
  if (masked) frame.insert(frame.end(), MASK.begin(), MASK.end());
  for (std::size_t i = 0; i < payload.size(); ++i) frame.push_back(masked ? payload[i] ^ MASK[i % 4] : payload[i]);
  return frame;
}

TEST(WebSocketFrames, JoinsFragmentsAndAllowsControlFramesBetweenThem) {
  ws::FrameDecoder decoder(1024);
  decoder.feed(rawFrame(0x01, bytes("hel")));
  decoder.feed(rawFrame(0x89, bytes("p")));
  decoder.feed(rawFrame(0x80, bytes("lo")));
  const auto ping = decoder.next();
  ASSERT_TRUE(ping.has_value());
  EXPECT_EQ(ping->opcode, ws::Opcode::Ping);
  const auto text = decoder.next();
  ASSERT_TRUE(text.has_value());
  EXPECT_EQ(text->opcode, ws::Opcode::Text);
  EXPECT_EQ(text->payload, bytes("hello"));
}

TEST(WebSocketFrames, RejectsProtocolViolations) {
  const auto status = [](const std::vector<std::uint8_t>& frame) {
    ws::FrameDecoder decoder(1024);
    decoder.feed(frame);
    EXPECT_FALSE(decoder.next().has_value());
    return decoder.status();
  };
  using Status = ws::FrameDecoder::Status;
  EXPECT_EQ(status(rawFrame(0x81, bytes("a"), false)), Status::ProtocolError);   // 客户端的帧必须带掩码
  EXPECT_EQ(status(rawFrame(0xC1, bytes("a"))), Status::ProtocolError);          // 保留位
  EXPECT_EQ(status(rawFrame(0x83, bytes("a"))), Status::ProtocolError);          // 未知操作码
  EXPECT_EQ(status(rawFrame(0x80, bytes("a"))), Status::ProtocolError);          // 没有起头的分片
  EXPECT_EQ(status(rawFrame(0x09, bytes(""))), Status::ProtocolError);           // 控制帧不能分片
  EXPECT_EQ(status(rawFrame(0x88, bytes("a"))), Status::ProtocolError);          // 关闭帧负载只有 1 字节
  EXPECT_EQ(status(ws::encodeFrame(ws::Opcode::Ping, std::vector<std::uint8_t>(126, 'a'), MASK)), Status::ProtocolError);  // 控制帧过长

  ws::FrameDecoder interleaved(1024);
  interleaved.feed(rawFrame(0x01, bytes("a")));
  interleaved.feed(rawFrame(0x01, bytes("b")));
  EXPECT_FALSE(interleaved.next().has_value());
  EXPECT_EQ(interleaved.status(), Status::ProtocolError);
}

TEST(WebSocketFrames, RejectsOversizedMessagesBeforeBufferingThem) {
  ws::FrameDecoder decoder(100);
  decoder.feed(clientFrame(ws::Opcode::Binary, std::vector<std::uint8_t>(101, 'a')));
  EXPECT_FALSE(decoder.next().has_value());
  EXPECT_EQ(decoder.status(), ws::FrameDecoder::Status::TooBig);

  // 分片合并后超限同样拒绝。
  ws::FrameDecoder fragmented(100);
  fragmented.feed(rawFrame(0x02, std::vector<std::uint8_t>(60, 'a')));
  fragmented.feed(rawFrame(0x80, std::vector<std::uint8_t>(60, 'a')));
  EXPECT_FALSE(fragmented.next().has_value());
  EXPECT_EQ(fragmented.status(), ws::FrameDecoder::Status::TooBig);

  // 只声明了很大的长度、还没有发数据时就拒绝。
  ws::FrameDecoder declared(100);
  std::vector<std::uint8_t> header{0x82, 0xFF, 0, 0, 0, 0, 0, 0x10, 0, 0};
  header.insert(header.end(), MASK.begin(), MASK.end());
  declared.feed(header);
  EXPECT_FALSE(declared.next().has_value());
  EXPECT_EQ(declared.status(), ws::FrameDecoder::Status::TooBig);
}

TEST(WebSocketFrames, RejectsInvalidUtf8InTextMessages) {
  ws::FrameDecoder decoder(1024);
  decoder.feed(clientFrame(ws::Opcode::Text, {0xC3, 0x28}));
  EXPECT_FALSE(decoder.next().has_value());
  EXPECT_EQ(decoder.status(), ws::FrameDecoder::Status::InvalidPayload);

  // 二进制消息不检查。
  ws::FrameDecoder binary(1024);
  binary.feed(clientFrame(ws::Opcode::Binary, {0xC3, 0x28}));
  EXPECT_TRUE(binary.next().has_value());
}

TEST(WebSocketFrames, Utf8ValidationFollowsTheStandard) {
  const auto valid = [](std::vector<std::uint8_t> data) { return ws::isValidUtf8(data); };
  EXPECT_TRUE(valid({}));
  EXPECT_TRUE(valid(bytes("ASCII 与 中文 与 😀")));
  EXPECT_FALSE(valid({0x80}));                  // 孤立的续字节
  EXPECT_FALSE(valid({0xC0, 0x80}));            // 过长编码
  EXPECT_FALSE(valid({0xE0, 0x80, 0x80}));      // 过长编码
  EXPECT_FALSE(valid({0xED, 0xA0, 0x80}));      // 代理对
  EXPECT_FALSE(valid({0xF4, 0x90, 0x80, 0x80}));  // 超过 U+10FFFF
  EXPECT_FALSE(valid({0xE4, 0xB8}));            // 被截断
  EXPECT_FALSE(valid({0xFF}));
}

TEST(WebSocketFrames, CloseFramesCarryTheCodeAndReason) {
  const auto frame = ws::encodeClose(ws::CLOSE_UNAUTHORIZED, "令牌不对");
  EXPECT_EQ(frame[0], 0x88);
  EXPECT_EQ((frame[2] << 8) | frame[3], 4001);
  EXPECT_EQ(std::string(frame.begin() + 4, frame.end()), "令牌不对");
}

}  // namespace
