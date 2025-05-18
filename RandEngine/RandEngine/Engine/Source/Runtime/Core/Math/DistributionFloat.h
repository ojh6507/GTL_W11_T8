#pragma once
#include "UObject/NameTypes.h"
#include "MathUtility.h"

enum class EDistributionType : uint8 // 명시적으로 기본 타입 지정 가능
{
    Constant,
    Uniform,
    Curve,
    Parameter,
};

struct FDistributionFloat
{
    EDistributionType DistributionType;

    float Constant;

    float Min;

    float Max;


    FName ParameterName; // 파라미터 이름

    // 생성자에서 기본값 설정
    FDistributionFloat()
        : DistributionType(EDistributionType::Constant)
        , Constant(0.0f)
        , Min(0.0f)
        , Max(1.0f)
        , ParameterName(NAME_None) // FName의 기본값
    {
    }

    // 특정 시간 및 컨텍스트에서 실제 float 값을 가져오는 함수
    float GetValue(float InTime = 0.0f) const
    {
        switch (DistributionType)
        {
        case EDistributionType::Constant:
            return Constant;

        case EDistributionType::Uniform:
        {
            return FMath::FRandRange(Min, Max);
        }

        case EDistributionType::Curve:
            // if (Curve)
            // {
            //     return Curve->GetFloatValue(InTime); // UCurveFloat의 함수 호출
            // }
            // UE_LOG(LogTemp, Warning, TEXT("FDistributionFloat::GetValue - Curve is null for Curve distribution."));
            return Constant; // 커브가 없으면 Constant 값 반환 (또는 0)

        case EDistributionType::Parameter:
            // if (ParamOwner && ParameterName != NAME_None)
            // {
            //     // ParamOwner에서 ParameterName을 가진 float 프로퍼티를 찾아서 반환하는 로직 필요
            //     // (리플렉션 또는 특정 인터페이스 기반)
            //     // 예: return UGameplayStatics::GetFloatAttribute(ParamOwner, ParameterName);
            // }
            // UE_LOG(LogTemp, Warning, TEXT("FDistributionFloat::GetValue - Parameter '%s' not found or ParamOwner is null."), *ParameterName.ToString());
            return Constant; // 파라미터를 못 찾으면 Constant 값 반환 (또는 0)
        }
        return Constant; // 기본적으로 Constant 값 반환
    }

    // 분포가 상수인지 확인 (에디터 등에서 사용)
    bool IsConstant() const
    {
        return DistributionType == EDistributionType::Constant;
    }

    // 상수 값을 직접 가져오기 (IsConstant()가 true일 때만 안전)
    float GetConstantValue() const
    {
        return Constant;
    }

    // 분포에서 가능한 (또는 대표적인) 최대값을 가져오려는 시도 (추정치 계산 등에 사용)
    float GetMaxValue() const
    {
        switch (DistributionType)
        {
        case EDistributionType::Constant:
            return Constant;
        case EDistributionType::Uniform:
            return Max;
        case EDistributionType::Curve:
            // if (Curve)
            // {
            //     float MinVal, MaxVal;
            //     Curve->GetTimeRange(MinVal, MaxVal); // 커브의 시간 범위
            //     // 실제로는 커브의 값 범위를 가져와야 함
            //     // return Curve->GetValueRange(MinVal, MaxVal).Max; // 예시
            //     return Constant; // 단순화를 위해 일단 Constant 반환
            // }
            return Constant;
        case EDistributionType::Parameter:
            // 파라미터의 최대값을 알 수 없으므로, 보수적으로 Constant나 특정 기본값 반환
            return Constant;
        }
        return Constant;
    }
};
