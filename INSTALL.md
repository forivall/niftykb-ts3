## Plugin Installation
1. Open and install the package.  
   **Note for 64-bit users:** If you are using a 64-bit operating  system you will need to use the 64-bit version of TeamSpeak 3.

2. Start TeamSpeak 3 and open the "Plugins" dialog (<kbd>Ctrl+Shift+P</kbd>).

3. Make sure the "G-Key Plugin" is checked.

4. Now to configure the G-Keys for usage with the plugin refer to the Tutorial section.

## Tutorial
This section will explain how to configure your Logitech software for use with the G-Key plugin, most users should refer to the Logitech Gaming Software tutorial. These are the latest drivers released by Logitech and a lot of devices are supported by it. You should update if you're still using older software.

However if you have a keyboard the old GamePanel software is still supported for now, refer to the GamePanel Software (Legacy) tutorial if you do not wish to update. (not converted)

### Logitech Gaming Software
1. Open the Logitech Gaming Software

2. Open the "Customize G-keys" tab

3. Open the script for your configuration by right-clicking on the icon.

4. Now enter the script, the simplest script that only supports push-to-talk is this:
```lua
function OnEvent(event, gkey, family)
    mkey = GetMKeyState()

    if gkey == 3 and mkey == 1 then
        if event == "G_PRESSED" then
            OutputDebugMessage("TS3_PTT_ACTIVATE")
        end
        if event == "G_RELEASED" then
            OutputDebugMessage("TS3_PTT_DEACTIVATE")
        end
    end
end
```

With this script G3 will trigger Push-to-talk regardless of your currently assigned hotkey in TeamSpeak 3.

To change Push-to-talk key you only have to change the following line:
`if gkey == 3 and mkey == 1 then`

In that line `gkey == 3` determines which G-Key activates push-to-talk, for example to change the key to G5 you change it to `gkey == 5`.

The `mkey == 1` part is meant for keyboards that have M-Keys and can be changed in the same way as the G-Key. If you don't have M-Keys or you don't use them you can just leave it at `mkey == 1` or remove it.

You have now successfully configured the software to activate Push-To-Talk in TeamSpeak 3.

There are a lot more commands that you can use, refer to the Command reference for a full list of commands.

For advanced scripting examples, refer to the Advanced scripting section.

## Advanced scripting
This section contains explanations and example scripts for advanced users. If you just want to use Push-to-talk you should refer to the tutorial above.

All scripts should have the following layout:
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

You can change `gkey == 1` and `mkey == 1` to the G-key and M-Key you want to use. Replace the string in `OutputDebugMessage` with the command you want to execute. The `G_PRESSED` event should contain the commands you want executed when pressing the key and `G_RELEASED` the commands on releasing the key. For a full list of commands refer to the Command reference.

### Different commands per device
If you use a non-keyboard device with G-Keys like the G930 headset in combination with a keyboard and you want the keys on the headset to activate different commands than the keys on the keyboard use the following variation on the script:

```lua
function OnEvent(event, gkey, family)
    mkey = GetMKeyState()

    -- commands on keyboard
    if family == "kb" or family == "lhc" then
        if gkey == 1 and mkey == 1 then
            if event == "G_PRESSED" then
                OutputDebugMessage("TS3_PTT_ACTIVATE")
            end
            if event == "G_RELEASED" then
                OutputDebugMessage("TS3_PTT_DEACTIVATE")
            end
        end
    else -- commands on other devices
        if gkey == 2 then
            if event == "G_PRESSED" then
                OutputDebugMessage("TS3_PTT_ACTIVATE")
            end
            if event == "G_RELEASED" then
                OutputDebugMessage("TS3_PTT_DEACTIVATE")
            end
        end
    end
end
```

### Mouse support
Some Logitech mice with G-Keys like the G300 are supported from the Logitech Gaming Software, but they use slightly different events. If your mouse is supported, use the following variation on the script:

```lua
function OnEvent(event, gkey, family)
    if gkey == 6 then
        if event == "MOUSE_BUTTON_PRESSED" then
            OutputDebugMessage("TS3_PTT_ACTIVATE")
        end
        if event == "MOUSE_BUTTON_RELEASED" then
            OutputDebugMessage("TS3_PTT_ACTIVATE")
        end
    end
end
```
