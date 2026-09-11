#pragma once

#include <QColor>
#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <array>

// State and sRGB channel conversion for ColorPicker. Presentation stays in QML.
class ColorPickerModel : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ColorPickerModel)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor previousColor READ previousColor WRITE setPreviousColor NOTIFY previousColorChanged)
    Q_PROPERTY(QString hex READ hex NOTIFY colorChanged)
    Q_PROPERTY(double hue READ hue NOTIFY channelsChanged)
    Q_PROPERTY(double saturation READ saturation NOTIFY channelsChanged)
    Q_PROPERTY(double brightness READ brightness NOTIFY channelsChanged)
    Q_PROPERTY(int revision READ revision NOTIFY channelsChanged)

public:
    enum Channel { Hue, Saturation, Brightness, Red, Green, Blue, Cyan, Magenta, Yellow, Black, Alpha, Gray };
    Q_ENUM(Channel)
    explicit ColorPickerModel(QObject *parent = nullptr);

    QColor color() const { return m_color; }
    QColor previousColor() const { return m_previousColor; }
    QString hex() const { return m_color.name(QColor::HexRgb).mid(1).toUpper(); }
    double hue() const { return m_hue; }
    double saturation() const { return m_saturation; }
    double brightness() const { return m_brightness; }
    int revision() const { return m_revision; }
    void setColor(const QColor &color);
    void setPreviousColor(const QColor &color);

    Q_INVOKABLE double channelValue(int channel) const;
    Q_INVOKABLE QColor channelColor(int channel, double position) const;
    Q_INVOKABLE void editChannel(int channel, double value);
    Q_INVOKABLE void editHsv(double hue, double saturation, double brightness);
    Q_INVOKABLE void editColor(const QColor &color);
    Q_INVOKABLE bool editHex(const QString &text);
    Q_INVOKABLE void accept();
    Q_INVOKABLE void cancel();

signals:
    void colorChanged();
    void previousColorChanged();
    void channelsChanged();
    void colorEdited(const QColor &color);

private:
    void updateColor(const QColor &color, bool edited, bool keepHsv = false, bool keepCmyk = false);
    QColor m_color = QColor("#7a5af8");
    QColor m_previousColor = m_color;
    double m_hue = 0;
    double m_saturation = 0;
    double m_brightness = 1;
    std::array<double, 4> m_cmyk{};
    int m_revision = 0;
    bool m_explicitPrevious = false;
};
