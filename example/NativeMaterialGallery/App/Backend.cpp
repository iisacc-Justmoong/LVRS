#include "Backend.h"
void MaterialBackend::change_accent(){model->accent.set(model->accent.get().green>.5?ii::gui::WidgetColor{0,.4,1,1}:ii::gui::WidgetColor{0,1,.12,1});}
void MaterialBackend::change_density(){model->density.set(model->density.get()==lvrs::MaterialDensity::glass25?lvrs::MaterialDensity::dense75:lvrs::MaterialDensity::glass25);}
void MaterialBackend::change_blur(){model->blur.set(!model->blur.get());}
void MaterialBackend::close(){if(on_close)on_close();}
