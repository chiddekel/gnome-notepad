#pragma once

#include <gtkmm/application.h>
#include <memory>

#include "core/app-state.h"
#include "modules/settings.h"
#include "window.h"

namespace Notepad {

class Application : public Gtk::Application {
protected:
  Application();

public:
  static Glib::RefPtr<Application> create();

protected:
  void on_startup() override;
  void on_activate() override;
  void on_open(const Gio::Application::type_vec_files &files,
               const Glib::ustring &hint) override;
  void on_shutdown() override;

private:
  Window &ensure_window();
  void on_action_quit();

  Settings settings_;
  AppState state_;
  std::unique_ptr<Window> window_;
};

} // namespace Notepad
