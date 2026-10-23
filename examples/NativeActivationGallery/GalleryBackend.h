#pragma once
#include <LVRS/ColorPicker.h>
#include <LVRS/Theme.h>
#include <functional>
#include <stdexcept>
class ActivationBackend final {
public:
    inline static int created=0,destroyed=0;
    std::shared_ptr<lvrs::ColorPickerModel> preview=std::make_shared<lvrs::ColorPickerModel>();
    std::function<void()> on_open,on_outside,on_opened,on_closed,on_close;
    ActivationBackend(){++created;if(auto r=preview->set_color(lvrs::Theme::accent_color(lvrs::Accent::purple));!r)throw std::runtime_error(r.error().message);}
    ~ActivationBackend(){++destroyed;}
    void open(){if(on_open)on_open();}void outside(){if(on_outside)on_outside();}
    void opened(){if(on_opened)on_opened();}void closed(){if(on_closed)on_closed();}void close(){if(on_close)on_close();}
};
