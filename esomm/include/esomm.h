#pragma once

#include "ui/ui_esomm.h"
#include <QtWidgets/QWidget>

class ESOMM : public QWidget {
    Q_OBJECT

public:
    ESOMM(QWidget *parent = nullptr);
    ~ESOMM();

private:
    Ui::ESOMMClass ui;
};
