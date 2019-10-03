#include "fsutils.hpp"

#include <fstream>
#include <iostream>

#include <gsl/gsl>

std::pair<bool, std::vector<uint8_t>> ReadFileToBuffer(
    const fs::path& filePath, uint64_t readLength /*= 0*/ )
{
    if ( fs::is_regular_file( filePath ) == false )
    {
        return { false, {} };
    }

    std::ifstream is( filePath, std::ios::binary | std::ios::ate );

    if ( is.is_open() == false )
    {
        return { false, {} };
    }

    const uint64_t iFileSize = gsl::narrow_cast<uint64_t>( is.tellg() );

    if ( readLength == 0 )
    {
        readLength = iFileSize;
    }

    if ( readLength > iFileSize )
    {
        return { false, {} };
    }

    std::vector<uint8_t> res( readLength );

    is.seekg( std::ios::beg );
    is.read( reinterpret_cast<char*>( res.data() ), readLength );

    return { true, std::move( res ) };
}

bool WriteBufferToFile( const fs::path& filePath, gsl::span<uint8_t> buff )
{
    std::ofstream os( filePath, std::ios::binary );

    if ( os.is_open() == false )
    {
        return false;
    }

    os.write( reinterpret_cast<char*>( buff.data() ), buff.size_bytes() );

    return true;
}

bool CreateDirIfUnexisting( const fs::path& newDirPath ) noexcept
{
    std::error_code errorCode;

    if ( fs::is_directory( newDirPath, errorCode ) == true )
    {
        if ( errorCode.value() == 0 )
        {
            return true;
        }
    }

    if ( fs::create_directories( newDirPath, errorCode ) == true )
    {
        if ( errorCode.value() == 0 )
        {
            return true;
        }
    }

    return false;
}
