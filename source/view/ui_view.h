#pragma once

#include "ui_draw.h"
#include "../model/ldn_model.h"
#include "../controller/app_controller.h"

void uiViewDraw(
    UiDrawContext* draw,
    const AppController* controller,
    const LdnModel* model
);