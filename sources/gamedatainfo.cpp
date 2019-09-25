#include "gamedatainfo.hpp"

constexpr const std::pair<std::string_view, std::string_view>
    GAME_PROVIDERS[5] = {
        { "Nexon"sv, "Counter-Strike Online 2"sv },
        { "Tiancity"sv, "Counter-Strike Online 2"sv },
        { "Beancity"sv, "Counter-Strike Online 2"sv },
        { "Nexon Japan"sv, "Counter-Strike Online 2"sv },
        { "Nexon"sv, "Titanfall Online"sv },
    };

constexpr const std::pair<std::string_view, GameProvider> PKG_FILENAMES[5] = {
    { "a9a34080ecb7db1b1defb7539eaa32a4.pkg"sv, GameProvider::Nexon },
    { "9aba81425766ded41499e447e36f6233.pkg"sv, GameProvider::Tiancity },
    { "7715a87e0e7e19becae97a3a93a0d9a2.pkg"sv, GameProvider::Beancity },
    { "451e91710e913436ae9f01eb3a0e010d.pkg"sv, GameProvider::NexonJP },
    { "0a4b4196394ecf251c532f1552ccf3b3.pkg"sv, GameProvider::Tfo },
};

GameDataInfo::GameDataInfo()
    : m_GameDataPath(), m_GameProvider( GameProvider::Unknown )
{
}

GameDataInfo::GameDataInfo( fs::path gameDataPath )
    : m_GameDataPath( gameDataPath ), m_GameProvider( GameProvider::Unknown )
{
    this->DetectGameProvider();
}

GameDataInfo::GameDataInfo( fs::path gameDataPath, GameProvider provider )
    : m_GameDataPath( gameDataPath ), m_GameProvider( provider )
{
}

GameDataInfo GameDataInfo::CreateWithProvider( fs::path gameDataPath,
                                               GameProvider provider )
{
    return GameDataInfo( gameDataPath, provider );
}

std::string_view GameDataInfo::GetPrintableProviderName() const
{
    switch ( this->m_GameProvider )
    {
        case GameProvider::Nexon:
            return GAME_PROVIDERS[0].first;
        case GameProvider::Tiancity:
            return GAME_PROVIDERS[1].first;
        case GameProvider::Beancity:
            return GAME_PROVIDERS[2].first;
        case GameProvider::NexonJP:
            return GAME_PROVIDERS[3].first;
        case GameProvider::Tfo:
            return GAME_PROVIDERS[4].first;
        default:
            return {};
    }
}

std::string_view GameDataInfo::GetPrintableGameName() const
{
    switch ( this->m_GameProvider )
    {
        case GameProvider::Nexon:
            return GAME_PROVIDERS[0].second;
        case GameProvider::Tiancity:
            return GAME_PROVIDERS[1].second;
        case GameProvider::Beancity:
            return GAME_PROVIDERS[2].second;
        case GameProvider::NexonJP:
            return GAME_PROVIDERS[3].second;
        case GameProvider::Tfo:
            return GAME_PROVIDERS[4].second;
        default:
            return {};
    }
}

void GameDataInfo::Reset()
{
    this->m_GameDataPath.clear();
    this->m_GameProvider = GameProvider::Unknown;
}

void GameDataInfo::DetectGameProvider()
{
    for ( auto&& filename : PKG_FILENAMES )
    {
        fs::path pkgPath = this->m_GameDataPath / filename.first;

        if ( fs::is_regular_file( pkgPath ) == true )
        {
            this->m_GameProvider = filename.second;
            return;
        }
    }

    this->m_GameProvider = GameProvider::Unknown;
}
