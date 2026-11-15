#pragma once

#include "civ7_session.h"

/* Reads at most one datagram and applies it to the session. */
void civ7ReceiveOne(Civ7Session *s);
