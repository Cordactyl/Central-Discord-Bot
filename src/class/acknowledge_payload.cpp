#include "acknowledge_payload.h"

namespace cordactyl
{
    nlohmann::json acknowledge_payload::pull_payload() {
        std::unique_lock lock(mutex);
        nlohmann::json result = payload;
        payload = {};

        nlohmann::json application = {};
        nlohmann::json bot = {};
        bot["id"] = this->current_application.bot.id;
        bot["username"] = this->current_application.bot.username;
        application["bot"] = bot;
        application["id"] = this->current_application.id;
        if (! this->current_application.name.empty()) {
            application["name"] = this->current_application.name;
        }
        if (! this->current_application.icon.to_string().empty()) {
            application["icon"] = this->current_application.icon.to_string();
        }
        if (this->current_application.flags) {
            application["flags"] = this->current_application.flags;
        }
        if (! this->current_application.redirect_uris.empty()) {
            application["redirect_uris"] = this->current_application.redirect_uris;
        }
        if (! this->current_application.custom_install_url.empty()) {
            application["custom_install_url"] = this->current_application.custom_install_url;
        }
        application["bot_public"] = this->current_application.bot_public;
        result["application"] = application;

        return result;
    }

    dpp::task<void> acknowledge_payload::send_acknowledgement(processed_message_cache &processed_messages, bool is_startup) {
        dpp::http_request_completion_t response = co_await bot.co_request(
                std::string(getenv("API_URL")) + "/acknowledge" + (is_startup ? "?startup=true" : ""),
                dpp::m_post,
                this->pull_payload().dump(),
                "application/json",
                API_HEADERS
            );
        if (response.status == 401) {
            bot.log(dpp::ll_error, "Acknowledge failed. Invalid API_TOKEN");
            if (!getenv("IS_CENTRAL")) {
                exit(EXIT_INVALID_API_TOKEN);
            }
        } else if (response.status >= 300 || response.error != dpp::http_error::h_success) {
            bot.log(dpp::ll_error, "Acknowledge failed. Error: " + std::to_string(response.error) + " Status: " + std::to_string(response.status) + "\nResponse:\n" + response.body);
        }
        if (! response.body.empty()) {
            nlohmann::json json;
            try {
                json = nlohmann::json::parse(response.body);
            } catch (nlohmann::json::exception &e) {
                bot.log(dpp::ll_error, "JSON parse error on acknowledge. Status: " + std::to_string(response.status) + "\nResponse:\n" + response.body + "\nException:\n" + e.what());
                co_return;
            }
            if (!json.empty() && !json.is_object()) {
                bot.log(dpp::ll_error, "acknowledge response is not an object");
            }
            if (json.contains("tasks")) {
                if (!json.at("tasks").empty() && !json.at("tasks").is_object()) {
                    bot.log(dpp::ll_error, "Expected tasks in acknowledge response to be an object");
                }
                if (json["tasks"].contains("dm_messages")) {
                    if (json["tasks"]["dm_messages"].is_array()) {
                        co_await processed_messages.process_new_messages(bot, json["tasks"]["dm_messages"]);
                    } else {
                        bot.log(dpp::ll_error, "Expected dm_messages to be an array");
                    }
                }
            }
        }

        co_return;
    }

    dpp::task<void> acknowledge_payload::fetch_current_application() {
        dpp::confirmation_callback_t result = co_await bot.co_current_application_get();
        if (result.is_error()) {
            bot.log(dpp::ll_error, "Failed to get current application");
            co_return;
        }
        this->current_application = result.get<dpp::application>();
    }

    void acknowledge_payload::edit_current_application() {
        nlohmann::json body;
        if (this->current_application.event_webhooks_status == dpp::application_event_webhook_status::ews_enabled) {
            body["event_webhooks_status"] = dpp::application_event_webhook_status::ews_disabled;
        }
        if (this->current_application.custom_install_url != CUSTOM_INSTALL_URL && this->current_application.bot_public) {
            body["custom_install_url"] = CUSTOM_INSTALL_URL;
            body["install_params"] = nullptr;
        }
        if (!body.empty()) {
            bot.post_rest(API_PATH "/applications", "@me", "", dpp::m_patch, body.dump(), [this](nlohmann::json &j, const dpp::http_request_completion_t& http) {
                if (http.error != dpp::h_success || http.status < 200 || http.status >= 300) {
                    bot.log(dpp::ll_error, "Failed to edit application. HTTP " + std::to_string(http.status) + "\nResponse: " + http.body);
                } else {
                    this->current_application = dpp::application().fill_from_json(&j);
                }
            });
        }
    }

    void acknowledge_payload::exit(int signal) {
        bot.request(
            std::string(getenv("API_URL")) + "/acknowledge?shutdown=true",
            dpp::m_post,
            [signal](const dpp::http_request_completion_t &response) {
                if (response.status == 401) {
                    bot.log(dpp::ll_error, "Acknowledge failed. Invalid API_TOKEN");
                } else if (response.status != 200 || response.error != dpp::h_success) {
                    bot.log(dpp::ll_error, "Acknowledge failed. Error: " + std::to_string(response.error) + " Status: " + std::to_string(response.status) + "\nResponse:\n" + response.body);
                } else {
                    bot.log(dpp::ll_info, "Acknowledged successfully!");
                }

                bot.shutdown();

                // recover default behaviour and raise signal again
                std::signal(signal, SIG_DFL);
                std::raise(signal);
            },
            this->pull_payload().dump(),
            "application/json",
            API_HEADERS
        );
    }
}