﻿#pragma once
#include <string>

struct FVector;
struct FRotator;

struct FImGuiWidget
{
    static void DrawVec3Control(const std::string& label, FVector& values, float resetValue = 0.0f, float columnWidth = 100.0f);

    static void DrawRot3Control(const std::string& label, FRotator& values, float resetValue = 0.0f, float columnWidth = 100.0f);
};
