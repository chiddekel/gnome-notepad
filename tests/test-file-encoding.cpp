#include <cassert>
#include <cstdio>

#include "modules/file.h"

using namespace Notepad;

namespace {

void test_bom_detection() {
  {
    std::string data = "\xEF\xBB\xBFhello";
    auto [encoding, line_ending] = detect_encoding(data);
    assert(encoding == Encoding::UTF8BOM);
    (void)line_ending;
  }
  {
    std::string data = std::string("\xFF\xFE", 2) + std::string("h\0e\0", 4);
    auto [encoding, line_ending] = detect_encoding(data);
    assert(encoding == Encoding::UTF16LE);
    (void)line_ending;
  }
  {
    std::string data = std::string("\xFE\xFF", 2) + std::string("\0h\0e", 4);
    auto [encoding, line_ending] = detect_encoding(data);
    assert(encoding == Encoding::UTF16BE);
    (void)line_ending;
  }
  {
    std::string data = "plain ascii, valid utf-8";
    auto [encoding, line_ending] = detect_encoding(data);
    assert(encoding == Encoding::UTF8);
    (void)line_ending;
  }
  {
    // 0x92 alone is not valid UTF-8 -> falls back to ANSI, matching
    // legacy-notepad's MultiByteToWideChar(MB_ERR_INVALID_CHARS) probe.
    std::string data = "caf\x92";
    auto [encoding, line_ending] = detect_encoding(data);
    assert(encoding == Encoding::ANSI);
    (void)line_ending;
  }
  std::puts("test_bom_detection: OK");
}

void test_line_ending_detection() {
  assert(detect_encoding("a\r\nb").second == LineEnding::CRLF);
  assert(detect_encoding("a\nb").second == LineEnding::LF);
  assert(detect_encoding("a\rb").second == LineEnding::CR);
  assert(detect_encoding("noeol").second == LineEnding::CRLF); // legacy default
  std::puts("test_line_ending_detection: OK");
}

void test_utf8_round_trip() {
  Glib::ustring text = "hello \xE2\x9C\x93 world\nsecond line";
  std::string encoded = encode_text(text, Encoding::UTF8, LineEnding::LF);
  Glib::ustring decoded = decode_text(encoded, Encoding::UTF8);
  assert(decoded == text);
  std::puts("test_utf8_round_trip: OK");
}

void test_utf8bom_round_trip() {
  Glib::ustring text = "with a BOM";
  std::string encoded = encode_text(text, Encoding::UTF8BOM, LineEnding::LF);
  assert(encoded.size() >= 3);
  assert(static_cast<unsigned char>(encoded[0]) == 0xEF);
  assert(static_cast<unsigned char>(encoded[1]) == 0xBB);
  assert(static_cast<unsigned char>(encoded[2]) == 0xBF);
  auto [encoding, line_ending] = detect_encoding(encoded);
  assert(encoding == Encoding::UTF8BOM);
  (void)line_ending;
  Glib::ustring decoded = decode_text(encoded, encoding);
  assert(decoded == text);
  std::puts("test_utf8bom_round_trip: OK");
}

void test_utf16_round_trip() {
  Glib::ustring text = "utf16 round trip \xC3\xA9\xC3\xA8"; // includes accented chars
  for (Encoding encoding : {Encoding::UTF16LE, Encoding::UTF16BE}) {
    std::string encoded = encode_text(text, encoding, LineEnding::LF);
    auto [detected, line_ending] = detect_encoding(encoded);
    assert(detected == encoding);
    (void)line_ending;
    Glib::ustring decoded = decode_text(encoded, detected);
    assert(decoded == text);
  }
  std::puts("test_utf16_round_trip: OK");
}

void test_line_ending_normalization() {
  Glib::ustring mixed = "a\r\nb\nc\rd";
  assert(encode_text(mixed, Encoding::UTF8, LineEnding::LF) == "a\nb\nc\nd");
  assert(encode_text(mixed, Encoding::UTF8, LineEnding::CRLF) ==
         "a\r\nb\r\nc\r\nd");
  assert(encode_text(mixed, Encoding::UTF8, LineEnding::CR) == "a\rb\rc\rd");
  std::puts("test_line_ending_normalization: OK");
}

} // namespace

int main() {
  test_bom_detection();
  test_line_ending_detection();
  test_utf8_round_trip();
  test_utf8bom_round_trip();
  test_utf16_round_trip();
  test_line_ending_normalization();
  return 0;
}
