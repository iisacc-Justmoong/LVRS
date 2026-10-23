#pragma once
#include <LVRS/ColorPicker.h>
#include <LVRS/Theme.h>
#include <functional>
#include <stdexcept>
class NativePickerBackend final {
public:
    inline static int created=0,destroyed=0;
    std::shared_ptr<lvrs::ColorPickerModel> model=std::make_shared<lvrs::ColorPickerModel>();
    std::function<void(std::string)> report;
    std::function<void()> on_close;
    NativePickerBackend(){++created;if(auto r=model->set_color(lvrs::Theme::accent_color(lvrs::Accent::purple));!r)throw std::runtime_error(r.error().message);}
    ~NativePickerBackend(){++destroyed;}
    void edited(ii::gui::WidgetColor){if(report)report("Shared color edited");}
    void accepted(ii::gui::WidgetColor){if(report)report("Applied · comparison baseline updated");}
    void canceled(){if(report)report("Canceled · comparison baseline restored");}
    void eyedropper(){if(report)report("Eyedropper requested · sampling belongs to the document host");}
    void close(){if(on_close)on_close();}
};
