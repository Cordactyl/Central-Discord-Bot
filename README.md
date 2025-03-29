# Cordactyl.com Official Discord-Bot

This is the Open-Source Discord-Bot used for the central [cordactyl.com](https://cordactyl.com/) panel. 
It can be used for individual tenants.

## Getting Started

### Requirements
- Python 3.8 or higher

The Python dependencies are listed in the `requirements.txt`.

### Create a Discord-Bot
Go to the [discord developer portal](https://discord.com/developers/applications) and create a Discord-Bot.
The Discord Bot needs at least the **Server Members** intent along with the following permissions:
- Send Messages
- Embed Links

For the discord Oauth2 login to work, you need to configure the following Oauth2 authentication redirect: `https://cordactyl.com/discord-login` in the "OAuth2" section.

### Installation
1. Clone the project and go into the project directory.
2. Rename the example config `.env.example` to `.env` and configure it with your Discord-Bot token and the Api-Token.
3. Install the Python dependencies in a virtual environment:
   ```shell
   # setup virtual environment
   python3 -m venv venv
   # install python packages into the virtual environment
   ./venv/bin/pip3 install -r requirements.txt
   ```
4. Congratulations. You can now start the discord bot using:
   ```
   ./venv/bin/python3 bot.py
   ```

### Suggestion to run the bot
On a linux server, you can use the following service file to run the bot in a systemd daemon.

```ini
[Unit]
Description=Cordactyl Discord-Bot
After=network.target

[Service]
WorkingDirectory=/path_to_project
ExecStart=/path_to_project/venv/bin/python3 bot.py
Type=simple
Restart=always
# User=discord # If you have setup a custom user for the discord-bot

[Install]
WantedBy=multi-user.target
```

## Localisation
Translations are stored in the `lang` directory. 
Each language file is named by the [IETF language tag](https://en.wikipedia.org/wiki/IETF_language_tag).

## Get in touch
If you want to contribute, just get in touch via our [Discord Server](https://discord.gg/J9QxV6awF6).

## Roadmap
- Implementing a ticket system
- Adding a social score system

## Contributing
Contributions, issues and feature requests are welcome.

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
