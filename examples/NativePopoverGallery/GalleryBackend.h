#pragma once
#include <LVRS/ColorPicker.h>
#include <LVRS/Theme.h>
#include <functional>
#include <stdexcept>
class NativePopoverBackend final {
public:
    inline static int created=0,destroyed=0;
    std::shared_ptr<lvrs::ColorPickerModel> model=std::make_shared<lvrs::ColorPickerModel>();
    std::function<void()> on_open,on_outside,on_opened,on_closed,on_apply,on_cancel,on_close;
    NativePopoverBackend(){++created;if(auto r=model->set_color(lvrs::Theme::accent_color(lvrs::Accent::purple));!r)throw std::runtime_error(r.error().message);}
    ~NativePopoverBackend(){++destroyed;}
    void open(){if(on_open)on_open();}
    void outside(){if(on_outside)on_outside();}
    void opened(){if(on_opened)on_opened();}
    void closed(){if(on_closed)on_closed();}
    void accepted(ii::gui::WidgetColor){if(on_apply)on_apply();}
    void canceled(){if(on_cancel)on_cancel();}
    void close(){if(on_close)on_close();}
};
