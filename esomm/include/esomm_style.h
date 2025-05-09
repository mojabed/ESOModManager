#pragma once

#include <QApplication>
#include <QFile>
#include <QStyle>
#include <QProxyStyle>
#include <QPainter>
#include <QPalette>
#include <QStyleOption>
#include <QMainWindow>

class EsommStyle : public QProxyStyle {
    Q_OBJECT

public:
    EsommStyle();

    void polish(QPalette& palette) override;
    void polish(QWidget* widget) override;
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;
};

// Helper class to apply the theme
class StyleLoader {
public:
    static void apply();
};
