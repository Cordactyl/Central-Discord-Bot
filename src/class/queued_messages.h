#pragma once
#include <dpp/dpp.h>
#include "../globals.hpp"
#include "../utilities.hpp"

extern dpp::cluster bot;

#ifndef QUEUED_MESSAGES_H
#define QUEUED_MESSAGES_H

namespace cordactyl
{
    struct queued_message_error {
        /**
         * Error code
         */
        uint32_t code = 0;
        /**
         * HTTP status code
         */
        uint16_t status = 0;

        queued_message_error(const dpp::confirmation_callback_t& callback);
    };

    struct queued_message {
        /**
         * Message identifier from the panel
         */
        uint64_t id;
        dpp::snowflake guild_id;
        dpp::snowflake recipient;
        dpp::embed embed;
        /**
         * The error of the message sending if there is one
         */
        std::optional<queued_message_error> error;
        uint32_t tries;
        /**
         * The timestamp when the message was processed (sent) by the bot
         */
        time_t processed_at;

        /**
         * @param message The message payload
         * @throws std::invalid_argument If the json message does not contain the expected fields
         */
        queued_message(nlohmann::json &message);

        dpp::task<void> send_to_discord(dpp::cluster &bot);
    };

    class processed_message_cache {
        /**
         * @brief Mutex to protect the cache
         *
         * This is a shared mutex so reading is cheap.
         */
        std::shared_mutex cache_mutex;

        /**
         * @brief Container of pointers to cached items
         */
        std::unordered_map<uint64_t, queued_message> cache_map;
    public:
        processed_message_cache() = default;

        dpp::task<void> process_new_messages(dpp::cluster &bot, nlohmann::json &messages);

        void store(queued_message &message);

        void clear_garbage_and_increase_tries();
    };
}

#endif
