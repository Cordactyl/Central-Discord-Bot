#pragma once
#include <dpp/dpp.h>
#include "class/acknowledge_payload.h"

extern dpp::cluster bot;
extern dpp::timer acknowledge_timer;
extern cordactyl::acknowledge_payload acknowledge;

namespace cordactyl
{
    void signal_handler(int signal);
}