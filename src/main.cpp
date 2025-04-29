#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>

#include "globals.hpp"
#include "utilities.hpp"
#include "class/translations.h"
#include "commands/appeals.h"
#include "class/queued_messages.h"
#include "signal_handler.h"

cordactyl::processed_message_cache processed_messages;

dpp::cluster bot(getenv("BOT_TOKEN") ? getenv("BOT_TOKEN") : "", dpp::i_guilds | dpp::i_guild_members | dpp::i_direct_messages); //!< Must be globally because of the signal handler

dpp::timer acknowledge_timer; //!< Must be globally because of the signal handler

#include "class/acknowledge_payload.h"

cordactyl::acknowledge_payload acknowledge; //!< Must be globally because of the signal handler

int main() {
    if (getenv("BOT_TOKEN") == nullptr) {
        std::cerr << "Missing environment variable BOT_TOKEN. You can get one from the discord developer portal." << std::endl;
        return EXIT_FAILURE;
    }
    if (getenv("API_TOKEN") == nullptr) {
        std::cerr << "Missing environment variable API_TOKEN. You can get one from the Cordactyl-Panel: https://cordactyl.com." << std::endl;
        return EXIT_FAILURE;
    }
    if (std::string(getenv("API_TOKEN")).find('|') == std::string::npos) {
        setenv("IS_CENTRAL", "1", 0);
    }
    setenv("API_URL", DEFAULT_API_URL, 0);

    // parse localisation files
    {
        const std::vector<std::filesystem::path> possible_paths = {
            std::filesystem::current_path() / "lang",
            std::filesystem::current_path().parent_path() / "lang"
        };

        for (const auto& path : possible_paths) {
            if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                std::cout << "Parsing localisations from " << path.string() << " directory" << std::endl;
                cordactyl::parse_localisations(path);
                break;
            }
        }
    }

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([](const dpp::slashcommand_t& event) -> dpp::task<void> {
        if (event.command.get_command_name() == "appeal") {
            co_await cordactyl::handle_appeal(event);
        }
    });

    bot.on_ready([](const dpp::ready_t& event) -> dpp::task<void> {
        bot.log(dpp::ll_info, "Logged in as " + bot.me.username + "! Using " + dpp::utility::version());

        if (dpp::run_once<struct startup>()) {
            co_await acknowledge.fetch_current_application();
            if (!getenv("IS_CENTRAL")) {
                acknowledge.edit_current_application();
            }
            bot.global_command_create(dpp::slashcommand("appeal", "Create an appeal", bot.me.id));

            acknowledge_timer = bot.start_timer([](const dpp::timer on_tick) -> dpp::task<void> {
                co_await acknowledge.send_acknowledgement(processed_messages);
            }, ACKNOWLEDGE_FREQUENCY, [](const dpp::timer on_stop) {
                bot.log(dpp::ll_info, "Stopped acknowledge timer");
            });

            bot.start_timer([](const dpp::timer on_tick) -> dpp::task<void> {
                co_await acknowledge.fetch_current_application();
            }, 60 * 12);
        }

        co_return;
    });

    std::signal(SIGINT, cordactyl::signal_handler);
    std::signal(SIGTERM, cordactyl::signal_handler);

    bot.start(dpp::st_wait);

    return EXIT_SUCCESS;
}
