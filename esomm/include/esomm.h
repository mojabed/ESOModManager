#pragma once

#include <QtWidgets/QWidget>
#include "ui/ui_esomm.h"

class ESOMM : public QWidget {
    Q_OBJECT

public:
    ESOMM(QWidget *parent = nullptr);
    ~ESOMM();

private:
    Ui::ESOMMClass ui;
};
