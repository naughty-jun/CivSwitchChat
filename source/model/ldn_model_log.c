#include "ldn_model.h"

#include <stdarg.h>
#include <stdio.h>

#include "../app_logger.h"

/* Returns the slot to write next, dropping the oldest entry when full. */
static u32 acquireLogSlot(LdnModel* model)
{
    if (model->log_count < LDN_LOG_CAPACITY) {
        u32 index = (model->log_start + model->log_count) % LDN_LOG_CAPACITY;
        model->log_count++;
        return index;
    }

    u32 index = model->log_start;
    model->log_start = (model->log_start + 1) % LDN_LOG_CAPACITY;
    return index;
}

void ldnModelLog(LdnModel* model, const char* format, ...)
{
    u32 index = acquireLogSlot(model);

    va_list args;
    va_start(args, format);
    vsnprintf(model->logs[index], LDN_LOG_LENGTH, format, args);
    va_end(args);

    appLoggerWrite(model->logs[index]);
}

u32 ldnModelGetLogCount(const LdnModel* model)
{
    return model->log_count;
}

const char* ldnModelGetLogLine(const LdnModel* model, u32 index)
{
    if (index >= model->log_count)
        return "";

    u32 actual = (model->log_start + index) % LDN_LOG_CAPACITY;
    return model->logs[actual];
}
