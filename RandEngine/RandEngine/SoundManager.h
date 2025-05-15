#include <fmod.hpp>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <string>

class FSoundManager {
public:
    static FSoundManager& GetInstance() {
        static FSoundManager instance;
        return instance;
    }

    bool Initialize() {
        FMOD_RESULT result = FMOD::System_Create(&system);
        if (result != FMOD_OK) {
            std::cerr << "FMOD System_Create failed!" << std::endl;
            return false;
        }

        result = system->init(512, FMOD_INIT_NORMAL, nullptr);
        if (result != FMOD_OK) {
            std::cerr << "FMOD system init failed!" << std::endl;
            return false;
        }

        return true;
    }

    void Shutdown() {
        for (auto& pair : soundMap) {
            pair.second->release();
        }
        soundMap.clear();

        if (system) {
            system->close();
            system->release();
        }
    }

    bool LoadSound(const std::string& name, const std::string& filePath, bool loop = false) {
        if (soundMap.find(name) != soundMap.end()) {
            return true; // �̹� �ε��
        }

        FMOD::Sound* sound = nullptr;
        FMOD_MODE mode = loop ? FMOD_LOOP_NORMAL : FMOD_DEFAULT;
        if (system->createSound(filePath.c_str(), mode, nullptr, &sound) != FMOD_OK) {
            std::cerr << "Failed to load sound: " << filePath << std::endl;
            return false;
        }
        soundMap[name] = sound;
        return true;
    }

    void PlaySound(const std::string& name, bool bStopPreviousIdentical = false) {
        auto it = soundMap.find(name);
        if (it != soundMap.end()) {
            FMOD::Sound* soundToPlay = it->second;

            if (bStopPreviousIdentical) {
                for (auto chanIt = activeChannels.begin(); chanIt != activeChannels.end(); /* 조건부 증가 */) {
                    FMOD::Channel* activeChannel = *chanIt;
                    FMOD::Sound* currentSoundInChannel = nullptr;
                    bool erased = false; // 제거되었는지 플래그

                    if (activeChannel) { // 채널 유효성 검사
                        FMOD_RESULT result = activeChannel->getCurrentSound(&currentSoundInChannel);
                        if (result == FMOD_OK && currentSoundInChannel == soundToPlay) {
                            activeChannel->stop();
                            chanIt = activeChannels.erase(chanIt); // 제거하고 다음 유효한 반복자 받음
                            erased = true;
                            // 여기서 continue; 해도 되지만, erased 플래그로 아래에서 ++chanIt 방지
                        }
                    }
                    else { // null 채널이라면 (이론상 없어야 하지만 방어 코드)
                        chanIt = activeChannels.erase(chanIt);
                        erased = true;
                    }

                    if (!erased) { // 제거되지 않았다면 다음 요소로 이동
                        ++chanIt;
                    }
                }
            }

            FMOD::Channel* newChannel = nullptr;
            system->playSound(soundToPlay, nullptr, false, &newChannel);
            if (newChannel) {
                activeChannels.push_back(newChannel);
            }
        }
    }

    void Update() {
        system->update();

        activeChannels.erase(
            std::remove_if(activeChannels.begin(), activeChannels.end(),
                [](FMOD::Channel* channel) {
                    bool isPlaying = false;
                    if (channel) {
                        channel->isPlaying(&isPlaying);
                    }
                    return !isPlaying; // ����� ���� ä�� ����
                }),
            activeChannels.end()
        );
    }

    void StopAllSounds()
    {
        // 1) 개별 채널을 멈추기
        for (FMOD::Channel* channel : activeChannels)
        {
            if (channel)
            {
                channel->stop();
            }
        }
        activeChannels.clear();

        // 2) 시스템 전체(마스터 채널 그룹) 정지 (Optional)
        FMOD::ChannelGroup* masterGroup = nullptr;
        if (system->getMasterChannelGroup(&masterGroup) == FMOD_OK && masterGroup)
        {
            masterGroup->stop();
        }
    }
    const std::vector<std::string>& GetLoadedSoundNames() const
    {
        if (FSoundManager::GetInstance().bSoundNamesCacheDirty)
        {
            FSoundManager::GetInstance().RebuildLoadedSoundNamesCache();
        }
        return loadedSoundNamesCache;
    }
private:
    FSoundManager() : system(nullptr) {}
    ~FSoundManager() { Shutdown(); }
    FSoundManager(const FSoundManager&) = delete;
    FSoundManager& operator=(const FSoundManager&) = delete;
    void RebuildLoadedSoundNamesCache()
    {
        loadedSoundNamesCache.clear();
        for (const auto& pair : soundMap)
        {
            loadedSoundNamesCache.push_back(pair.first);
        }
        std::sort(loadedSoundNamesCache.begin(), loadedSoundNamesCache.end()); // 정렬
        bSoundNamesCacheDirty = false;

    }
    FMOD::System* system;
    std::unordered_map<std::string, FMOD::Sound*> soundMap;
    std::vector<FMOD::Channel*> activeChannels;
    std::vector<std::string> loadedSoundNamesCache;
    bool bSoundNamesCacheDirty = true;
};
