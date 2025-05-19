#pragma once
#include "Serialization/Archive.h"
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

    friend FArchive& operator<<(FArchive& Ar, FDistributionFloat& Dist)
    {
        // 1. DistributionType (enum) 직렬화
        // enum을 uint8로 저장하고 로드 시 다시 enum으로 캐스팅
        if (Ar.IsLoading())
        {
            uint8 DistTypeInt;
            Ar << DistTypeInt;
            Dist.DistributionType = static_cast<EDistributionType>(DistTypeInt);
        }
        else // Saving
        {
            uint8 DistTypeInt = static_cast<uint8>(Dist.DistributionType);
            Ar << DistTypeInt;
        }

        // 2. 분포 타입에 따라 필요한 데이터 직렬화
        switch (Dist.DistributionType)
        {
        case EDistributionType::Constant:
            Ar << Dist.Constant;
            break;

        case EDistributionType::Uniform:
            Ar << Dist.Min;
            Ar << Dist.Max;
            break;

        case EDistributionType::Curve:

            Ar << Dist.Constant;
            break;

        case EDistributionType::Parameter:
            Ar << Dist.ParameterName; // FName 직렬화 (FArchive에 operator<<(FName&) 오버로드 필요)
            // Parameter 타입일 경우에도 Constant 값을 fallback으로 저장/로드할 수 있습니다.
            Ar << Dist.Constant;
            // UE_LOG(LogSerialization, Verbose, TEXT("Serializing FDistributionFloat: Parameter type (Name: %s, Constant fallback: %f)."), *Dist.ParameterName.ToString(), Dist.Constant);
            break;

        default:
            // 알 수 없는 분포 타입 처리 (오류 로그 또는 기본값 처리)
            // UE_LOG(LogSerialization, Warning, TEXT("Serializing FDistributionFloat: Unknown distribution type encountered."));
            // 기본적으로 Constant 값을 저장/로드할 수 있도록 처리 (안전한 fallback)
            if (Ar.IsLoading()) { // 로딩 시에는 Constant 필드가 이미 초기화되어 있을 수 있으므로, 읽어오는 것이 안전
                Ar << Dist.Constant;
            }
            else if (Dist.DistributionType != EDistributionType::Constant) { // 저장 시, 명시적으로 Constant가 아니었다면, Constant 필드라도 저장
                Ar << Dist.Constant;
            }
            break;
        }
        // 로딩 시, 사용되지 않는 필드들은 기본값으로 남아있거나, 필요하다면 여기서 명시적으로 초기화할 수 있습니다.
        // 예를 들어, 로딩 시 타입이 Constant로 바뀌었다면 Min, Max, ParameterName 등을 초기화.
        if (Ar.IsLoading())
        {
            if (Dist.DistributionType != EDistributionType::Uniform)
            {
                // Dist.Min = 0.0f; // 필요에 따라 초기화
                // Dist.Max = 0.0f;
            }
            if (Dist.DistributionType != EDistributionType::Parameter)
            {
                // Dist.ParameterName = NAME_None; // 필요에 따라 초기화
            }
            // Curve 타입에 대한 처리도 유사하게 가능
        }


        return Ar;
    }
};
