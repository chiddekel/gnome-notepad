#pragma once

#include <deque>
#include <string>
#include <utility>

#include "core/app-state.h" // kMaxRecentFiles
#include "core/enums.h"

namespace Notepad {

// Pure encoding/line-ending logic, ported verbatim from legacy-notepad's
// file.cpp (DetectEncoding/DecodeText/EncodeText) with GLib/UTF-8 swapped in
// for the Win32 MultiByteToWideChar/WideCharToMultiByte codec calls. `data`
// is a raw byte buffer (not necessarily valid UTF-8), hence std::string
// rather than Glib::ustring.
std::pair<Encoding, LineEnding> detect_encoding(const std::string &data);
Glib::ustring decode_text(const std::string &data, Encoding encoding);
std::string encode_text(const Glib::ustring &text, Encoding encoding,
                         LineEnding line_ending);

Glib::ustring encoding_display_name(Encoding encoding);
Glib::ustring line_ending_display_name(LineEnding line_ending);

struct LoadResult {
  bool success = false;
  Glib::ustring text;
  Encoding encoding = Encoding::UTF8;
  LineEnding line_ending = LineEnding::LF;
  Glib::ustring error_message;
};

// Loads and decodes a file via GFile. Does not touch AppState/GtkTextBuffer
// itself -- the caller (Phase 6 commands.cpp) applies the result and then
// calls add_recent_file().
LoadResult load_file(const Glib::ustring &path);

// Encodes and writes text via GFile. Returns true on success; on failure
// error_message is set and the file is left untouched.
bool save_file(const Glib::ustring &path, const Glib::ustring &text,
                Encoding encoding, LineEnding line_ending,
                Glib::ustring &error_message);

// Dedupe + push-front + cap-at-kMaxRecentFiles, ported verbatim from
// legacy-notepad's AddRecentFile(). Pure list logic only -- the caller
// (Phase 6 commands.cpp) is responsible for persisting the result via
// Settings::save_recent_files() and rebuilding the recent-files GMenu.
void update_recent_files(std::deque<Glib::ustring> &recent_files,
                          const Glib::ustring &path);

} // namespace Notepad
