#include "queued_messages.h"

namespace cordactyl
{
    queued_message_error::queued_message_error(const dpp::confirmation_callback_t& callback) {
        this->code = callback.get_error().code;
        this->status = callback.http_info.status;
    }

    queued_message::queued_message(nlohmann::json &message) {
        this->tries = 0;
        this->processed_at = std::time(nullptr);
        dpp::set_int64_not_null(&message, "id", this->id);
        dpp::set_int64_not_null(&message, "guild_id", this->guild_id);
        dpp::set_int64_not_null(&message, "recipient", this->recipient);
        if (message.contains("embed") && message["embed"].is_object()) {
            this->embed = dpp::embed(&message["embed"]);
        } else {
            throw std::invalid_argument("message does not contain valid embed object");
        }
        if (! this->id) {
            throw std::invalid_argument("message has no id");
        }
        if (! this->guild_id) {
            throw std::invalid_argument("message has no guild_id");
        }
        if (! this->recipient) {
            throw std::invalid_argument("message has no recipient");
        }
    }

    dpp::task<void> queued_message::send_to_discord(dpp::cluster &bot) {
        // TODO: check sub guild's members
        const auto m_result = co_await bot.co_guild_get_member(this->guild_id, this->recipient);
        this->processed_at = std::time(nullptr);

        if (m_result.is_error()) {
            this->error = queued_message_error(m_result);
            bot.log(dpp::ll_warning, "Recipient " + std::to_string(this->recipient) + " was not found in guild " + std::to_string(this->guild_id) + " to send dm message");
            co_return;
        }

        dpp::message dm_message;
        dm_message.add_embed(this->embed);
        const auto result = co_await bot.co_direct_message_create(this->recipient, dm_message);
        if (result.is_error()) {
            this->error = queued_message_error(result);
            bot.log(dpp::ll_warning, "Failed to send dm message to " + std::to_string(this->recipient) + ": " + result.get_error().human_readable);
        }

        co_return;
    }

    dpp::task<void> processed_message_cache::process_new_messages(dpp::cluster &bot, nlohmann::json &messages) {
        if (! messages.is_array()) {
            bot.log(dpp::ll_error, "Passed queued messages must be an array");
            co_return;
        }

        if (messages.empty()) {
            co_return;
        }

        // store messages to cache
        for (auto message : messages) {
            try {
                auto qm = queued_message(message);
                co_await qm.send_to_discord(bot);
                this->store(qm);
            } catch (const std::exception &e) {
                bot.log(dpp::ll_error, e.what());
            }
        }

        this->clear_garbage_and_increase_tries();

        // build json
        nlohmann::json j;
        j["messages"] = nlohmann::json::array();
        std::vector<uint64_t> used_queued_messages;
        std::shared_lock l(this->cache_mutex);
        for (auto &[id, queued_message]: this->cache_map) {
            used_queued_messages.push_back(id);
            nlohmann::json m;
            m["id"]= queued_message.id;
            m["processed_at"] = utilities::get_current_timestamp(queued_message.processed_at);
            m["was_successfully_sent"] = !queued_message.error.has_value();
            if (queued_message.error.has_value()) {
                nlohmann::json err;
                err["code"] = queued_message.error->code;
                err["http_status"] = queued_message.error->status;
                m["error"] = err;
            }
            j["messages"].push_back(m);
        }
        l.unlock();

        bot.request(
            std::string(getenv("API_URL")) + "/processed-queued-messages",
            dpp::m_post,
            [this, used_queued_messages, &bot](const dpp::http_request_completion_t &response) {
                if (response.status == 401) {
                    bot.log(dpp::ll_error, "Failed to report back queued messages. Invalid API_TOKEN");
                } else if (response.status >= 300 || response.error != dpp::h_success) {
                    bot.log(dpp::ll_error, "Failed to report back queued messages. Error: " + std::to_string(response.error) + " Status: " + std::to_string(response.status) + "\nResponse:\n" + response.body);
                } else {
                    // remove the used messages from cache
                    std::unique_lock l(this->cache_mutex);
                    for (uint64_t id : used_queued_messages) {
                        auto existing = this->cache_map.find(id);
                        if (existing != this->cache_map.end()) {
                            this->cache_map.erase(existing);
                        }
                    }
                }
            },
            j.dump(),
            "application/json",
            API_HEADERS
        );

        co_return;
    }

    void processed_message_cache::store(queued_message &message) {
        std::unique_lock l(this->cache_mutex);
        this->cache_map.emplace(message.id, message);
    }

    void processed_message_cache::clear_garbage_and_increase_tries() {
        std::vector<uint64_t> to_remove;
        std::unique_lock l(this->cache_mutex);

        for (auto it = cache_map.begin(); it != cache_map.end(); ) {
            it->second.tries += 1;
            if (it->second.tries >= 4) {
                bot.log(dpp::ll_error, "Could not report back queued message ID " + std::to_string(it->first) + " to API");
                it = cache_map.erase(it);
            } else {
                ++it;
            }
        }
    }
}