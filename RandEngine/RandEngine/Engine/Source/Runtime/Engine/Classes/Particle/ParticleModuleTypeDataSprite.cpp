#include "ParticleModuleTypeDataSprite.h"
#include "ParticleEmitter.h"
#include "Launch/EngineLoop.h"
#include "Engine/ResourceMgr.h"

void UParticleModuleTypeDataSprite::Build(const FParticleEmitterBuildInfo& EmitterBuildInfo)
{
    Super::Build(EmitterBuildInfo); // 부모 클래스 Build 호출

    // 에디터 모드일 때만, 또는 경로가 있고 아직 캐시된 포인터가 없을 때 로드 시도
    if ((EmitterBuildInfo.bIsEditorBuild || !CachedTexture) && !TextureAssetPath.IsEmpty())
    {
        FWString TexturePath = TextureAssetPath.ToWideString(); // 경로를 WString으로 변환
        std::shared_ptr<FTexture> LoadedTextureSharedPtr = FEngineLoop::ResourceManager.GetTexture(TexturePath);

        if (LoadedTextureSharedPtr)
        {
            CachedTexture = LoadedTextureSharedPtr.get();
        }
    }
    else if (TextureAssetPath.IsEmpty()) // 경로가 비어있으면 캐시된 포인터도 무효화
    {
        CachedTexture = nullptr;
    }
}
