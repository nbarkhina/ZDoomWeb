// web/stubs.cpp
// Minimal stubs for subsystems disabled in the GZDoom web build.
// ZMusic and OpenAL are now compiled/linked for real; only Discord rich
// presence remains stubbed (discord-rpc is dropped from the web build).

// richpresence.cpp is excluded from the web build; provide its one entry point.
void I_UpdateDiscordPresence(bool /*SendPresence*/, const char* /*curstatus*/, const char* /*appid*/, const char* /*steamappid*/)
{
}
