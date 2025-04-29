#include "appeals.h"

extern dpp::cluster bot;

namespace cordactyl
{
    dpp::task<void> handle_appeal(const dpp::slashcommand_t &event) {
        dpp::snowflake guild_id;
        dpp::guild_member member;
        if ((event.command.context.has_value() && event.command.context.value() != dpp::itc_guild) || ! event.command.guild_id) {
            // TODO: check only sub guild's members
            auto cache = dpp::get_guild_cache();
            std::shared_lock l(cache->get_mutex());
            auto guilds = cache->get_container();
            for (auto &[id, g] : guilds) {
                for (auto gm = g->members.begin(); gm != g->members.end(); ++gm) {
                    if (gm->first == event.command.get_issuing_user().id) {
                        if (guild_id) {
                            // user has more than one mutual guilds
                            l.unlock();
                            event.reply(dpp::message(_("You must be in a guild to use this command.", event.command.locale)).set_flags(dpp::m_ephemeral));
                            co_return;
                        } else {
                            guild_id = id;
                            member = gm->second;
                        }
                    }
                }
            }
        }
        if (! guild_id) {
            guild_id = event.command.guild_id;
        }
        if (! member.user_id) {
            member = event.command.member;
        }

        if (event.command.context.has_value() && event.command.context.value() != dpp::itc_bot_dm) {
            // send an empty message to check if the bot can send the member a DM.
            // If it returns 400 instead of 403 because of the empty message, the bot will be able to DM the member.
            auto result = co_await bot.co_direct_message_create(event.command.get_issuing_user().id, dpp::message(""));
            if (result.http_info.status == 403) {
                event.reply(dpp::message(_("It seems like i can't send you a DM. Please send me a DM so that I can keep you informed about your appeal.", event.command.locale)).set_flags(dpp::m_ephemeral));
                co_return;
            }
            if (result.http_info.status != 400) {
                bot.log(dpp::ll_error, "Failed to check whether user " + std::to_string(event.command.get_issuing_user().id) + " in guild " + std::to_string(guild_id) + " can be sent a DM. Status: " + std::to_string(result.http_info.status) + " Response: " + result.http_info.body);
                event.reply(dpp::message(_("Error. Couldn't generate a response. Please try again later.", event.command.locale)).set_flags(dpp::m_ephemeral));
                co_return;
            }
        }

        nlohmann::json body;
        body["guild_id"] = guild_id;
        nlohmann::json user;
        user["id"] = event.command.get_issuing_user().id;
        std::string name;
        if (!member.get_nickname().empty()) {
            name = member.get_nickname();
        } else if (!event.command.get_issuing_user().global_name.empty()) {
            name = event.command.get_issuing_user().global_name;
        } else {
            name = event.command.get_issuing_user().username;
        }
        user["name"] = name;
        std::string avatar;
        if (!member.avatar.to_string().empty()) {
            avatar = member.avatar.to_string();
        } else {
            avatar = event.command.get_issuing_user().avatar.to_string();
        }
        if (!avatar.empty()) {
            user["avatar"] = avatar;
        }
        uint32_t default_avatar_index;
        if (event.command.get_issuing_user().discriminator) {
            default_avatar_index = event.command.get_issuing_user().discriminator % 5;
        } else {
            default_avatar_index = (event.command.get_issuing_user().id >> 22) % 6;
        }
        user["default_avatar_index"] = default_avatar_index;
        nlohmann::json::array_t roles;
        for (const auto role_id : member.get_roles()) {
            dpp::role* role = dpp::find_role(role_id);
            if (role) {
                nlohmann::json j;
                j["id"] = role_id;
                j["name"] = role->name;
                std::stringstream stream;
                stream << std::hex << role->colour;
                std::string hex_color_str( stream.str() );
                j["color"] = "#" + hex_color_str;
                roles.push_back(j);
            }
        }
        user["roles"] = roles;
        body["user"] = user;

        dpp::http_request_completion_t response = co_await bot.co_request(
            std::string(getenv("API_URL")) + "/appeal-invitations/create",
            dpp::m_post,
            body.dump(),
            "application/json",
            API_HEADERS
        );
        if (response.status == 503) {
            event.reply(dpp::message(_("The server is currently undergoing maintenance. Please try again later.", event.command.locale)).set_flags(dpp::m_ephemeral));
            co_return;
        }
        if (response.status >= 500) {
            bot.log(dpp::ll_error, std::to_string(response.status) + " API-Error while requesting /appeal-invitations/create: " + response.body + "\nBody: " + body.dump());
            event.reply(dpp::message(_("Cordactyl API error. Please try again later.", event.command.locale)).set_flags(dpp::m_ephemeral));
            co_return;
        }
        if (response.status >= 400) {
            bot.log(dpp::ll_error, std::to_string(response.status) + " API-Error while requesting /appeal-invitations/create: " + response.body + "\nBody: " + body.dump());
            event.reply(dpp::message(_("Error. Couldn't generate a response. Please try again later.", event.command.locale)).set_flags(dpp::m_ephemeral));
            co_return;
        }
        nlohmann::json json;
        try {
            json = nlohmann::json::parse(response.body);
        } catch (nlohmann::json::exception &e) {
            bot.log(dpp::ll_error, "JSON parse error after appeal creation. Status: " + std::to_string(response.status) + "\nResponse:\n" + response.body + "\nException:\n" + e.what());
            event.reply(dpp::message(_("Error. Couldn't generate a response. Please try again later.", event.command.locale)).set_flags(dpp::m_ephemeral));
            co_return;
        }
        if (!json.contains("embed")) {
            bot.log(dpp::ll_error, "Appeal invitation doesn't contain an embed for user " + event.command.get_issuing_user().id.str() + "\nResponse: " + response.body);
            event.reply(dpp::message(_("Error. Couldn't generate a response. Please try again later.", event.command.locale)).set_flags(dpp::m_ephemeral));
            co_return;
        }
        auto embed = dpp::embed(&json["embed"]);
        auto result = co_await event.co_reply(dpp::message().add_embed(embed).set_flags(dpp::m_ephemeral));
        if (result.is_error()) {
            bot.log(dpp::ll_error, "Appeal invitation embed could not be sent: " + result.get_error().human_readable);
            event.reply(dpp::message(_("Error. Couldn't generate a response. Please try again later.", event.command.locale)).set_flags(dpp::m_ephemeral));
        }
    }
}