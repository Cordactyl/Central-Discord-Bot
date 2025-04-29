#include "signal_handler.h"

namespace cordactyl {
    void signal_handler(int signal) {
        bot.log(dpp::ll_info, "Stopping Discord-Bot... Signal: " + std::to_string(signal));
        bot.stop_timer(acknowledge_timer);

        acknowledge.exit(signal);

        bot.start_timer([](const dpp::timer on_tick) {
            bot.stop_timer(on_tick);
            bot.log(dpp::ll_error, "Timeout for acknowledge reached");
            bot.shutdown();
            exit(EXIT_FAILURE);
        }, 5);
    }
}