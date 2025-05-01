#pragma once
#include <dpp/dpp.h>
#include "json_wrapper.h"
#include "queued_messages.h"

#ifndef ACKNOWLEDGE_PAYLOAD_H
#define ACKNOWLEDGE_PAYLOAD_H

extern dpp::cluster bot;

namespace cordactyl
{
    class acknowledge_payload: protected json_wrapper {
    public:
        dpp::application current_application;

        acknowledge_payload() = default;

        nlohmann::json pull_payload();

        dpp::task<void> send_acknowledgement(processed_message_cache &processed_messages, bool is_startup = false);

        dpp::task<void> fetch_current_application();

        void edit_current_application();

        /**
         * @brief Send the last acknowledgement before exiting.
         * Raises the signal when the acknowledgement is sent.
         */
        void exit(int signal);
    };
}
#endif
