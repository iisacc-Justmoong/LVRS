#include <LVRS/HierarchyModel.h>
#include <LVRS/ProgressModel.h>
#include <LVRS/Theme.h>
#include <iiGUI/Host.h>

int main()
{
    lvrs::ProgressModel model;
    const auto changed = model.set_current_value(42.0);
    return changed && model.current_value() == 42.0 ? 0 : 1;
}
