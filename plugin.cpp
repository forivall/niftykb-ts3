/*
 * TeamSpeak 3 G-key plugin
 * Author: Jules Blok (jules@aerix.nl)
 *
 * Copyright (c) 2008-2012 TeamSpeak Systems GmbH
 */

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#include <TlHelp32.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "public_definitions.h"
#include "public_rare_definitions.h"
#include "ts3_functions.h"
#include "plugin.h"
#include "functions.h"

struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); dest[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 16

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128
#define REQUESTCLIENTMOVERETURNCODES_SLOTS 5

#define PLUGINTHREAD_TIMEOUT 1000

#define TIMER_MSEC 10000

/* Array for request client move return codes. See comments within ts3plugin_processCommand for details */
static char requestClientMoveReturnCodes[REQUESTCLIENTMOVERETURNCODES_SLOTS][RETURNCODE_BUFSIZE];

// Plugin values
static char* pluginID = NULL;
static bool pluginRunning = false;
static char configFile[MAX_PATH];
static char errorSound[MAX_PATH];
static char infoIcon[MAX_PATH];

// Error codes
enum PluginError {
	PLUGIN_ERROR_NONE = 0,
	PLUGIN_ERROR_HOOK_FAILED,
	PLUGIN_ERROR_READ_FAILED,
	PLUGIN_ERROR_NOT_FOUND
};

// Thread handles
static HANDLE hDebugThread = NULL;
static HANDLE hPTTDelayThread = NULL;

// PTT Delay Timer
static HANDLE hPttDelayTimer = (HANDLE)NULL;
static LARGE_INTEGER dueTime;

// Active server
static uint64 scHandlerID = (uint64)NULL;

/*********************************** Plugin functions ************************************/

VOID CALLBACK PTTDelayCallback(LPVOID lpArgToCompletionRoutine,DWORD dwTimerLowValue,DWORD dwTimerHighValue)
{
	SetPushToTalk(scHandlerID, false);
}

void ParseCommand(char* cmd, char* arg)
{
	/***** Communication *****/
	if(!strcmp(cmd, "TS3_PTT_ACTIVATE"))
	{
		if(pttActive) CancelWaitableTimer(hPttDelayTimer);
		SetPushToTalk(scHandlerID, true);
	}
	else if(!strcmp(cmd, "TS3_PTT_DEACTIVATE"))
	{
		char str[6];
		GetPrivateProfileStringA("Profiles", "Capture\\Default\\PreProcessing\\delay_ptt", "false", str, 10, configFile);
		if(!strcmp(str, "true"))
		{
			GetPrivateProfileStringA("Profiles", "Capture\\Default\\PreProcessing\\delay_ptt_msecs", "300", str, 10, configFile);
			dueTime.QuadPart = 0 - atoi(str) * TIMER_MSEC;

			SetWaitableTimer(hPttDelayTimer, &dueTime, 0, PTTDelayCallback, NULL, FALSE);
		}
		else
		{
			SetPushToTalk(scHandlerID, false);
		}
	}
	else if(!strcmp(cmd, "TS3_PTT_TOGGLE"))
	{
		if(pttActive) CancelWaitableTimer(hPttDelayTimer);
		SetPushToTalk(scHandlerID, !pttActive);
	}
	else if(!strcmp(cmd, "TS3_VAD_ACTIVATE"))
	{
		SetVoiceActivation(scHandlerID, true);
	}
	else if(!strcmp(cmd, "TS3_VAD_DEACTIVATE"))
	{
		SetVoiceActivation(scHandlerID, false);
	}
	else if(!strcmp(cmd, "TS3_VAD_TOGGLE"))
	{
		SetVoiceActivation(scHandlerID, !vadActive);
	}
	else if(!strcmp(cmd, "TS3_CT_ACTIVATE"))
	{
		SetContinuousTransmission(scHandlerID, true);
	}
	else if(!strcmp(cmd, "TS3_CT_DEACTIVATE"))
	{
		SetContinuousTransmission(scHandlerID, false);
	}
	else if(!strcmp(cmd, "TS3_CT_TOGGLE"))
	{
		SetContinuousTransmission(scHandlerID, !inputActive);
	}
	else if(!strcmp(cmd, "TS3_INPUT_MUTE"))
	{
		SetInputMute(scHandlerID, true);
	}
	else if(!strcmp(cmd, "TS3_INPUT_UNMUTE"))
	{
		SetInputMute(scHandlerID, false);
	}
	else if(!strcmp(cmd, "TS3_INPUT_TOGGLE"))
	{
		int muted;
		ts3Functions.getClientSelfVariableAsInt(scHandlerID, CLIENT_INPUT_MUTED, &muted);
		SetInputMute(scHandlerID, !muted);
	}
	else if(!strcmp(cmd, "TS3_OUTPUT_MUTE"))
	{
		SetOutputMute(scHandlerID, true);
	}
	else if(!strcmp(cmd, "TS3_OUTPUT_UNMUTE"))
	{
		SetOutputMute(scHandlerID, false);
	}
	else if(!strcmp(cmd, "TS3_OUTPUT_TOGGLE"))
	{
		int muted;
		ts3Functions.getClientSelfVariableAsInt(scHandlerID, CLIENT_OUTPUT_MUTED, &muted);
		SetOutputMute(scHandlerID, !muted);
	}
	/***** Server interaction *****/
	else if(!strcmp(cmd, "TS3_AWAY_ZZZ"))
	{
		SetGlobalAway(true);
	}
	else if(!strcmp(cmd, "TS3_AWAY_NONE"))
	{
		SetGlobalAway(false);
	}
	else if(!strcmp(cmd, "TS3_AWAY_TOGGLE"))
	{
		int away;
		ts3Functions.getClientSelfVariableAsInt(scHandlerID, CLIENT_AWAY, &away);
		SetGlobalAway(!away);
	}
	else if(!strcmp(cmd, "TS3_ACTIVATE_SERVER"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			uint64 handle;
			GetServerHandleByVariable(arg, VIRTUALSERVER_NAME, &handle);
			if(handle != (uint64)NULL && handle != scHandlerID)
			{
				CancelWaitableTimer(hPttDelayTimer);
				SetPushToTalk(scHandlerID, TRUE);
				WhisperListClear(scHandlerID);
				SetActiveServer(handle);
			}
			else ErrorMessage(scHandlerID, "Server not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_ACTIVATE_SERVERID"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			uint64 handle;
			GetServerHandleByVariable(arg, VIRTUALSERVER_UNIQUE_IDENTIFIER, &handle);
			if(handle != (uint64)NULL)
			{
				CancelWaitableTimer(hPttDelayTimer);
				SetPushToTalk(scHandlerID, TRUE);
				WhisperListClear(scHandlerID);
				SetActiveServer(handle);
			}
			else ErrorMessage(scHandlerID, "Server not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_JOIN_CHAN"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			uint64 id;
			GetChannelIDByVariable(scHandlerID, arg, CHANNEL_NAME, &id);
			if(id != (uint64)NULL) JoinChannel(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Channel not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_JOIN_CHANID"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			uint64 id = atoi(arg);
			if(id != (uint64)NULL) JoinChannel(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Channel not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_KICK_CLIENT"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			anyID id;
			GetClientIDByVariable(scHandlerID, arg, CLIENT_NICKNAME, &id);
			if(id != (anyID)NULL) ServerKickClient(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Client not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_KICK_CLIENTID"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			anyID id;
			GetClientIDByVariable(scHandlerID, arg, CLIENT_UNIQUE_IDENTIFIER, &id);
			if(id != (anyID)NULL) ServerKickClient(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Client not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_CHANKICK_CLIENT"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			anyID id;
			GetClientIDByVariable(scHandlerID, arg, CLIENT_NICKNAME, &id);
			if(id != (anyID)NULL) ChannelKickClient(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Client not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_CHANKICK_CLIENTID"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			anyID id;
			GetClientIDByVariable(scHandlerID, arg, CLIENT_UNIQUE_IDENTIFIER, &id);
			if(id != (anyID)NULL) ChannelKickClient(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Client not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	/***** Whispering *****/
	else if(!strcmp(cmd, "TS3_WHISPER_ACTIVATE"))
	{
		SetWhisperList(scHandlerID, TRUE);
	}
	else if(!strcmp(cmd, "TS3_WHISPER_DEACTIVATE"))
	{
		SetWhisperList(scHandlerID, FALSE);
	}
	else if(!strcmp(cmd, "TS3_WHISPER_TOGGLE"))
	{
		SetWhisperList(scHandlerID, !whisperActive);
	}
	else if(!strcmp(cmd, "TS3_WHISPER_CLEAR"))
	{
		WhisperListClear(scHandlerID);
	}
	else if(!strcmp(cmd, "TS3_WHISPER_ADD_CLIENT"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			anyID id;
			GetClientIDByVariable(scHandlerID, arg, CLIENT_NICKNAME, &id);
			if(id != (anyID)NULL) WhisperAddClient(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Client not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_WHISPER_ADD_CLIENTID"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			anyID id;
			GetClientIDByVariable(scHandlerID, arg, CLIENT_UNIQUE_IDENTIFIER, &id);
			if(id != (anyID)NULL) WhisperAddClient(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Client not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_WHISPER_ADD_CHAN"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			uint64 id;
			GetChannelIDByVariable(scHandlerID, arg, CHANNEL_NAME, &id);
			if(id != (uint64)NULL) WhisperAddChannel(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Channel not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_WHISPER_ADD_CHANID"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			uint64 id = atoi(arg);
			if(id != (uint64)NULL) WhisperAddChannel(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Channel not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	/***** Miscellaneous *****/
	else if(!strcmp(cmd, "TS3_MUTE_CLIENT"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			anyID id;
			GetClientIDByVariable(scHandlerID, arg, CLIENT_NICKNAME, &id);
			if(id != (anyID)NULL) MuteClient(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Client not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_MUTE_CLIENTID"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			anyID id;
			GetClientIDByVariable(scHandlerID, arg, CLIENT_UNIQUE_IDENTIFIER, &id);
			if(id != (anyID)NULL) MuteClient(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Client not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_UNMUTE_CLIENT"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			anyID id;
			GetClientIDByVariable(scHandlerID, arg, CLIENT_NICKNAME, &id);
			if(id != (anyID)NULL) UnmuteClient(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Client not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_UNMUTE_CLIENTID"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			anyID id;
			GetClientIDByVariable(scHandlerID, arg, CLIENT_UNIQUE_IDENTIFIER, &id);
			if(id != (anyID)NULL) UnmuteClient(scHandlerID, id);
			else ErrorMessage(scHandlerID, "Client not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_MUTE_TOGGLE_CLIENT"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			anyID id;
			GetClientIDByVariable(scHandlerID, arg, CLIENT_NICKNAME, &id);
			if(id != (anyID)NULL)
			{
				int muted;
				ts3Functions.getClientVariableAsInt(scHandlerID, id, CLIENT_IS_MUTED, &muted);
				if(!muted) MuteClient(scHandlerID, id);
				else UnmuteClient(scHandlerID, id);
			}
			else ErrorMessage(scHandlerID, "Client not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_MUTE_TOGGLE_CLIENTID"))
	{
		if(arg != NULL && *arg != (char)NULL)
		{
			anyID id;
			GetClientIDByVariable(scHandlerID, arg, CLIENT_UNIQUE_IDENTIFIER, &id);
			if(id != (anyID)NULL)
			{
				int muted;
				ts3Functions.getClientVariableAsInt(scHandlerID, id, CLIENT_IS_MUTED, &muted);
				if(!muted) MuteClient(scHandlerID, id);
				else UnmuteClient(scHandlerID, id);
			}
			else ErrorMessage(scHandlerID, "Client not found", infoIcon, errorSound);
		}
		else ErrorMessage(scHandlerID, "Missing argument", infoIcon, errorSound);
	}
	else if(!strcmp(cmd, "TS3_VOLUME_UP"))
	{
		float value;
		ts3Functions.getPlaybackConfigValueAsFloat(scHandlerID, "volume_modifier", &value);
		SetMasterVolume(scHandlerID, value+1.0f);
	}
	else if(!strcmp(cmd, "TS3_VOLUME_DOWN"))
	{
		float value;
		ts3Functions.getPlaybackConfigValueAsFloat(scHandlerID, "volume_modifier", &value);
		SetMasterVolume(scHandlerID, value-1.0f);
	}
	else if(!strcmp(cmd, "TS3_VOLUME_SET"))
	{
		float value = (float)atof(arg);
		SetMasterVolume(scHandlerID, value);
	}
	/***** Error handler *****/
	else
	{
		ts3Functions.logMessage("Command not recognized:", LogLevel_WARNING, "G-Key Plugin", 0);
		ts3Functions.logMessage(cmd, LogLevel_INFO, "G-Key Plugin", 0);
		ErrorMessage(scHandlerID, "Command not recognized", infoIcon, errorSound);
	}
}

int GetLogitechProcessId(DWORD* ProcessId)
{
	PROCESSENTRY32 entry;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, (DWORD)NULL);

	entry.dwSize = sizeof(PROCESSENTRY32);

	if(Process32First(snapshot, &entry))
	{
		while(Process32Next(snapshot, &entry))
		{
			if(!wcscmp(entry.szExeFile, L"LCore.exe"))
			{
				*ProcessId = entry.th32ProcessID;
				CloseHandle(snapshot);
				return 0;
			}
			else if(!wcscmp(entry.szExeFile, L"LGDCore.exe")) // Legacy support
			{
				*ProcessId = entry.th32ProcessID;
				CloseHandle(snapshot);
				return 0;
			} 
		}
	}

	CloseHandle(snapshot);
	return 1; // No processes found
}

void DebugMain(DWORD ProcessId, HANDLE hProcess)
{
	DEBUG_EVENT DebugEv; // Buffer for debug messages

	// While the plugin is running
	while(pluginRunning)
	{
		// Wait for a debug message
		if(WaitForDebugEvent(&DebugEv, PLUGINTHREAD_TIMEOUT))
		{
			// If the debug message is from the logitech driver
			if(DebugEv.dwProcessId == ProcessId)
			{
				// If this is a debug message and it uses ANSI
				if(DebugEv.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT && !DebugEv.u.DebugString.fUnicode)
				{
					char *DebugStr, *arg;

					// Retrieve debug string
					DebugStr = (char*)malloc(DebugEv.u.DebugString.nDebugStringLength);
					ReadProcessMemory(hProcess, DebugEv.u.DebugString.lpDebugStringData, DebugStr, DebugEv.u.DebugString.nDebugStringLength, NULL);
					
					// Continue the process
					ContinueDebugEvent(DebugEv.dwProcessId, DebugEv.dwThreadId, DBG_CONTINUE);

					// Seperate the argument from the command
					arg = strchr(DebugStr, ' ');
					if(arg != NULL)
					{
						// Split the string by inserting a NULL-terminator
						*arg = (char)NULL;
						arg++;
					}

					// Parse debug string
					ParseCommand(DebugStr, arg);

					// Free the debug string
					free(DebugStr);
				}
				else if(DebugEv.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
				{
					// The process is shutting down, exit the debugger
					return;
				}
				else if(DebugEv.dwDebugEventCode == EXCEPTION_DEBUG_EVENT && DebugEv.u.Exception.ExceptionRecord.ExceptionCode != STATUS_BREAKPOINT)
				{
					// The process has crashed, exit the debugger
					return;
				}
				else ContinueDebugEvent(DebugEv.dwProcessId, DebugEv.dwThreadId, DBG_CONTINUE); // Continue the process
			}
			else ContinueDebugEvent(DebugEv.dwProcessId, DebugEv.dwThreadId, DBG_CONTINUE); // Continue the process
		}
	}
}

/*********************************** Plugin threads ************************************/
/*
 * NOTE: Never let threads sleep longer than PLUGINTHREAD_TIMEOUT per iteration,
 * the shutdown procedure will not wait that long for the thread to exit.
 */

DWORD WINAPI DebugThread(LPVOID pData)
{
	DWORD ProcessId; // Process ID for the Logitech software
	HANDLE hProcess; // Handle for the Logitech software

	// Get process id of the logitech software
	if(GetLogitechProcessId(&ProcessId))
	{
		ts3Functions.logMessage("Could not find Logitech software", LogLevel_ERROR, "G-Key Plugin", 0);
		pluginRunning = FALSE;
		return PLUGIN_ERROR_NOT_FOUND;
	}

	// Open a read memory handle to the Logitech software
	hProcess = OpenProcess(PROCESS_VM_READ, FALSE, ProcessId);
	if(hProcess==NULL)
	{
		ts3Functions.logMessage("Failed to open Logitech software for reading", LogLevel_ERROR, "G-Key Plugin", 0);
		pluginRunning = FALSE;
		return PLUGIN_ERROR_READ_FAILED;
	}

	// Attach debugger to Logitech software
	if(!DebugActiveProcess(ProcessId))
	{
		// Could not attach debugger, exit debug thread
		ts3Functions.logMessage("Failed to attach debugger", LogLevel_ERROR, "G-Key Plugin", 0);
		pluginRunning = FALSE;
		return PLUGIN_ERROR_HOOK_FAILED;
	}

	ts3Functions.logMessage("Debugger attached to Logitech software", LogLevel_INFO, "G-Key Plugin", 0);
	DebugMain(ProcessId, hProcess);

	// Dettach the debugger
	DebugActiveProcessStop(ProcessId);
	ts3Functions.logMessage("Debugger detached from Logitech software", LogLevel_INFO, "G-Key Plugin", 0);

	// Close the handle to the Logitech software
	CloseHandle(hProcess);

	return 0;
}

/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
    return "G-Key Plugin";
}

/* Plugin version */
const char* ts3plugin_version() {
    return "0.5.2";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
    return "Jules Blok";
}

/* Plugin description */
const char* ts3plugin_description() {
    return "This plugin allows you to use the macro G-keys on any Logitech device to control TeamSpeak 3 without rebinding them to other keys.\n\nPlease read the G-Key ReadMe in the plugins folder for instructions, also available at http://jules.aerix.nl/g-key";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
	size_t length;

	// Find config file
	ts3Functions.getConfigPath(configFile, MAX_PATH);
	strcat_s(configFile, MAX_PATH, "ts3clientui_qt.conf");

	// Find error sound
	ts3Functions.getResourcesPath(errorSound, MAX_PATH);
	strcat_s(errorSound, MAX_PATH, "sound/");
	length = strlen(errorSound);
	GetPrivateProfileStringA("Notifications", "SoundPack", "default", errorSound+length, MAX_PATH-(DWORD)length, configFile);
	strcat_s(errorSound, MAX_PATH, "/error.wav");

	// Find info icon
	ts3Functions.getResourcesPath(infoIcon, MAX_PATH);
	strcat_s(infoIcon, MAX_PATH, "gfx/");
	length = strlen(infoIcon);
	GetPrivateProfileStringA("Application", "IconPack", "default", infoIcon+length, MAX_PATH-(DWORD)length, configFile);
	strcat_s(infoIcon, MAX_PATH, "/16x16_message_info.png");

	// Get first connection handler
	scHandlerID = ts3Functions.getCurrentServerConnectionHandlerID();

	// Create the PTT delay timer
	hPttDelayTimer = CreateWaitableTimer(NULL, FALSE, NULL);

	// Start the plugin threads
	pluginRunning = true;
	hDebugThread = CreateThread(NULL, (SIZE_T)NULL, DebugThread, 0, 0, NULL);

	if(hDebugThread==NULL)
	{
		ts3Functions.logMessage("Failed to start threads, unloading plugin", LogLevel_ERROR, "G-Key Plugin", 0);
		return 1;
	}

	/* Initialize return codes array for requestClientMove */
	memset(requestClientMoveReturnCodes, 0, REQUESTCLIENTMOVERETURNCODES_SLOTS * RETURNCODE_BUFSIZE);

    return 0;  /* 0 = success, 1 = failure */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
	// Stop the plugin threads
	pluginRunning = FALSE;
	
	// Cancel PTT delay timer
	CancelWaitableTimer(hPttDelayTimer);

	// Wait for the thread to stop
	WaitForSingleObject(hDebugThread, PLUGINTHREAD_TIMEOUT);

	/*
	 * Note:
	 * If your plugin implements a settings dialog, it must be closed and deleted here, else the
	 * TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	 */

	/* Free pluginID if we registered it */
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

/****************************** Optional functions ********************************/
/*
 * Following functions are optional, if not needed you don't need to implement them.
 */

/* Tell client if plugin offers a configuration window. If this function is not implemented, it's an assumed "does not offer" (PLUGIN_OFFERS_NO_CONFIGURE). */
int ts3plugin_offersConfigure() {
	/*
	 * Return values:
	 * PLUGIN_OFFERS_NO_CONFIGURE         - Plugin does not implement ts3plugin_configure
	 * PLUGIN_OFFERS_CONFIGURE_NEW_THREAD - Plugin does implement ts3plugin_configure and requests to run this function in an own thread
	 * PLUGIN_OFFERS_CONFIGURE_QT_THREAD  - Plugin does implement ts3plugin_configure and requests to run this function in the Qt GUI thread
	 */
	return PLUGIN_OFFERS_NO_CONFIGURE;  /* In this case ts3plugin_configure does not need to be implemented */
}

/* Plugin might offer a configuration window. If ts3plugin_offersConfigure returns 0, this function does not need to be implemented. */
void ts3plugin_configure(void* handle, void* qParentWidget) {
}

/*
 * If the plugin wants to use error return codes or plugin commands, it needs to register a command ID. This function will be automatically
 * called after the plugin was initialized. This function is optional. If you don't use error return codes or plugin commands, the function
 * can be omitted.
 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
 */
void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
}

/* Plugin command keyword. Return NULL or "" if not used. */
const char* ts3plugin_commandKeyword() {
	return NULL;
}

/* Plugin processes console command. Return 0 if plugin handled the command, 1 if not handled. */
int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command) {
	return 1;  /* Plugin did not handle command */
}

/* Client changed current server connection handler */
void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID) {
	scHandlerID = serverConnectionHandlerID;
}

/*
 * Implement the following three functions when the plugin should display a line in the server/channel/client info.
 * If any of ts3plugin_infoTitle, ts3plugin_infoData or ts3plugin_freeMemory is missing, the info text will not be displayed.
 */

/* Static title shown in the left column in the info frame */
const char* ts3plugin_infoTitle() {
	return NULL;
}

/*
 * Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
 * function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
 * Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
 * "data" to NULL to have the client ignore the info data.
 */
void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data) {
}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData */
void ts3plugin_freeMemory(void* data) {
	free(data);
}

/*
 * Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
 * the user manually disabled it in the plugin dialog.
 * This function is optional. If missing, no autoload is assumed.
 */
int ts3plugin_requestAutoload() {
	return 1;  /* 1 = request autoloaded, 0 = do not request autoload */
}

/* Show an error message if the plugin failed to load */
void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber) {
    if(newStatus == STATUS_CONNECTION_ESTABLISHED)
	{
		if(!pluginRunning) 
		{
			DWORD errorCode;
			if(GetExitCodeThread(hDebugThread, &errorCode))
			{
				switch(errorCode)
				{
					case PLUGIN_ERROR_HOOK_FAILED: ErrorMessage(serverConnectionHandlerID, "Could not hook into Logitech software, make sure you're using the 64-bit version", infoIcon, errorSound); break;
					case PLUGIN_ERROR_READ_FAILED: ErrorMessage(serverConnectionHandlerID, "Not enough permissions to hook into Logitech software, try running as as administrator", infoIcon, errorSound); break;
					case PLUGIN_ERROR_NOT_FOUND: ErrorMessage(serverConnectionHandlerID, "Logitech software not running, start the Logitech software and reload the G-Key Plugin", infoIcon, errorSound); break;
					default: ErrorMessage(serverConnectionHandlerID, "G-Key Plugin failed to start, check the clientlog for more info", infoIcon, errorSound); break;
				}
			}
		}
	}
}