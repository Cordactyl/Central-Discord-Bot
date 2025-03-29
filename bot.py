# Copyright 2025 by Phil B.
# All rights reserved. This file is part of the Cordactyl.com panel,
# and licensed under Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International.
# To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-nd/4.0/
# or see the LICENSE file that should have been included as part of this project.
import aiohttp
import asyncio
import discord
from dotenv import load_dotenv
import os


class CordactylClient(discord.Bot):
    def __init__(self, description=None, *args, **options):
        super().__init__(description, *args, **options)
        self.session = None
        self.is_central = False if '|' in os.getenv('API_TOKEN') else True

    async def start(self, token: str, *, reconnect: bool = True) -> None:
        ssl = os.getenv("API_SSL", 'true').lower() in ('true', '1', 't')
        headers = {
            "Authorization": f"Bearer {os.getenv('API_TOKEN')}",
            "Accept": "application/json",
        }
        connector = aiohttp.TCPConnector(ssl=ssl, limit=0, ttl_dns_cache=300)
        timeout = aiohttp.ClientTimeout(connect=10)
        self.session = aiohttp.ClientSession(headers=headers, connector=connector, timeout=timeout, raise_for_status=False)
        await super().start(token, reconnect=reconnect)

    async def close(self):
        await self.session.close()
        # Wait 250 ms for the underlying SSL connections to close
        await asyncio.sleep(0.250)
        await super().close()


load_dotenv()
client = CordactylClient(
    intents=discord.Intents(
        message_content=False,
        guild_messages=False,
        members=True,
        guilds=True,
        voice_states=False,
    ),
    allowed_mentions=discord.AllowedMentions.none(),
)
client.load_extension('cogs.appeals')


@client.event
async def on_ready():
    print(f"Logged in as {client.user.name} ({client.user.id}) with PyCord v{discord.__version__}")


client.run(os.getenv('BOT_TOKEN'))