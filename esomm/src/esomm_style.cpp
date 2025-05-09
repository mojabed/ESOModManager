#include "esomm_style.h"

EsommStyle::EsommStyle() : QProxyStyle() {
    // Set up cross-platform styling
}

void EsommStyle::polish(QPalette& palette) {
    // Material design color palette
    palette.setColor(QPalette::Window, QColor(245, 245, 245));
    palette.setColor(QPalette::WindowText, QColor(33, 33, 33));
    palette.setColor(QPalette::Base, QColor(255, 255, 255));
    palette.setColor(QPalette::AlternateBase, QColor(240, 240, 240));
    palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    palette.setColor(QPalette::ToolTipText, QColor(33, 33, 33));
    palette.setColor(QPalette::Text, QColor(33, 33, 33));
    palette.setColor(QPalette::Button, QColor(66, 133, 244));
    palette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
    palette.setColor(QPalette::BrightText, QColor(255, 255, 255));
    palette.setColor(QPalette::Highlight, QColor(66, 133, 244));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    palette.setColor(QPalette::Link, QColor(66, 133, 244));
    palette.setColor(QPalette::LinkVisited, QColor(158, 158, 158));
}

void EsommStyle::polish(QWidget* widget) {
    QProxyStyle::polish(widget);
}

int EsommStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const {
    switch (metric) {
    case PM_ButtonMargin:
        return 8;
    case PM_ButtonDefaultIndicator:
        return 0;
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        return 0;
    case PM_LayoutLeftMargin:
    case PM_LayoutRightMargin:
    case PM_LayoutTopMargin:
    case PM_LayoutBottomMargin:
        return 10;
    default:
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

void StyleLoader::apply() {
    qApp->setStyle(new EsommStyle());

    // Load global stylesheet
    QFile styleFile(":/resources/esomm_style.qss");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qApp->setStyleSheet(styleFile.readAll());
        styleFile.close();
    }
}
