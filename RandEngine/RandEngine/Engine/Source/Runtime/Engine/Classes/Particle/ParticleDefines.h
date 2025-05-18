// ParticleDefines.h
#pragma once

#include "Math/Vector.h"
#include "Math/Color.h" 

// --- 파티클 기본 데이터 구조체 ---
struct FBaseParticle
{
    FVector Location;
    
    FVector Velocity;
    
    float RelativeTime; // 파티클 수명 내의 현재 시간 (0.0 ~ 1.0
    float Lifetime;     // 이 파티클의 총 수명
    
    FVector BaseVelocity; // 다른 요소(가속 등)에 의해 수정되기 전의 초기 속도
    
    float Rotation;     // 파티클의 현재 회전값 (Z축 기준, 2D 스프라이트용)
    float RotationRate; // 파티클의 회전 속도
    
    FVector Size;       // 파티클의 크기 (X, Y, Z. 스프라이트는 보통 X, Y만 사용)
    
    FColor Color;       // 파티클의 현재 색상
    // ... 기타 필요한 공통 데이터 (예: 활성 여부 플래그, 고유 ID 등)
    
    bool bActive;       // 파티클 활성 여부
    
    int32 ParticleID;   // 파티클 고유 ID (디버깅 등에 사용 가능)

    FBaseParticle()
        : RelativeTime(0.0f), Lifetime(1.0f), Rotation(0.0f), RotationRate(0.0f),
        Size(1.0f, 1.0f, 1.0f), Color(255, 255, 255, 255), bActive(false), ParticleID(0)
    {
    }
};

// --- 모듈 타입 식별용 Enum ---
enum class EModuleType
{
    Base,
    Required,
    Spawn,
    Lifetime,
    Location,
    Velocity,
    Color,
    Size,
    TypeDataBase,
    TypeDataSprite,
    TypeDataMesh,
    // ... 기타 지원할 모듈 타입 ...
};
