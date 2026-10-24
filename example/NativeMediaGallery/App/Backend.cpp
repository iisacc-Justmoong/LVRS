#include "Backend.h"
MediaBackend::MediaBackend(){++created;}
void MediaBackend::select(bool value){model->selected.set(value);}
void MediaBackend::update(){model->title.set("Updated native image");model->progress.set(.75);model->rows.set({{"Files","2,048"},{"Size","512 MB"}});}
void MediaBackend::menu(){++model->menu_events;model->status.set("Card menu requested");}
void MediaBackend::action(){++model->action_events;model->status.set("Card action triggered");}
void MediaBackend::close(){if(on_close)on_close();}
