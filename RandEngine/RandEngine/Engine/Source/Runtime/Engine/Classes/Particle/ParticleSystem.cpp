#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModuleRequired.h"

#include "Core/Serialization/MemoryArchive.h"

#include "UObject/ObjectFactory.h"
#include <fstream>

UParticleSystem::UParticleSystem()
    : Delay(0.0f)
    , bAutoActivate(true) // 기본적으로 자동 활성화
{
}

// 시스템 내 모든 이미터 빌드
void UParticleSystem::BuildEmitters()
{
    // Emitters 배열에 있는 각 UParticleEmitter에 대해 Build() 함수를 호출합니다.
    for (UParticleEmitter* Emitter : Emitters)
    {
        if (Emitter)
        {
            Emitter->Build(); // 각 이미터의 Build 함수 호출
        }
    }
}

void UParticleSystem::InitializeSystem()
{
    BuildEmitters();
    UpdateComputedFlags();
}

void UParticleSystem::UpdateComputedFlags() // 이 함수는 private 멤버로 선언하는 것이 좋음
{
    bIsLooping_Computed = false; // 기본값으로 시작

    for (const UParticleEmitter* Emitter : Emitters)
    {
        if (Emitter)
        {
            UParticleLODLevel* HighLOD = Emitter->GetHighestLODLevel(); // UParticleEmitter에 GetHighestLODLevel() 구현 필요
            if (HighLOD && HighLOD->RequiredModule)
            {
                // UParticleModuleRequired에 EmitterLoops 멤버 필요
                if (HighLOD->RequiredModule->EmitterLoops == 0) // 0이면 무한 루프로 간주
                {
                    bIsLooping_Computed = true;
                    break; // 하나라도 무한 루프면 전체 시스템도 루핑
                }
            }
        }
    }
}
void UParticleSystem::PostLoad()
{
    InitializeSystem();
}

void UParticleSystem::SaveParticleSystemToBinary()
{
    TArray<uint8> BinaryData;
    FMemoryWriter Ar(BinaryData);

    Ar << Delay;
    Ar << bAutoActivate;
    Ar << bIsLooping_Computed;
    int32 NumEmitters = Emitters.Num();
    Ar << NumEmitters;
    for (UParticleEmitter* Emitter : Emitters)
    {
        if (Emitter)
        {
            // UParticleEmitter에 Serialize(FArchive& Ar) 또는
            // friend FArchive& operator<<(FArchive& Ar, UParticleEmitter& E) 가 구현되어 있어야 함
            //Emitter->Serialize(Ar); // 직접 호출 방식
            Ar << (*Emitter);     // 연산자 오버로딩 방식 (UParticleEmitter에 해당 연산자 구현 필요)
        }
    }
    FString FilePath = "Contents/Particle/";
    FilePath += ParticleSystemFileName +".myparticle";
    const char* pathForStream = nullptr;
    std::string str = FilePath.ToAnsiString();
    pathForStream = str.c_str();

    std::ofstream outFile(pathForStream, std::ios::out | std::ios::binary);

    if (outFile.is_open())
    {
        if (!BinaryData.IsEmpty())
        {
            outFile.write(reinterpret_cast<const char*>(BinaryData.GetData()), BinaryData.Num());
        }

        if (outFile.good()) // 쓰기 작업 후 스트림 상태 확인
        {
            UE_LOG(ELogLevel::Display, "Save Success");
        }
        else
        {
            UE_LOG(ELogLevel::Error, "Error: Failed to write all data to file: %s", pathForStream);
        }
        outFile.close();
    }
    else
    {
        UE_LOG(ELogLevel::Error, "Error: Failed to open file for writing: %s", pathForStream);
    }
}


UParticleSystem* UParticleSystem::LoadParticleSystemFromBinary(const FString& FilePath, UObject* OuterForSystem)
{
    if (FilePath.IsEmpty())
    {
        std::cerr << "LoadParticleSystemFromBinary: Error - FilePath is empty." << std::endl;
        return nullptr;
    }

    std::string filePathStdStr = FilePath.ToAnsiString().c_str(); // 변환 (안전한 방법 사용)
    std::ifstream inFile(filePathStdStr, std::ios::in | std::ios::binary | std::ios::ate);

    if (!inFile.is_open())
    {
        std::cerr << "LoadParticleSystemFromBinary: Error - Failed to open file for reading: " << filePathStdStr << std::endl;
        return nullptr;
    }

    std::streamsize fileSize = inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    if (fileSize == 0)
    {
        inFile.close();
        return nullptr;
    }

    TArray<uint8> BinaryData;
    BinaryData.SetNum(fileSize);

    if (!inFile.read(reinterpret_cast<char*>(BinaryData.GetData()), fileSize))
    {
        inFile.close();
        return nullptr;
    }
    inFile.close();

    FMemoryReader Ar(BinaryData);

    // --- 데이터 포맷 버전 읽기 및 확인 부분 제거 ---
    // int32 LoadedDataVersion = 0;
    // Ar << LoadedDataVersion;
    // ... (버전 비교 로직 제거) ...

    UParticleSystem* LoadedSystem = FObjectFactory::ConstructObject<UParticleSystem>(OuterForSystem); // 예시
    if (!LoadedSystem)
    {
        std::cerr << "LoadParticleSystemFromBinary: Error - Failed to construct UParticleSystem object." << std::endl;
        return nullptr;
    }

    // UParticleSystem 데이터 역직렬화 (버전 정보 없이 바로 데이터 시작)
    Ar << LoadedSystem->Delay;
    Ar << LoadedSystem->bAutoActivate;
    Ar << LoadedSystem->bIsLooping_Computed;

    int32 NumEmitters = 0;
    Ar << NumEmitters;
    // std::cout << "Loading " << NumEmitters << " emitters." << std::endl; // 로그는 유지 가능

    LoadedSystem->Emitters.Empty();
    LoadedSystem->Emitters.Reserve(NumEmitters);

    for (int32 i = 0; i < NumEmitters; ++i)
    {
        UParticleEmitter* NewEmitter = FObjectFactory::ConstructObject<UParticleEmitter>(LoadedSystem); // 예시
        if (NewEmitter)
        {
            Ar << (*NewEmitter);
            LoadedSystem->Emitters.Add(NewEmitter);
        }
        else
        {
            std::cerr << "LoadParticleSystemFromBinary: Error - Failed to construct UParticleEmitter object at index " << i << "." << std::endl;
            // delete LoadedSystem; // 부분적 성공/실패 처리 정책에 따라
            return nullptr;
        }
    }

    // PostLoad 처리 (이전과 동일)
    for (UParticleEmitter* Emitter : LoadedSystem->Emitters)
    {
        if (Emitter)
        {
            // Emitter->PostLoad(); 또는 Emitter->Build(); 등
        }
    }
    // LoadedSystem->PostLoad(); 또는 LoadedSystem->UpdateComputedFlags();

    std::cout << "Particle system loaded successfully from: " << filePathStdStr << std::endl;
    return LoadedSystem;
}
