#include "Backend.h"
#include <stdexcept>
using namespace ii;
void TooltipSession::start(runtime::Application& app){
    auto weak=weak_from_this();auto owner=app.event_loop().sender();auto scope=jobs.sender();
    auto submitted=jobs.post(app.workers(),[weak,owner,scope]()mutable{const auto value=6*7;(void)scope.post(owner,[weak,value]{if(auto p=weak.lock();p&&!p->retired){p->background_done=true;p->background.set("Background result: "+std::to_string(value)+" · iiSDKCore");}});});
    if(!submitted)throw std::runtime_error("Core background work admission failed");
}
void TooltipSession::retire(){if(retired)return;retired=true;jobs.request_close();(void)jobs.close();}
void TooltipBackend::opened(){++session->opens;status.set("Opened: "+std::to_string(session->opens));}
void TooltipBackend::closed(){++session->closes;status.set("Closed: "+std::to_string(session->closes));}
void TooltipBackend::origin(){++session->clicks;status.set("Origin action: "+std::to_string(session->clicks));}
void TooltipBackend::action(){detail.set("The same retained child handled this action.");}
void TooltipBackend::close(){if(on_close)on_close();}
