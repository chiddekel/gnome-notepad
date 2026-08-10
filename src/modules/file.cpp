#include "file.h"

#include <algorithm>
#include <giomm/file.h>
#include <glib.h>
#include <glibmm/convert.h>
#include <glibmm/error.h>

namespace Notepad {

std::pair<Encoding, LineEnding> detect_encoding(const std::string &data) {
  Encoding encoding = Encoding::UTF8;
  const auto *bytes = reinterpret_cast<const unsigned char *>(data.data());
  if (data.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
    encoding = Encoding::UTF8BOM;
  } else if (data.size() >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
    encoding = Encoding::UTF16LE;
  } else if (data.size() >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF) {
    encoding = Encoding::UTF16BE;
  } else if (!g_utf8_validate(data.data(), static_cast<gssize>(data.size()), nullptr)) {
    encoding = Encoding::ANSI;
  }

  LineEnding line_ending = LineEnding::CRLF;
  for (std::size_t i = 0; i < data.size(); ++i) {
    if (data[i] == '\r') {
      line_ending = (i + 1 < data.size() && data[i + 1] == '\n')
                        ? LineEnding::CRLF
                        : LineEnding::CR;
      break;
    }
    if (data[i] == '\n') {
      line_ending = LineEnding::LF;
      break;
    }
  }
  return {encoding, line_ending};
}

Glib::ustring decode_text(const std::string &data, Encoding encoding) {
  std::size_t skip = 0;
  if (encoding == Encoding::UTF8BOM) {
    skip = 3;
  } else if (encoding == Encoding::UTF16LE || encoding == Encoding::UTF16BE) {
    skip = 2;
  }
  if (data.size() < skip) {
    return {};
  }
  std::string body = data.substr(skip);

  try {
    switch (encoding) {
    case Encoding::UTF16LE:
      return Glib::ustring(Glib::convert(body, "UTF-8", "UTF-16LE"));
    case Encoding::UTF16BE:
      return Glib::ustring(Glib::convert(body, "UTF-8", "UTF-16BE"));
    case Encoding::ANSI:
      return Glib::locale_to_utf8(body);
    case Encoding::UTF8:
    case Encoding::UTF8BOM:
      return Glib::ustring(body);
    }
  } catch (const Glib::ConvertError &) {
    // Best-effort fallback: treat as raw UTF-8 rather than losing the file.
  }
  return Glib::ustring(body);
}

std::string encode_text(const Glib::ustring &text, Encoding encoding,
                         LineEnding line_ending) {
  Glib::ustring converted;
  converted.reserve(text.size());
  for (auto it = text.begin(); it != text.end();) {
    gunichar c = *it;
    ++it;
    if (c == '\r' || c == '\n') {
      if (c == '\r' && it != text.end() && *it == '\n') {
        ++it;
      }
      switch (line_ending) {
      case LineEnding::CRLF:
        converted += "\r\n";
        break;
      case LineEnding::LF:
        converted += '\n';
        break;
      case LineEnding::CR:
        converted += '\r';
        break;
      }
    } else {
      converted += c;
    }
  }

  try {
    switch (encoding) {
    case Encoding::UTF8BOM:
      return "\xEF\xBB\xBF" + converted.raw();
    case Encoding::UTF16LE:
      return "\xFF\xFE" + Glib::convert(converted.raw(), "UTF-16LE", "UTF-8");
    case Encoding::UTF16BE:
      return "\xFE\xFF" + Glib::convert(converted.raw(), "UTF-16BE", "UTF-8");
    case Encoding::ANSI:
      return Glib::locale_from_utf8(converted);
    case Encoding::UTF8:
      return converted.raw();
    }
  } catch (const Glib::ConvertError &) {
    // Best-effort fallback: write as UTF-8 rather than losing the file.
  }
  return converted.raw();
}

Glib::ustring encoding_display_name(Encoding encoding) {
  switch (encoding) {
  case Encoding::UTF8:
    return "UTF-8";
  case Encoding::UTF8BOM:
    return "UTF-8 BOM";
  case Encoding::UTF16LE:
    return "UTF-16 LE";
  case Encoding::UTF16BE:
    return "UTF-16 BE";
  case Encoding::ANSI:
    return "ANSI";
  }
  return {};
}

Glib::ustring line_ending_display_name(LineEnding line_ending) {
  switch (line_ending) {
  case LineEnding::CRLF:
    return "CRLF";
  case LineEnding::LF:
    return "LF";
  case LineEnding::CR:
    return "CR";
  }
  return {};
}

LoadResult load_file(const Glib::ustring &path) {
  LoadResult result;
  auto file = Gio::File::create_for_path(path);

  char *raw_contents = nullptr;
  gsize length = 0;
  try {
    file->load_contents(raw_contents, length);
  } catch (const Glib::Error &error) {
    result.error_message = error.what();
    return result;
  }
  std::string data(raw_contents, length);
  g_free(raw_contents);

  auto [encoding, line_ending] = detect_encoding(data);
  result.success = true;
  result.text = decode_text(data, encoding);
  result.encoding = encoding;
  result.line_ending = line_ending;
  return result;
}

bool save_file(const Glib::ustring &path, const Glib::ustring &text,
                Encoding encoding, LineEnding line_ending,
                Glib::ustring &error_message) {
  auto file = Gio::File::create_for_path(path);
  std::string data = encode_text(text, encoding, line_ending);
  try {
    std::string new_etag;
    file->replace_contents(data, std::string(), new_etag);
  } catch (const Glib::Error &error) {
    error_message = error.what();
    return false;
  }
  return true;
}

void update_recent_files(std::deque<Glib::ustring> &recent_files,
                          const Glib::ustring &path) {
  auto it = std::find(recent_files.begin(), recent_files.end(), path);
  if (it != recent_files.end()) {
    recent_files.erase(it);
  }
  recent_files.push_front(path);
  while (recent_files.size() > kMaxRecentFiles) {
    recent_files.pop_back();
  }
}

} // namespace Notepad
