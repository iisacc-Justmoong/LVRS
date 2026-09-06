#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QDebug>
#include <memory>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
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
}
)", QUrl());
    std::unique_ptr<QObject> object(component.create());
    if (!object) {
        qCritical() << component.errors();
        return 1;
    }
    for (const QString &target : {QStringLiteral("macos"), QStringLiteral("ios"),
                                 QStringLiteral("android"), QStringLiteral("macos")}) {
        if (!object->setProperty("target", target) || !object->property("unscaled").toBool()) {
            qCritical() << "Installed LVRS must keep authored sizes on" << target;
            return 2;
        }
        qInfo() << "Installed LVRS uses 1x metrics and typography on" << target;
    }
    return 0;
}
