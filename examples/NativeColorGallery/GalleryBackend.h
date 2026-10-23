#pragma once
#include <LVRS/ColorPicker.h>
#include <LVRS/Theme.h>
#include <functional>
#include <stdexcept>
class NativeColorBackend final {
    static void good(ii::core::Result<void> result) {if(!result)throw std::runtime_error(result.error().message);}
public:
    inline static int created=0,destroyed=0;
    std::shared_ptr<lvrs::ColorPickerModel> model=std::make_shared<lvrs::ColorPickerModel>();
    std::function<void()> on_commit,on_close,on_accept,on_cancel;
    NativeColorBackend(){++created;good(model->set_color(lvrs::Theme::accent_color(lvrs::Accent::purple)));}
    ~NativeColorBackend(){++destroyed;}
    void hue(double v){good(model->edit_channel(lvrs::ColorChannel::hue,v));}
    void saturation(double v){good(model->edit_channel(lvrs::ColorChannel::saturation,v));}
    void brightness(double v){good(model->edit_channel(lvrs::ColorChannel::brightness,v));}
    void alpha(double v){good(model->edit_channel(lvrs::ColorChannel::alpha,v));}
    void red(){auto s=model->state();if(!s)throw std::runtime_error(s.error().message);good(model->edit_color({1,0,0,s->color.alpha}));}
    void ink(){auto v=model->channel_value(lvrs::ColorChannel::black);if(!v)throw std::runtime_error(v.error().message);good(model->edit_channel(lvrs::ColorChannel::black,*v+10));}
    void gray(){good(model->edit_channel(lvrs::ColorChannel::gray,50));}
    void commit(){if(on_commit)on_commit();}
    void accept(){good(model->accept());if(on_accept)on_accept();}
    void cancel(){good(model->cancel());if(on_cancel)on_cancel();}
    void close(){if(on_close)on_close();}
};
