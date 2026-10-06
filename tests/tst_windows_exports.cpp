#include "backend/runtime/bootstrapparallel.h"
#include "backend/runtime/foregroundservices.h"
#include <QCoreApplication>
#include "backend/graphics/colorpickermodel.h"
#include <QSignalSpy>

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    ColorPickerModel model;
    QSignalSpy changes(&model, &ColorPickerModel::colorEdited);
    if (!changes.isValid() || !model.editHex("123456") || changes.size() != 1) return 1;
    lvrs::ForegroundServiceGate gate;
    const auto result = lvrs::runBootstrapParallelTasks({});
    return gate.started() || gate.startAttemptCount() != 0 || result.fatalFailure();
}
