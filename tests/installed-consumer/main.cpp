#include <QGuiApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QDebug>
#include <memory>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
#ifdef LVRS_CONSUMER_SOURCE_DIR
    const QDir sourceRoot(QStringLiteral(LVRS_CONSUMER_SOURCE_DIR));
    QDirIterator files(sourceRoot.filePath(QStringLiteral("qml")),
                       {QStringLiteral("*.qml")}, QDir::Files, QDirIterator::Subdirectories);
    int checked = 0;
    while (files.hasNext()) {
        QFile source(files.next());
        const QString relativePath = sourceRoot.relativeFilePath(source.fileName());
        QFile embedded(QStringLiteral(":/qt/qml/LVRS/") + relativePath);
        if (!embedded.exists())
            embedded.setFileName(QStringLiteral(":/qt/qml/LVRS/") + QFileInfo(source.fileName()).fileName());
        if (!source.open(QIODevice::ReadOnly) || !embedded.open(QIODevice::ReadOnly)
            || QCryptographicHash::hash(source.readAll(), QCryptographicHash::Sha256)
                != QCryptographicHash::hash(embedded.readAll(), QCryptographicHash::Sha256)) {
            qCritical() << "Installed LVRS resource differs from source:" << relativePath;
            return 3;
        }
        ++checked;
    }
    if (checked == 0) {
        qCritical() << "No source QML found under" << sourceRoot.path();
        return 3;
    }
    qInfo() << "Installed LVRS matches" << checked << "source QML resources";
#endif
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import LVRS 1.0 as LV

LV.VStack {
    width: 64
    height: 64
    property string target: "macos"
    onTargetChanged: LV.Theme.targetOverride = target
    Component.onCompleted: LV.Theme.targetOverride = target

    LV.Label { id: titleLabel; style: title; text: "Title" }
    LV.IconButton { id: button }
    LV.InputField { id: field }
    LV.LabelSegmentedControl {
        id: labels
        LV.LabelButton {}
        LV.LabelButton {}
    }
    LV.IconSegmentedControl {
        id: icons
        LV.IconButton {}
        LV.IconButton {}
    }
    LV.ListItem { id: form; type: LV.ListItem.Form }
    LV.HierarchyToolbar {
        id: toolbar
        property int idChanges: 0
        onActiveButtonIdChanged: idChanges += 1
    }

    function checkStableHierarchy() {
        const before = toolbar.idChanges
        for (let i = 0; i < 8; ++i)
            toolbar.normalizeActiveButton()
        return toolbar.idChanges === before && toolbar.activeButton === null
            && toolbar.activeButtonId === -1 && toolbar.activeIndex === -1
    }
    property bool hierarchySettled: !toolbar._normalizeScheduled
    property bool componentContract:
        field.implicitHeight === 22 && field.height === 22
        && field.insetVertical === 4.5 && field.inputItem.y === 4.5
        && field.inputItem.height === 13
        && field.placeholderColor === LV.Theme.disabledColor
        && labels.resolvedCornerRadius === 12 && icons.resolvedCornerRadius === 12
        && form.implicitHeight === 154
    property string diagnostics: JSON.stringify({
        inputHeight: field.height, inputY: field.inputItem.y,
        labelRadius: labels.resolvedCornerRadius, iconRadius: icons.resolvedCornerRadius,
        formHeight: form.implicitHeight
    })

    property bool unscaled:
        LV.Theme.effectiveTarget === target
        && LV.Theme.mobileTarget === (target === "ios" || target === "android")
        && LV.Theme.metricScaleFactor === 1.0
        && LV.Theme.typographyScaleFactor === 1.0
        && LV.Theme.gap8 === 8
        && LV.Theme.iconSm === 18
        && LV.Theme.textTitle === 26
        && LV.Theme.textTitle2 === 22
        && LV.Theme.textHeader === 17
        && LV.Theme.textHeader2 === 15
        && LV.Theme.textBody === 13
        && LV.Theme.textDescription === 12
        && LV.Theme.textCaption === 11
        && titleLabel.font.pixelSize === 26
        && button.implicitHeight === 22
        && !LV.FontPolicy.isThemeTextStyleCompliant(52, Font.Bold, "Bold")

    Item {
        Repeater {
            id: cards
            model: 8
            delegate: LV.Card { required property int index; type: index }
        }
    }
    function checkCards() {
        for (let i = 0; i < 8; ++i) {
            const card = cards.itemAt(i)
            if (!card || card.type !== i || card.implicitWidth !== (i === 1 ? 480 : 256)
                || card.implicitHeight !== (i < 2 ? 320 : 280))
                return false
        }
        return true
    }
    Item {
        Repeater {
            id: sliders
            model: 28
            delegate: LV.Slider {
                required property int index
                type: Math.floor(index / 4)
                size: index % 4
            }
        }
    }
    function checkSliders() {
        const heights = [22, 24, 32, 44]
        for (let i = 0; i < 28; ++i) {
            const slider = sliders.itemAt(i)
            if (!slider || slider.type !== Math.floor(i / 4) || slider.size !== i % 4
                || slider.implicitWidth !== 320 || slider.implicitHeight !== heights[i % 4]
                || slider.value !== 0.5 || slider.stepSize !== (i >= 24 ? 0.25 : 0))
                return false
        }
        return true
    }
    Item {
        Repeater {
            id: colorPickers
            model: 6
            delegate: LV.ColorPicker { required property int index; type: index }
        }
    }
    function checkColorPickers() {
        const heights = [461, 437, 437, 361, 413, 439]
        for (let i = 0; i < 6; ++i) {
            const picker = colorPickers.itemAt(i)
            if (!picker || picker.type !== i || picker.implicitWidth !== 320
                || picker.implicitHeight !== heights[i] || !picker.setHex("7A5AF8"))
                return false
        }
        return true
    }
}
)", QUrl());
    std::unique_ptr<QObject> object(component.create());
    if (!object) {
        qCritical() << component.errors();
        return 1;
    }
    for (const QString &target : {QStringLiteral("macos"), QStringLiteral("ios"),
                                 QStringLiteral("android"), QStringLiteral("macos")}) {
        object->setProperty("target", target);
        QCoreApplication::processEvents();
        if (!object->property("unscaled").toBool() || !object->property("componentContract").toBool()) {
            qCritical() << "Installed LVRS contract failed on" << target
                        << object->property("diagnostics").toString();
            return 2;
        }
        QVariant stable;
        QVariant cardsReady;
        QVariant slidersReady;
        QVariant colorPickersReady;
        if (!QMetaObject::invokeMethod(object.get(), "checkColorPickers", Q_RETURN_ARG(QVariant, colorPickersReady))
            || !colorPickersReady.toBool()) {
            qCritical() << "Installed LV.ColorPicker variants or color editing failed";
            return 2;
        }
        if (!QMetaObject::invokeMethod(object.get(), "checkSliders", Q_RETURN_ARG(QVariant, slidersReady))
            || !slidersReady.toBool()) {
            qCritical() << "Installed LV.Slider types, sizes or native range properties failed";
            return 2;
        }
        if (!QMetaObject::invokeMethod(object.get(), "checkCards", Q_RETURN_ARG(QVariant, cardsReady))
            || !cardsReady.toBool()) {
            qCritical() << "Installed LV.Card types or internal resources failed";
            return 2;
        }
        if (!QMetaObject::invokeMethod(object.get(), "checkStableHierarchy", Q_RETURN_ARG(QVariant, stable))
            || !stable.toBool()) {
            qCritical() << "Installed HierarchyToolbar repeats unchanged ID notifications";
            return 2;
        }
        QCoreApplication::processEvents();
        if (!object->property("hierarchySettled").toBool()) {
            qCritical() << "Installed HierarchyToolbar keeps scheduling normalization";
            return 2;
        }
        qInfo() << "Installed LVRS component contracts passed on" << target
                << object->property("diagnostics").toString();
    }
    return 0;
}
