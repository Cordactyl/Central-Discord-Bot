#pragma once
#include <dpp/dpp.h>

#define DEFAULT_API_URL getenv("API_URL") ? getenv("API_URL") : "https://cordactyl.com/api/bot"
#define CUSTOM_INSTALL_URL "https://discord.com/oauth2/authorize?client_id=1332837776319713300&permissions=19584&response_type=code&redirect_uri=https%3A%2F%2Fcordactyl.com%2Fsetup&integration_type=0&scope=identify+applications.commands+bot"
#define ACKNOWLEDGE_FREQUENCY 30
#define API_HEADERS { \
    {"Authorization", "Bearer " + std::string(getenv("API_TOKEN") ? getenv("API_TOKEN") : "")}, \
    {"Accept", "application/json"}, \
    {"User-Agent", "cordactyl-discord-bot/2.1"} \
}
#define EXIT_INVALID_API_TOKEN 24
