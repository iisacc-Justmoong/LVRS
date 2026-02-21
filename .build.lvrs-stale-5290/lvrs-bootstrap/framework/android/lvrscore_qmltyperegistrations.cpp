/****************************************************************************
** Generated QML type registration code
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <QtQml/qqml.h>
#include <QtQml/qqmlmoduleregistration.h>

#if __has_include(<appstate.h>)
#  include <appstate.h>
#endif
#if __has_include(<backend.h>)
#  include <backend.h>
#endif
#if __has_include(<debuglogger.h>)
#  include <debuglogger.h>
#endif
#if __has_include(<fontpolicy.h>)
#  include <fontpolicy.h>
#endif
#if __has_include(<nativewindowstyle.h>)
#  include <nativewindowstyle.h>
#endif
#if __has_include(<pagemonitor.h>)
#  include <pagemonitor.h>
#endif
#if __has_include(<platforminfo.h>)
#  include <platforminfo.h>
#endif
#if __has_include(<renderingmonitor.h>)
#  include <renderingmonitor.h>
#endif
#if __has_include(<renderquality.h>)
#  include <renderquality.h>
#endif
#if __has_include(<routematcher.h>)
#  include <routematcher.h>
#endif
#if __has_include(<routeresolver.h>)
#  include <routeresolver.h>
#endif
#if __has_include(<runtimeevents.h>)
#  include <runtimeevents.h>
#endif
#if __has_include(<svgmanager.h>)
#  include <svgmanager.h>
#endif
#if __has_include(<textmarkup.h>)
#  include <textmarkup.h>
#endif
#if __has_include(<viewmodelregistry.h>)
#  include <viewmodelregistry.h>
#endif
#if __has_include(<viewstatetracker.h>)
#  include <viewstatetracker.h>
#endif


#if !defined(QT_STATIC)
#define Q_QMLTYPE_EXPORT Q_DECL_EXPORT
#else
#define Q_QMLTYPE_EXPORT
#endif
Q_QMLTYPE_EXPORT void qml_register_types_LVRS()
{
    QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    qmlRegisterTypesAndRevisions<AppState>("LVRS", 1);
    qmlRegisterTypesAndRevisions<Backend>("LVRS", 1);
    qmlRegisterTypesAndRevisions<DebugLogger>("LVRS", 1);
    qmlRegisterTypesAndRevisions<FontPolicy>("LVRS", 1);
    qmlRegisterTypesAndRevisions<NativeWindowStyle>("LVRS", 1);
    qmlRegisterTypesAndRevisions<PageMonitor>("LVRS", 1);
    qmlRegisterTypesAndRevisions<PlatformInfo>("LVRS", 1);
    qmlRegisterTypesAndRevisions<RenderQuality>("LVRS", 1);
    QMetaType::fromType<RenderQuality::DeviceTier>().id();
    qmlRegisterTypesAndRevisions<RenderingMonitor>("LVRS", 1);
    qmlRegisterTypesAndRevisions<RouteMatcher>("LVRS", 1);
    qmlRegisterTypesAndRevisions<RouteResolver>("LVRS", 1);
    qmlRegisterTypesAndRevisions<RuntimeEvents>("LVRS", 1);
    QMetaType::fromType<RuntimeEvents::CaptureProfile>().id();
    qmlRegisterTypesAndRevisions<SvgManager>("LVRS", 1);
    qmlRegisterTypesAndRevisions<TextMarkup>("LVRS", 1);
    qmlRegisterTypesAndRevisions<ViewModelRegistry>("LVRS", 1);
    qmlRegisterTypesAndRevisions<ViewStateTracker>("LVRS", 1);
    QMetaType::fromType<ViewStateTracker::ViewState>().id();
    QT_WARNING_POP
    qmlRegisterModule("LVRS", 1, 0);
}

static const QQmlModuleRegistration lVRSRegistration("LVRS", qml_register_types_LVRS);
