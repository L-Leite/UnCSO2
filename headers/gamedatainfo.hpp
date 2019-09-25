#pragma once

#include <filesystem>
#include <string_view>

namespace fs = std::filesystem;
using namespace std::string_view_literals;

enum class GameProvider
{
    Unknown = -1,
    Nexon = 0,
    Tiancity,
    Beancity,
    NexonJP,
    Tfo
};

inline static GameProvider GetGameProviderByIndex( const int index ) noexcept
{
    switch ( index )
    {
        case 0:
            return GameProvider::Nexon;
        case 1:
            return GameProvider::Tiancity;
        case 2:
            return GameProvider::Beancity;
        case 3:
            return GameProvider::NexonJP;
        case 4:
            return GameProvider::Tfo;
        default:
            return GameProvider::Unknown;
    };
}

class GameDataInfo
{
public:
    GameDataInfo();
    GameDataInfo( fs::path gameDataPath );
    ~GameDataInfo() = default;

private:
    GameDataInfo( fs::path gameDataPath, GameProvider provider );

public:
    static GameDataInfo CreateWithProvider( fs::path gameDataPath,
                                            GameProvider provider );

    inline GameProvider GetGameProvider() const;
    inline bool WasGameDetected() const;

    std::string_view GetPrintableProviderName() const;
    std::string_view GetPrintableGameName() const;

    inline const fs::path& GetGameDataPath() const;
    inline void SetGameDataPath( fs::path gameDataPath );

    void Reset();

private:
    void DetectGameProvider();

private:
    fs::path m_GameDataPath;
    GameProvider m_GameProvider;
};

inline GameProvider GameDataInfo::GetGameProvider() const
{
    return this->m_GameProvider;
}

inline bool GameDataInfo::WasGameDetected() const
{
    return this->m_GameProvider != GameProvider::Unknown;
}

inline const fs::path& GameDataInfo::GetGameDataPath() const
{
    return this->m_GameDataPath;
}

inline void GameDataInfo::SetGameDataPath( fs::path gameDataPath )
{
    this->m_GameDataPath = gameDataPath;
    this->DetectGameProvider();
}
