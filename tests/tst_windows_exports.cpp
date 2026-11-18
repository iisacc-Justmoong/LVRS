#include "backend/runtime/bootstrapparallel.h"
#include "backend/runtime/foregroundservices.h"
#include <QCoreApplication>
#include "backend/graphics/colorpickermodel.h"
#include "CTestSupport.h"

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner;
    runner.run("exportedModelsAndBootstrapLink", [&] {
        ColorPickerModel model;
        int changes = 0;
        const auto connection = QObject::connect(&model, &ColorPickerModel::colorEdited,
                                                  &application, [&] { ++changes; });
        CTEST_CHECK(connection);
        CTEST_CHECK(model.editHex("123456"));
        CTEST_COMPARE(changes, 1);
        lvrs::ForegroundServiceGate gate;
        const auto result = lvrs::runBootstrapParallelTasks({});
        CTEST_CHECK(!gate.started());
        CTEST_COMPARE(gate.startAttemptCount(), 0);
        CTEST_CHECK(!result.fatalFailure());
    });
    return runner.result();
}
