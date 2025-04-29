#pragma once
#include <dpp/dpp.h>
#include "../globals.hpp"
#include "../class/translations.h"

#ifndef APPEALS_H
#define APPEALS_H

namespace cordactyl
{
    dpp::task<void> handle_appeal(const dpp::slashcommand_t &event);
}

#endif
