#include "pch.h"
#include "aimassist.h"

#include "convar.h"
#include "r2engine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

AUTOHOOK_INIT();

static ConVar* sv_soup_aimassist_multiplier = nullptr;
static ConVar* sv_soup_aimassist_snapshot_fix = nullptr;
static ConVar* sv_soup_aimassist_fps_independence_fix = nullptr;

static bool s_prevAdsActive = false;
static bool s_latestAttackHeld = false;
static bool s_latestAdsInputHeld = false;
static float s_timeSinceHipfireAttack = 999.0f;
static float s_snapshotBlockTimeRemaining = 0.0f;

constexpr uint32_t IN_ATTACK = 1u << 0;
constexpr uint32_t IN_ATTACK2 = 1u << 11;
constexpr uint32_t IN_ZOOM = 1u << 19;

constexpr int kCmdHistorySize = 300;
constexpr int kUserCmdStride = 328;
constexpr int kInputCmdBufferOffset = 248;
constexpr int kUserCmdButtonsOffset = 64;

constexpr float kMaxTrackedTime = 999.0f;
constexpr float kSnapshotPrefireWindowSeconds = 0.20f;
constexpr float kSnapshotBlockSeconds = 0.20f;
constexpr float kMinCmdFrameSeconds = 1.0f / 200.0f; // 200 FPS cap
constexpr float kMaxValidFrameSeconds = 1.0f / 20.0f; // ignore large hitch samples

static float GetFrameSeconds(float frameSecondsHint)
{
	float frameSeconds = 0.0f;

	if (frameSecondsHint > 0.0f && frameSecondsHint <= kMaxValidFrameSeconds)
		frameSeconds = frameSecondsHint;

	if (frameSeconds <= 0.0f && R2::g_pEngine)
	{
		const float frameTime = R2::g_pEngine->m_flFrameTime;
		if (frameTime > 0.0f && frameTime <= kMaxValidFrameSeconds)
			frameSeconds = frameTime;
	}

	if (frameSeconds <= 0.0f)
		frameSeconds = 1.0f / 144.0f;

	// Mirror CL_Move-style effective cap: do not treat faster-than-200fps as smaller dt.
	return std::max(frameSeconds, kMinCmdFrameSeconds);
}

static float GetFpsNormalisationScale(float frameSecondsHint)
{
	if (!IsAimAssistFpsIndependenceFixEnabled())
		return 1.0f;

	const float frameTime = GetFrameSeconds(frameSecondsHint);

	constexpr float kReferenceFps = 144.0f;
	constexpr float kMinScale = 0.25f;
	constexpr float kMaxScale = 4.0f;
	return std::clamp(frameTime * kReferenceFps, kMinScale, kMaxScale);
}

static float GetAimAssistScale(float frameSecondsHint, bool applyFpsNormalisation)
{
	float scale = std::max(0.0f, GetAimAssistMultiplier());
	if (applyFpsNormalisation)
		scale *= GetFpsNormalisationScale(frameSecondsHint);
	return scale;
}

static float GetGlobalAimAssistMultiplier()
{
	return std::max(0.0f, GetAimAssistMultiplier());
}

static void ApplyVec2Scale(float* vec, float scale)
{
	if (!vec || scale == 1.0f)
		return;

	vec[0] *= scale;
	vec[1] *= scale;
}

static void AdvanceSnapshotState(float frameSeconds)
{
	s_timeSinceHipfireAttack = std::min(s_timeSinceHipfireAttack + frameSeconds, kMaxTrackedTime);
	s_snapshotBlockTimeRemaining = std::max(0.0f, s_snapshotBlockTimeRemaining - frameSeconds);
}

static void CaptureInputButtonsFromCmd(int buttons)
{
	s_latestAttackHeld = (buttons & IN_ATTACK) != 0;
	s_latestAdsInputHeld = (buttons & (IN_ATTACK2 | IN_ZOOM)) != 0;

	if (s_latestAttackHeld && !s_latestAdsInputHeld)
		s_timeSinceHipfireAttack = 0.0f;
}
struct Vector3 { float x, y, z; };

typedef __int64 (*InputBuildUserCmdType)(__int64 a1, int a2, float a3, char a4);
AUTOHOOK(InputBuildUserCmd, client.dll + 0x254D10, __int64, (__int64 a1, int a2, float a3, char a4))
{
	const __int64 result = InputBuildUserCmd(a1, a2, a3, a4);
	if (!a1)
		return result;

	const uintptr_t cmdBuffer = *reinterpret_cast<uintptr_t*>(a1 + kInputCmdBufferOffset);
	if (!cmdBuffer)
		return result;

	int cmdIndex = a2 % kCmdHistorySize;
	if (cmdIndex < 0)
		cmdIndex += kCmdHistorySize;
	const uintptr_t cmdAddr = cmdBuffer + static_cast<uintptr_t>(kUserCmdStride) * static_cast<uintptr_t>(cmdIndex);
	const int buttons = *reinterpret_cast<int*>(cmdAddr + kUserCmdButtonsOffset);
	CaptureInputButtonsFromCmd(buttons);

	return result;
}

typedef void (*AimAssistApplyType)(__int64 a1, char a2, float a3, float a4, int a5, int a6, float a7, char a8, float a9, float* a10);
AUTOHOOK(AimAssistApplyContext, client.dll + 0x99410, void, (__int64 a1, char a2, float a3, float a4, int a5, int a6, float a7, char a8, float a9, float* a10))
{
	const float frameSeconds = GetFrameSeconds(a7);
	AdvanceSnapshotState(frameSeconds);

	const bool adsActive = a2 != 0;
	if (adsActive && !s_prevAdsActive && IsAimAssistSnapshotFixEnabled())
	{
		// Snapshot abuse is pre-fire hipfire then ADS-in; require recent hipfire attack before ADS transition.
		if (s_timeSinceHipfireAttack <= kSnapshotPrefireWindowSeconds && s_latestAttackHeld && s_latestAdsInputHeld)
			s_snapshotBlockTimeRemaining = std::max(s_snapshotBlockTimeRemaining, kSnapshotBlockSeconds);
	}

	s_prevAdsActive = adsActive;
	AimAssistApplyContext(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}

typedef double (*AimAssistTimeRampType)(__int64 a1, __int64 a2, int a3);
AUTOHOOK(AimAssistTimeRamp, client.dll + 0x9A120, double, (__int64 a1, __int64 a2, int a3))
{
	const double base = AimAssistTimeRamp(a1, a2, a3);
	const float scaled = std::clamp(static_cast<float>(base) * GetGlobalAimAssistMultiplier(), 0.0f, 1.0f);
	return static_cast<double>(scaled);
}

AUTOHOOK(AimAssistDo, client.dll + 0x9FCE0, bool, (__int64 player, char a2, char* a3, float a4))
{
	if (sv_soup_aimassist_multiplier && sv_soup_aimassist_multiplier->GetFloat() <= 0.0f)
		return false;

	return AimAssistDo(player, a2, a3, a4);
}

typedef void (*AimAssistStickPullType)(
	__int64 a1, __int64 a2, float* a3, char a4, float a5, float a6, float a7, float a8, float* a9, float* a10, float* a11);
AUTOHOOK(AimAssistStickPull, client.dll + 0xA0B80, void, (__int64 a1, __int64 a2, float* a3, char a4, float a5, float a6, float a7, float a8, float* a9, float* a10, float* a11))
{
	AimAssistStickPull(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
	const float multiplier = GetGlobalAimAssistMultiplier();
	if (a10)
		*a10 *= multiplier;
	if (a11)
		*a11 *= multiplier;
}

typedef void (*AimAssistAdditiveTermType)(__int64 a1, void* a2, float* a3);
AUTOHOOK(AimAssistAdditiveTerm, client.dll + 0x9AED0, void, (__int64 a1, void* a2, float* a3))
{
	AimAssistAdditiveTerm(a1, a2, a3);
	ApplyVec2Scale(a3, GetAimAssistScale(0.0f, true));
}

typedef float* (*AimAssistSnapType)(
	__int64 a1, void* a2, float* a3, float* a4, __int64 a5, __int64 a6, float a7, __int64 a8, float* a9);
AUTOHOOK(AimAssistSnapshotTerm, client.dll + 0x9A2B0, float*, (__int64 a1, void* a2, float* a3, float* a4, __int64 a5, __int64 a6, float a7, __int64 a8, float* a9))
{
	float* result = AimAssistSnapshotTerm(a1, a2, a3, a4, a5, a6, a7, a8, a9);
	if (!a9)
		return result;

	if (IsAimAssistSnapshotFixEnabled() && s_snapshotBlockTimeRemaining > 0.0f)
	{
		a9[0] = 0.0f;
		a9[1] = 0.0f;
		return result;
	}

	ApplyVec2Scale(a9, GetAimAssistScale(a7, true));
	return result;
}

ON_DLL_LOAD("client.dll", AimAssistHooks, (CModule module))
{
	AUTOHOOK_DISPATCH();
}

ON_DLL_LOAD_RELIESON("engine.dll", AimAssistConVars, ConVar, (CModule module))
{
	sv_soup_aimassist_multiplier = RegisterConVar(
		"sv_soup_aimassist_multiplier",
		"1.0",
		FCVAR_REPLICATED,
		"Global multiplier for controller aim assist strength.");

	sv_soup_aimassist_snapshot_fix = RegisterConVar(
		"sv_soup_aimassist_snapshot_fix",
		"0",
		FCVAR_REPLICATED,
		"Suppress snapshot snap-term for fire-before-ADS transitions.");

	sv_soup_aimassist_fps_independence_fix = RegisterConVar(
		"sv_soup_aimassist_fps_independence_fix",
		"0",
		FCVAR_REPLICATED,
		"Normalize unscaled aim-assist additive terms by frame time.");

}

ConVar* GetAimAssistMultiplierConVar()
{
	return sv_soup_aimassist_multiplier;
}

ConVar* GetAimAssistSnapshotFixConVar()
{
	return sv_soup_aimassist_snapshot_fix;
}

ConVar* GetAimAssistFpsIndependenceFixConVar()
{
	return sv_soup_aimassist_fps_independence_fix;
}

float GetAimAssistMultiplier()
{
	return sv_soup_aimassist_multiplier ? sv_soup_aimassist_multiplier->GetFloat() : 1.0f;
}

bool IsAimAssistSnapshotFixEnabled()
{
	return sv_soup_aimassist_snapshot_fix ? sv_soup_aimassist_snapshot_fix->GetBool() : true;
}

bool IsAimAssistFpsIndependenceFixEnabled()
{
	return sv_soup_aimassist_fps_independence_fix ? sv_soup_aimassist_fps_independence_fix->GetBool() : true;
}


void ResetAimAssistConVarRefs()
{
	sv_soup_aimassist_multiplier = nullptr;
	sv_soup_aimassist_snapshot_fix = nullptr;
	sv_soup_aimassist_fps_independence_fix = nullptr;
	s_prevAdsActive = false;
	s_latestAttackHeld = false;
	s_latestAdsInputHeld = false;
	s_timeSinceHipfireAttack = kMaxTrackedTime;
	s_snapshotBlockTimeRemaining = 0.0f;
}
