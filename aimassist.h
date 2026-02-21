#pragma once

class ConVar;

ConVar* GetAimAssistMultiplierConVar();
ConVar* GetAimAssistSnapshotFixConVar();
ConVar* GetAimAssistFpsIndependenceFixConVar();
ConVar* GetRequireClientModConVar();

float GetAimAssistMultiplier();
bool IsAimAssistSnapshotFixEnabled();
bool IsAimAssistFpsIndependenceFixEnabled();
bool IsClientModRequired();

void ResetAimAssistConVarRefs();
