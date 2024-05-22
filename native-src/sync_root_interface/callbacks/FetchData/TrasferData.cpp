#include <iostream>
#include <filesystem>
#include <string>
#include "TransferData.h"
#include "Logger.h"

namespace fs = std::filesystem;

void TransferData::run(const std::wstring &sourcePath, const std::wstring &destinationPath)
{
    try
    {
        // Logger::getInstance().log("Copying data from " + Logger::fromWStringToString(sourcePath) + " to " + Logger::fromWStringToString(destinationPath), LogLevel::DEBUG);
        for (const auto &entry : fs::directory_iterator(sourcePath))
        {
            const auto &sourceFilePath = entry.path();
            auto destinationFilePath = destinationPath + L'\\' + sourceFilePath.filename().wstring();

            // Copiar el archivo o directorio
            if (fs::is_regular_file(sourceFilePath))
            {
                fs::copy_file(sourceFilePath, destinationFilePath, fs::copy_options::overwrite_existing);
            }
            else if (fs::is_directory(sourceFilePath))
            {
                fs::copy(sourceFilePath, destinationFilePath, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            }

            // Logger::getInstance().log(L"Copied: " + sourceFilePath.wstring() + L" to " + destinationFilePath, LogLevel::DEBUG);
            // Logger::getInstance().log("Copied: " + Logger::fromWStringToString(sourceFilePath.wstring()), LogLevel::DEBUG);
            // Logger::getInstance().log(" to " + Logger::fromWStringToString(destinationFilePath), LogLevel::DEBUG);
        }
    }
    catch (const fs::filesystem_error &e)
    {
        // Logger::getInstance().log(L"FileSystem Error: " + std::wstring(e.what(), e.what() + std::char_traits<char>::length(e.what())), LogLevel::ERROR);
        // Logger::getInstance().log("FileSystem Error: " + Logger::fromWStringToString(std::wstring(e.what(), e.what() + std::char_traits<char>::length(e.what()))), LogLevel::ERROR);
    }
    catch (const std::exception &e)
    {
        // Logger::getInstance().log(L"Error: " + std::wstring(e.what(), e.what() + std::char_traits<char>::length(e.what())), LogLevel::ERROR);
        // Logger::getInstance().log("Error: " + Logger::fromWStringToString(std::wstring(e.what(), e.what() + std::char_traits<char>::length(e.what()))), LogLevel::ERROR);
    }
}
