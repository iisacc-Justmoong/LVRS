#pragma once
#include <functional>
#include <string>
struct GalleryBackend final {
    inline static int created=0,destroyed=0;
    std::function<void()> on_action,on_close;
    std::function<void(bool)> on_check;
    std::function<void(std::string)> on_edit;
    std::function<void(double)> on_move;
    std::function<void(int)> on_select;
    std::function<void(int)> on_step;
    std::function<void(char)> on_control_event;
    std::function<void()> on_open_menu,on_open_context,on_open_more,on_submenu_closed;
    std::function<void(int)> on_menu_select;
    GalleryBackend() { ++created; }
    ~GalleryBackend() { ++destroyed; }
    void action() { if(on_action) on_action(); }
    void close() { if(on_close) on_close(); }
    void checked(bool value) { if(on_check) on_check(value); }
    void edited(std::string value) { if(on_edit) on_edit(std::move(value)); }
    void moved(double value) { if(on_move) on_move(value); }
    void selected(int value) { if(on_select) on_select(value); }
    void stepped(int value) { if(on_step) on_step(value); }
    void pressed() { if(on_control_event) on_control_event('p'); }
    void released() { if(on_control_event) on_control_event('r'); }
    void canceled() { if(on_control_event) on_control_event('x'); }
    void open_menu() { if(on_open_menu) on_open_menu(); }
    void open_context() { if(on_open_context) on_open_context(); }
    void open_more() { if(on_open_more) on_open_more(); }
    void submenu_closed() { if(on_submenu_closed) on_submenu_closed(); }
    void menu_selected(int value) { if(on_menu_select) on_menu_select(value); }
};
