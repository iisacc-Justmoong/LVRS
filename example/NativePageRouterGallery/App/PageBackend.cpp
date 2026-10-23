#include "PageBackend.h"
#include <stdexcept>
namespace {
void check(ii::core::Result<bool> result){if(!result)throw std::runtime_error(result.error().message);}
}
PageBackend::PageBackend(){++created;}
PageBackend::~PageBackend(){++retired;}
void PageBackend::report(){if(navigator)check(navigator->go("/reports/42"));}
void PageBackend::back(){if(navigator)check(navigator->back());}
