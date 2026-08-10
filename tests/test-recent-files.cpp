#include <cassert>
#include <cstdio>
#include <deque>

#include "modules/file.h"

using namespace Notepad;

namespace {

void test_dedupe_on_reopen() {
  std::deque<Glib::ustring> recent;
  update_recent_files(recent, "a.txt");
  update_recent_files(recent, "b.txt");
  update_recent_files(recent, "c.txt");
  assert(recent.size() == 3);
  assert(recent[0] == "c.txt");

  // Reopening "a.txt" should move it to the front, not duplicate it.
  update_recent_files(recent, "a.txt");
  assert(recent.size() == 3);
  assert(recent[0] == "a.txt");
  assert(recent[1] == "c.txt");
  assert(recent[2] == "b.txt");
  std::puts("test_dedupe_on_reopen: OK");
}

void test_cap_at_max() {
  std::deque<Glib::ustring> recent;
  for (int i = 0; i < static_cast<int>(kMaxRecentFiles) + 5; ++i) {
    update_recent_files(recent, Glib::ustring::compose("file%1.txt", i));
  }
  assert(recent.size() == kMaxRecentFiles);
  // Most recently added is at the front; oldest entries were evicted.
  assert(recent.front() ==
         Glib::ustring::compose("file%1.txt",
                                 static_cast<int>(kMaxRecentFiles) + 4));
  assert(recent.back() ==
         Glib::ustring::compose("file%1.txt", 5)); // files 0-4 evicted
  std::puts("test_cap_at_max: OK");
}

} // namespace

int main() {
  test_dedupe_on_reopen();
  test_cap_at_max();
  return 0;
}
