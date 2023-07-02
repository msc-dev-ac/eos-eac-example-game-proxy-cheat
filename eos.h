#pragma once
#include <stdint.h>

typedef int32_t EOS_EResult;
typedef int32_t EOS_EAntiCheatClientMode;
typedef uint64_t EOS_NotificationId;
typedef struct EOS_AntiCheatClientHandle* EOS_HAntiCheatClient;
typedef struct EOS_ProductUserIdDetails* EOS_ProductUserId;
typedef struct EOS_PlatformHandle* EOS_HPlatform;

typedef void (* EOS_AntiCheatClient_OnMessageToServerCallback)(const void* Data);
typedef void (* EOS_AntiCheatClient_OnClientIntegrityViolatedCallback)(const void* Data);

