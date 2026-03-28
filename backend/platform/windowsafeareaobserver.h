#pragma once

#include <QObject>
#include <QPointer>
#include <QtQml/qqml.h>

class QScreen;
class QEvent;
class QWindow;

class WindowSafeAreaObserver : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(WindowSafeAreaObserver)

    Q_PROPERTY(QObject *window READ window WRITE setWindow NOTIFY windowChanged)
    Q_PROPERTY(qreal leftInset READ leftInset NOTIFY marginsChanged)
    Q_PROPERTY(qreal topInset READ topInset NOTIFY marginsChanged)
    Q_PROPERTY(qreal rightInset READ rightInset NOTIFY marginsChanged)
    Q_PROPERTY(qreal bottomInset READ bottomInset NOTIFY marginsChanged)
    Q_PROPERTY(bool resolved READ resolved NOTIFY resolvedChanged)

public:
    explicit WindowSafeAreaObserver(QObject *parent = nullptr);

    QObject *window() const;
    void setWindow(QObject *windowObject);

    qreal leftInset() const;
    qreal topInset() const;
    qreal rightInset() const;
    qreal bottomInset() const;
    bool resolved() const;

    Q_INVOKABLE void refresh();

signals:
    void windowChanged();
    void marginsChanged();
    void resolvedChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void attachWindow(QWindow *window);
    void detachWindow();
    void attachScreen(QScreen *screen);
    void detachScreen();
    void updateMargins();

    QPointer<QWindow> m_window;
    QPointer<QScreen> m_screen;

    qreal m_leftInset = 0.0;
    qreal m_topInset = 0.0;
    qreal m_rightInset = 0.0;
    qreal m_bottomInset = 0.0;
    bool m_resolved = false;

    QMetaObject::Connection m_windowWidthChangedConnection;
    QMetaObject::Connection m_windowHeightChangedConnection;
    QMetaObject::Connection m_windowXChangedConnection;
    QMetaObject::Connection m_windowYChangedConnection;
    QMetaObject::Connection m_windowVisibleChangedConnection;
    QMetaObject::Connection m_windowScreenChangedConnection;
    QMetaObject::Connection m_windowActiveChangedConnection;
    QMetaObject::Connection m_windowDestroyedConnection;
    QMetaObject::Connection m_screenGeometryChangedConnection;
    QMetaObject::Connection m_screenAvailableGeometryChangedConnection;
    QMetaObject::Connection m_screenOrientationChangedConnection;
};
