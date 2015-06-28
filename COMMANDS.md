
## Command reference
This is a full list of commands supported by the plugin with a description about their function. You can send these commands to the plugin with `OutputDebugMessage`.
Some commands need a parameter, enter a value for the parameter after the command separated by a space. Values themselves may contain spaces.

#### [Communication](#communication-arrow_double_up)
TS3_PTT_ACTIVATE  
TS3_PTT_DEACTIVATE  
TS3_PTT_TOGGLE  
TS3_VAD_ACTIVATE  
TS3_VAD_DEACTIVATE  
TS3_VAD_TOGGLE  
TS3_CT_ACTIVATE  
TS3_CT_DEACTIVATE  
TS3_CT_TOGGLE  
TS3_INPUT_MUTE  
TS3_INPUT_UNMUTE  
TS3_INPUT_TOGGLE  
TS3_OUTPUT_MUTE  
TS3_OUTPUT_UNMUTE  
TS3_OUTPUT_TOGGLE

#### [Server interaction](#server-interaction-arrow_double_up)
TS3_AWAY_ZZZ  
TS3_AWAY_NONE  
TS3_AWAY_TOGGLE  
TS3_ACTIVATE_SERVER  
TS3_ACTIVATE_SERVERID  
TS3_ACTIVATE_SERVERIP  
TS3_ACTIVATE_CURRENT  
TS3_SERVER_NEXT  
TS3_SERVER_PREV  
TS3_JOIN_CHANNEL  
TS3_JOIN_CHANNELID  
TS3_CHANNEL_NEXT  
TS3_CHANNEL_PREV  
TS3_KICK_CLIENT  
TS3_KICK_CLIENTID  
TS3_CHANKICK_CLIENT  
TS3_CHANKICK_CLIENTID  
TS3_BOOKMARK_CONNECT  

#### [Whispering](#whispering-arrow_double_up)
TS3_WHISPER_ACTIVATE  
TS3_WHISPER_DEACTIVATE  
TS3_WHISPER_TOGGLE  
TS3_WHISPER_CLEAR  
TS3_WHISPER_CLIENT  
TS3_WHISPER_CLIENTID  
TS3_WHISPER_CHANNEL  
TS3_WHISPER_CHANNELID  
TS3_REPLY_ACTIVATE  
TS3_REPLY_DEACTIVATE  
TS3_REPLY_TOGGLE  
TS3_REPLY_CLEAR  

#### [Miscellaneous](#miscellaneous-arrow_double_up)
TS3_MUTE_CLIENT  
TS3_MUTE_CLIENTID  
TS3_UNMUTE_CLIENT  
TS3_UNMUTE_CLIENTID  
TS3_MUTE_TOGGLE_CLIENT  
TS3_MUTE_TOGGLE_CLIENTID  
TS3_VOLUME_UP  
TS3_VOLUME_DOWN  
TS3_VOLUME_SET  
TS3_PLUGIN_COMMAND  

### Communication [:arrow_double_up:](#command-reference)

#### Push-to-talk
##### Commands
TS3_PTT_ACTIVATE  
TS3_PTT_DEACTIVATE  
TS3_PTT_TOGGLE  
##### Description
Turns push-to-talk on/off on the currently active server. The push-to-talk release will be delayed if Push-to-talk Delay is enabled in the Default profile.
##### Example
Push-to-talk: activate on key press, deactivate on key release.
```lua
function OnEvent(event, gkey, family)
    mkey = GetMKeyState()
    if gkey == 1 and mkey == 1 then
        if event == "G_PRESSED" then
            OutputDebugMessage("TS3_PTT_ACTIVATE")
        end
        if event == "G_RELEASED" then
            OutputDebugMessage("TS3_PTT_DEACTIVATE")
        end
    end
end
```

#### Voice activation
##### Commands
TS3_VAD_ACTIVATE  
TS3_VAD_DEACTIVATE  
TS3_VAD_TOGGLE  
##### Description
Turns voice activation on/off on the currently active server.

#### Continuous transmission
##### Commands
TS3_CT_ACTIVATE  
TS3_CT_DEACTIVATE  
TS3_CT_TOGGLE  
##### Description
Turns continuous transmission on/off on the currently active server.

#### Mute microphone
##### Commands
TS3_INPUT_MUTE  
TS3_INPUT_UNMUTE  
TS3_INPUT_TOGGLE  
##### Description
Turns microphone on/off on the currently active server.
##### Example
Mute the microphone when G1 is pressed, unmute when G2 is pressed.
```lua
function OnEvent(event, gkey, family)
    mkey = GetMKeyState()
    if gkey == 1 and mkey == 1 then
        if event == "G_PRESSED" then
            OutputDebugMessage("TS3_INPUT_MUTE")
        end
    end
    if gkey == 2 and mkey == 1 then
        if event == "G_PRESSED" then
            OutputDebugMessage("TS3_INPUT_UNMUTE")
        end
    end
end
```

#### Mute speakers/headphones
TS3_OUTPUT_MUTE  
TS3_OUTPUT_UNMUTE  
TS3_OUTPUT_TOGGLE  
##### Description
Turns speakers/headphones on/off on the currently active server.

### Server interaction [:arrow_double_up:](#command-reference)
#### Away status
##### Commands
TS3_AWAY_ZZZ &lt;Message>  
TS3_AWAY_NONE  
TS3_AWAY_TOGGLE &lt;Message>  
##### Description
Turns away status on current servers on/off with an optional message.
##### Example
Toggle away status with the away message "Studying".
```lua
function OnEvent(event, gkey, family)
    mkey = GetMKeyState()
    if gkey == 2 and mkey == 1 then
        if event == "G_PRESSED" then
            OutputDebugMessage("TS3_AWAY_TOGGLE Studying")
        end
    end
end
```

#### Globally away status
##### Commands
TS3_GLOBALAWAY_ZZZ &lt;Message>  
TS3_GLOBALAWAY_NONE  
TS3_GLOBALAWAY_TOGGLE &lt;Message>  
##### Description
Turns away status on all connected servers on/off with an optional message.
#### Activate server
##### Commands
TS3_ACTIVATE_SERVER &lt;Server Name>  
TS3_ACTIVATE_SERVERID &lt;Unique ID>  
TS3_ACTIVATE_SERVERIP &lt;IP Address>  
TS3_ACTIVATE_CURRENT  
TS3_SERVER_NEXT  
TS3_SERVER_PREV  
##### Description
The active server determines where all voice input will be transmitted to and where all commands will be executed. You can change the active server based on Server Name, Unique ID or IP Address. You can also switch to the next/previous server or set the server tab currently opened in the interface as the active server.
##### Example
Set the server named MyServer as the currently active server.
```lua
function OnEvent(event, gkey, family)
    mkey = GetMKeyState()
    if gkey == 1 and mkey == 1 then
        if event == "G_PRESSED" then
            OutputDebugMessage("TS3_ACTIVATE_SERVER MyServer")
        end
    end
end
```

#### Channel joining
##### Commands
TS3_JOIN_CHANNEL &lt;Name/Path>  
TS3_JOIN_CHANNELID &lt;Channel ID>  
TS3_CHANNEL_NEXT  
TS3_CHANNEL_PREV  
##### Description
Join a channel based on Name/Path or Channel ID, you can also join the next/previous channel in the channel list. The complete channel path is not necessary but allows you to be more specific and is described as follows:  
`Parent-channel/Sub-channel`

The Channel ID is only viewable if you have installed the extended info theme and is located next to the channel name.

#### Client kicking
TS3_KICK_CLIENT &lt;Nickname>  
TS3_KICK_CLIENTID &lt;Unique ID>  
TS3_CHANKICK_CLIENT &lt;Nickname>  
TS3_CHANKICK_CLIENTID &lt;Unique ID>  
##### Description
Allows you to kick a client out of the server or channel based on Nickname of Unique ID. The Unique ID is only viewable if you have installed the extended info theme and is located beneath the nickname.
##### Example
Kick the client nicknamed "Annoyance".
```lua
function OnEvent(event, gkey, family)
    mkey = GetMKeyState()
    if gkey == 1 and mkey == 1 then
        if event == "G_PRESSED" then
            OutputDebugMessage("TS3_KICK_CLIENT Annoyance")
        end
    end
end
```

### Whispering [:arrow_double_up:](#command-reference)
#### Activating/clearing the whisper list
##### Commands
TS3_WHISPER_ACTIVATE  
TS3_WHISPER_DEACTIVATE  
TS3_WHISPER_TOGGLE  
TS3_WHISPER_CLEAR  
##### Description
Activates/clears the current whisper list. The whisper list is set per server and is not remembered between sessions, but this is not a problem since you can save the whisper list in the script.

#### Adding clients to the whisper list
##### Commands
TS3_WHISPER_CLIENT &lt;Nickname>
TS3_WHISPER_CLIENTID &lt;Unique ID>
##### Description
Add a client to the current whisper list based on Nickname or Unique ID. The Unique ID is only viewable if you have installed the extended info theme and is located beneath the nickname.

##### Example
Push-to-whisper: Set and activate the whisper list and push-to-talk when pressed, clear the whisper list and push-to-talk when released.
```lua
function OnEvent(event, gkey, family)
    mkey = GetMKeyState()

    if gkey == 1 and mkey == 1 then
        if event == "G_PRESSED" then
            OutputDebugMessage("TS3_WHISPER_CLIENT Bob")
            OutputDebugMessage("TS3_WHISPER_CLIENT Sarah")
            OutputDebugMessage("TS3_WHISPER_ACTIVATE")
            OutputDebugMessage("TS3_PTT_ACTIVATE")
        end
        if event == "G_RELEASED" then
            OutputDebugMessage("TS3_PTT_DEACTIVATE")
            OutputDebugMessage("TS3_WHISPER_CLEAR")
        end
    end
end
```

#### Adding channels to the whisper list
##### Commands
TS3_WHISPER_CHANNEL &lt;Name/Path>
TS3_WHISPER_CHANNELID &lt;Channel ID>
##### Description
Add a channel to the current whisper list based on Name/Path or Channel ID. The complete channel path is not necessary but allows you to be more specific and is described as follows:  
`Parent-channel/Sub-channel`

The Channel ID is only viewable if you have installed the extended info theme and is located next to the channel name.
##### Example
Push-to-whisper: Set and activate the whisper list and push-to-talk when pressed, clear the whisper list and push-to-talk when released.
```lua
function OnEvent(event, gkey, family)
    mkey = GetMKeyState()

    if gkey == 1 and mkey == 1 then
        if event == "G_PRESSED" then
            OutputDebugMessage("TS3_WHISPER_CHANNEL Channel 1")
            OutputDebugMessage("TS3_WHISPER_ACTIVATE")
            OutputDebugMessage("TS3_PTT_ACTIVATE")
        end
        if event == "G_RELEASED" then
            OutputDebugMessage("TS3_PTT_DEACTIVATE")
            OutputDebugMessage("TS3_WHISPER_CLEAR")
        end
    end
end
```

#### Replying to whispers
##### Commands
TS3_REPLY_ACTIVATE  
TS3_REPLY_DEACTIVATE  
TS3_REPLY_TOGGLE  
TS3_REPLY_CLEAR  
##### Description
Activates/clears the current reply list. This is a special whisper list set per server that remembers the clients that have recently whispered to you, it is not remembered between sessions.
##### Example
Push-to-reply: Activate the reply list and push-to-talk when pressed, clear the reply list and push-to-talk when released.
```lua
function OnEvent(event, gkey, family)
    mkey = GetMKeyState()

    if gkey == 1 and mkey == 1 then
        if event == "G_PRESSED" then
            OutputDebugMessage("TS3_REPLY_ACTIVATE")
            OutputDebugMessage("TS3_PTT_ACTIVATE")
        end
        if event == "G_RELEASED" then
            OutputDebugMessage("TS3_PTT_DEACTIVATE")
            OutputDebugMessage("TS3_REPLY_CLEAR")
        end
    end
end
```

#### Connect to bookmark
##### Commands
TS3_BOOKMARK_CONNECT &lt;Label>
##### Description
Allows you to connect to a bookmarked server by specifying the label of the bookmark.

### Miscellaneous [:arrow_double_up:](#command-reference)
#### Client muting
##### Commands
TS3_MUTE_CLIENT &lt;Nickname>  
TS3_MUTE_CLIENTID &lt;Unique ID>  
TS3_UNMUTE_CLIENT &lt;Nickname>  
TS3_UNMUTE_CLIENTID &lt;Unique ID>  
TS3_MUTE_TOGGLE_CLIENT &lt;Nickname>  
TS3_MUTE_TOGGLE_CLIENTID &lt;Unique ID>  
##### Description
Allows you to mute a client based on Nickname or Unique ID. The Unique ID is only viewable if you have installed the extended info theme and is located beneath the nickname.
#### Master volume
##### Commands
TS3_VOLUME_UP &lt;Amount>  
TS3_VOLUME_DOWN &lt;Amount>  
TS3_VOLUME_SET &lt;Value>  
##### Description
Changes the master output volume, you can increment/decrement it with an optionally specified amount (default 1) or you can set it to a desired value. The key press is not repeated so you will have to press the key multiple times if you want to increase the volume further.

**Due to a technical limitation the change in volume is not displayed in the interface.**

#### Plugin commands
##### Commands
TS3_PLUGIN_COMMAND <command>
##### Description
Allows you to send a command to other enabled TeamSpeak3 plugins as you would from the console. The slash (/) prefix may be omitted.
##### Example
Test Plugin: Instruct the Test Plugin to join the channel with channel ID 1.
```lua
function OnEvent(event, gkey, family)
    mkey = GetMKeyState()

    if gkey == 1 and mkey == 1 then
        if event == "G_PRESSED" then
            OutputDebugMessage("TS3_PLUGIN_COMMAND /test join 1")
        end
    end
end
```
[:arrow_double_up:](#command-reference)
