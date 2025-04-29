# Cordactyl.com Official Discord-Bot

This is the Open-Source Discord-Bot used for the central [cordactyl.com](https://cordactyl.com/) panel written in C++. 
It can be used for individual tenants.

## Getting Started

### Requirements

#### Build requirements

- [cmake](https://cmake.org/) (version 3.15+)
- [g++](https://gcc.gnu.org) (version 8 or higher)

#### Included dependencies (in the `/libs` folder)

- [DPP](https://github.com/brainboxdotcc/DPP) (version 10.1.2)

#### External Dependencies (Must be installed)

- [OpenSSL](https://openssl.org/) (For HTTPS, will use whichever `-dev` package comes with your OS)
- [zlib](https://zlib.net/) (For websocket compression, will use whichever `-dev` package comes with your OS)


### Create a Discord-Bot
Go to the [discord developer portal](https://discord.com/developers/applications) and create a Discord-Bot.
The Discord Bot needs at least the **Server Members** intent along with the following permissions:
- Send Messages
- Embed Links

For the discord Oauth2 login to work, you need to configure the following Oauth2 authentication redirect: `https://cordactyl.com/discord-login` in the "OAuth2" section.

## Installation

### Build instructions for Linux:

First, download the project, for example via `git clone https://gitlab.com/cordactyl/discord-bot.git`.

Navigate into the project directory and execute the following commands:

```bash
mkdir build
cd build
cmake ..
make -j8
```

Tip: Replace the number after -j with the number of CPU cores available on your machine for optimal performance.

After the build process completes, an executable will be generated. You can execute this executable to start the Discord bot.

Before running the bot, make sure to set the following environment variables:
- `BOT_TOKEN`
- `API_TOKEN`

> For more detailed instructions and information tailored to your operating system, refer to the [D++ library documentation](https://dpp.dev/installing.html).

### Suggestion to run the bot
On a linux server, you can use the following service file to run the bot in a systemd daemon.

```ini
[Unit]
Description=Cordactyl Discord-Bot
After=network.target

[Service]
WorkingDirectory=/path_to_project
ExecStart=/build/Cordactyl_Bot
Type=simple
Restart=always
# User=cordactyl # If you have setup a custom user for the discord-bot

[Install]
WantedBy=multi-user.target
```

## Localisation
Translations are stored in the `lang` directory.
Each language file must have a locale name from [this list](https://discord.com/developers/docs/reference#locales).

## Get in touch
If you want to contribute, just get in touch via our [Discord Server](https://discord.gg/J9QxV6awF6).

## Roadmap
- Implementing a ticket system
- Adding a social score system
- Moderation features

## Contributing
Issues and feature requests are welcome. Feel free to contribute if you're interested!

## License
This project is licensed under the **Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License (CC BY-NC-ND 4.0)**.

#### What does this mean?
- **Attribution (BY):** You must give appropriate credit when using or referencing this work.
- **NonCommercial (NC):** You may not use this work for commercial purposes.
- **NoDerivatives (ND):** You may not distribute modified versions of this work.

#### Permissions
You are free to:
- View, download, and use the source code for personal and non-commercial purposes.

#### Restrictions
- You may not modify and redistribute this project or any derivatives.
- Commercial use is strictly prohibited.

For more details, see the full license at: https://creativecommons.org/licenses/by-nc-nd/4.0/.

## Show your kindness

Be sure to leave a ⭐️ if you like the project :) Thank you!