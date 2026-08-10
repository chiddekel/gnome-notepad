#include "application.h"

#include <adwaita.h>
#include <giomm/simpleaction.h>

#include "config.h"
#include "modules/theme.h"

namespace Notepad {

Application::Application()
    : Gtk::Application(APP_ID, Gio::Application::Flags::HANDLES_OPEN) {}

Glib::RefPtr<Application> Application::create() {
  return Glib::make_refptr_for_instance<Application>(new Application());
}

void Application::on_startup() {
  Gtk::Application::on_startup();

  adw_init();
  settings_.load_into(state_);
  Theme::apply_color_scheme(state_.color_scheme);

  add_action("quit", sigc::mem_fun(*this, &Application::on_action_quit));
  set_accel_for_action("app.quit", "<Control>q");

  // Matches legacy-notepad's notepad.rc ACCELERATORS block; entries for
  // actions not yet registered (Find/Replace/Goto/Font -- Phase 7) are
  // added as those actions are wired up in later phases.
  set_accels_for_action("win.new", {"<Control>n"});
  set_accels_for_action("win.open", {"<Control>o"});
  set_accels_for_action("win.save", {"<Control>s"});
  set_accels_for_action("win.save-as", {"<Control><Shift>s"});
  set_accels_for_action("win.print", {"<Control>p"});
  set_accels_for_action("win.undo", {"<Control>z"});
  set_accels_for_action("win.redo", {"<Control>y"});
  set_accels_for_action("win.cut", {"<Control>x"});
  set_accels_for_action("win.copy", {"<Control>c"});
  set_accels_for_action("win.paste", {"<Control>v"});
  set_accels_for_action("win.select-all", {"<Control>a"});
  set_accels_for_action("win.time-date", {"F5"});
  set_accels_for_action("win.find", {"<Control>f"});
  set_accels_for_action("win.find-next", {"F3"});
  set_accels_for_action("win.find-previous", {"<Shift>F3"});
  set_accels_for_action("win.replace", {"<Control>h"});
  set_accels_for_action("win.goto", {"<Control>g"});
  set_accels_for_action("win.zoom-in", {"<Control>plus"});
  set_accels_for_action("win.zoom-out", {"<Control>minus"});
  set_accels_for_action("win.zoom-default", {"<Control>0"});
}

void Application::on_activate() { ensure_window().present(); }

void Application::on_open(const Gio::Application::type_vec_files &files,
                           const Glib::ustring & /*hint*/) {
  auto &window = ensure_window();
  // Matches legacy-notepad's wWinMain command-line LoadFile() call; only
  // the first file is opened, same as the original (single-document app).
  if (!files.empty()) {
    window.open_external_path(files.front()->get_path());
  }
  window.present();
}

void Application::on_shutdown() {
  if (window_) {
    window_->save_state(state_);
    settings_.save_window(state_);
  }
  Gtk::Application::on_shutdown();
}

Window &Application::ensure_window() {
  if (!window_) {
    window_ = std::make_unique<Window>(gobj(), state_, settings_);
    add_window(*window_->gtk_window());
  }
  return *window_;
}

void Application::on_action_quit() { quit(); }

} // namespace Notepad
