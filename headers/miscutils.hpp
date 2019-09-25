#pragma once

#include <cstring>
#include <filesystem>
#include <gsl/gsl>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

inline size_t GenerateHashFromString( std::string_view strView )
{
    return std::hash<std::string_view>{}( strView );
}

inline size_t GenerateHashFromString( const std::string& str )
{
    return std::hash<std::string>{}( str );
}

template <typename DataType, typename DataSizeType,
          typename NoPtrDataType = typename std::remove_pointer<DataType>::type>
inline gsl::span<NoPtrDataType> PairToSpan(
    std::pair<DataType, DataSizeType> src )
{
    return { src.first, src.second };
}

inline std::string PathStrFromIterators( fs::path::const_iterator begin,
                                         fs::path::const_iterator last )
{
    constexpr const std::size_t PKG_FILENAME_MAX_LEN = 261;

    std::string outPathStr;

    // this is the max path in the pkg's headers
    outPathStr.reserve( PKG_FILENAME_MAX_LEN );

    auto end = last;
    end++;

    for ( auto cur = begin; cur != end; cur++ )
    {
        outPathStr += '/';
        outPathStr += cur->generic_string();
    }

    return outPathStr;
}

inline std::string CopyViewToNewStr( std::string_view targetView )
{
    return { targetView.data(), targetView.length() };
}
