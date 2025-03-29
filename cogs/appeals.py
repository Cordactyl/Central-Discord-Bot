# Copyright 2025 by Phil B.
# All rights reserved. This file is part of the Cordactyl.com panel,
# and licensed under Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International.
# To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-nd/4.0/
# or see the LICENSE file that should have been included as part of this project.
import os
from datetime import datetime

import aiohttp
import discord
from discord.ext import tasks, commands

from i18n import _


class Appeal(commands.Cog):
    def __init__(self, bot):
        self.bot = bot
        self._batch_messages = []
        self.fetch_queued_messages.start()
        self.queued_messages_bulker.start()

    def cog_unload(self):
        self.fetch_queued_messages.cancel()
        self.queued_messages_bulker.cancel()

    @discord.slash_command(
        name="appeal",
        description="Create an appeal",
        name_localizations={
            "de": "entbannungsantrag",
        },
        description_localizations={
            "de": "Erstelle einen Entbannungsantrag",
        },
    )
    async def appeal(self, ctx: discord.ApplicationContext):
        if ctx.interaction.context != discord.InteractionContextType.guild or not isinstance(ctx.author, discord.Member):
            if self.bot.is_central:
                await ctx.respond('You must be in a guild to use this command.', ephemeral=True)
                return
            mutual_guilds = ctx.author.mutual_guilds
            if len(mutual_guilds) != 1:
                await ctx.respond('You must be in a guild to use this command.', ephemeral=True)
                return
            else:
                member = mutual_guilds[0].get_member(ctx.user.id)
                guild = mutual_guilds[0]
        else:
            member = ctx.author
            guild = ctx.guild

        if ctx.interaction.context != discord.InteractionContextType.bot_dm:
            await ctx.defer(ephemeral=True, invisible=True)
            try:
                await ctx.user.create_dm()
            except discord.Forbidden:
                await ctx.respond(_('appeals.i_cannot_dm_you', guild.preferred_locale), ephemeral=True)
                return
            except discord.DiscordException:
                pass

        body = {
            'guild_id': guild.id,
            'user': {
                'id': ctx.user.id,
                'username': ctx.user.global_name,
                'avatar': ctx.user.avatar.key if ctx.user.avatar is not None else None,
                'default_avatar_index': ctx.user.default_avatar.key,
                'roles': [{'id': r.id, 'name': r.name, 'color': str(r.color)} for r in member.roles if r.id != guild.id]
            },
        }
        url = f"{os.getenv('API_URL')}/appeal-invitations/create"
        timeout = aiohttp.ClientTimeout(total=5)
        try:
            async with self.bot.session.post(url, json=body, timeout=timeout) as resp:
                if resp.status == 503:
                    await ctx.respond(_('maintenance', guild.preferred_locale), ephemeral=True)
                    return
                if resp.status >= 500:
                    print(f"Error while requesting /appeal-invitations/create: {resp}\nBody: {body}")
                    await ctx.respond('Cordactyl API error. Please try again later', ephemeral=True)
                    return
                json = await resp.json()
                if 'embed' not in json:
                    print(f"Error: Appeal invitation doesn't contain an embed for user {ctx.user.id}\nResponse: {json}")
                    await ctx.respond(_('appeals.error_generating_response', guild.preferred_locale), ephemeral=True)
                embed = discord.Embed().from_dict(json['embed'])
                try:
                    await ctx.respond(embed=embed, ephemeral=True)
                except Exception as e:
                    print(f"Error while sending appeal invitation to {ctx.user.id}: {e}\nResponse: {json}")
                    await ctx.respond(_('appeals.error_generating_response', guild.preferred_locale), ephemeral=True)
        except ConnectionError:
            await ctx.respond(_('api_error', guild.preferred_locale), ephemeral=True)

    @tasks.loop(seconds=30)
    async def fetch_queued_messages(self):
        async with self.bot.session.get(f"{os.getenv('API_URL')}/queued-messages", raise_for_status=True) as resp:
            json = await resp.json()
            for message in json['data']:
                is_success = False
                try:
                    is_success = await self.send_queued_message(message)
                except Exception as e:
                    print(e)
                if 'id' in message:
                    self._batch_messages.append({
                        'id': message['id'],
                        'processed_at': datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                        'is_success': is_success,
                    })

    async def send_queued_message(self, message: dict) -> bool:
        if ('recipient' not in message) or ('embed' not in message) or ('id' not in message):
            print(f"Error: Incomplete queued message: {message}")
            return False
        user = await self.bot.get_or_fetch_user(int(message['recipient']))
        if user is None:
            print(f"Discord user with ID {message['recipient']} not found")
            return False
        embed = discord.Embed().from_dict(message['embed'])
        try:
            await user.send(embed=embed)
            return True
        except discord.Forbidden:
            print(f"Forbidden to send queued message to user: {message['recipient']}")
        except Exception as e:
            print(f"Error while sending queued message: {e}\nMessage: {message}")
        return False

    @fetch_queued_messages.before_loop
    async def before_fetch_queued_messages(self):
        await self.bot.wait_until_ready()
        print("Started fetching queued messages")

    @fetch_queued_messages.after_loop
    async def after_fetch_queued_messages(self):
        print("Stopped fetching queued messages")

    @fetch_queued_messages.error
    async def fetch_queued_messages_error(self, error):
        print(f"Error occurred while fetching the queues messages. Error: {error}")

    async def queued_messages_bulk(self):
        if len(self._batch_messages) == 0:
            return
        body = {
            'messages': self._batch_messages,
        }
        backup_batch_messages = self._batch_messages
        self._batch_messages = []
        async with self.bot.session.post(f"{os.getenv('API_URL')}/processed-queued-messages", json=body) as resp:
            if not resp.ok:
                print(f"Error while requesting /processed-queued-messages: {resp}\nBody: {body}")
                for message in backup_batch_messages:
                    self._batch_messages.append(message)

    @tasks.loop(minutes=2)
    async def queued_messages_bulker(self):
        await self.queued_messages_bulk()

    @queued_messages_bulker.after_loop
    async def on_queued_messages_bulker_cancel(self):
        if self.queued_messages_bulker.is_being_cancelled():
            # if we're cancelled and we have some data left...
            # let's push it to the API
            await self.queued_messages_bulk()
            if self._batch_messages:
                print(f"Some queued messages could not be delivered to API on cancellation: {self._batch_messages}")
            self._batch_messages = []


def setup(bot):
    bot.add_cog(Appeal(bot))
