#pragma once

#include "CommonTypes.h"

class TextureCompressor
{
public:
    static std::string removeExtension(const std::string& filename) {
        return filename.substr(0, filename.find_last_of('.'));
    }

    static void collectFileNames(const std::filesystem::path& folderPath, std::unordered_set<std::string>& compress, std::unordered_set<std::string>& copy) 
    {
        for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                if (extension != ".png" && extension != ".ktx2")
                {
                    copy.insert(entry.path().filename().string());
                }
                else
                {
					compress.insert(removeExtension(entry.path().filename().string()));
                }
            }
        }
    }

    static void CompressTextures(std::filesystem::path sourceTexturesPath, std::filesystem::path destTexturesPath)
    {

        if (!std::filesystem::exists(sourceTexturesPath) || !std::filesystem::exists(destTexturesPath)) {
            std::cerr << "One or both directories do not exist.\n";
            return;
        }

		std::unordered_set<std::string> sourceCompress;
		std::unordered_set<std::string> sourceCopy;

		std::unordered_set<std::string> destCompress;
		std::unordered_set<std::string> destCopy;

        collectFileNames(sourceTexturesPath, sourceCompress, sourceCopy);
        collectFileNames(destTexturesPath, destCompress, destCopy);

        std::vector<std::string> filesToCopy;
        std::vector<std::string> filesToCompress;

		for (const std::string& file : sourceCopy)
        {
            if (destCopy.find(file) == destCopy.end())
            {
				filesToCopy.push_back(file);
			}
		}

        for (const std::string& file : filesToCopy)
		{
			std::filesystem::copy_file(sourceTexturesPath / file, destTexturesPath / file, std::filesystem::copy_options::overwrite_existing);
		}
        
        for (const std::string& file : sourceCompress)
        {
            if (destCompress.find(file) == destCompress.end())
            {           
                filesToCompress.push_back(file);
            }        
        }
        
        if(filesToCompress.empty()) 
        {
			std::cout << "No new files to compress." << std::endl;
			return;
		}

        std::string batch_file_name = "batch_script.nvtt";

        std::ofstream batch_file(batch_file_name);
        if (!batch_file.is_open()) {
            std::cerr << "Failed to create batch script file." << std::endl;
            return;
        }

        for (const std::string& file : filesToCompress) 
        {
            batch_file << (sourceTexturesPath / (file + ".png")).string() << " --format bc7 --output " << (destTexturesPath / (file + ".ktx2")).string() << " --no-mips\n";
        }

        batch_file.close();

        // Execute the batch script using nvtt_export
        int result = std::system("nvtt_export --batch batch_script.nvtt");
        if (result != 0) {
            std::cerr << "nvtt_export execution failed with code: " << result << std::endl;
            return;
        }

        try {
            if (std::filesystem::exists(batch_file_name)) {
                std::filesystem::remove(batch_file_name);
                std::cout << "Batch script file deleted successfully." << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to delete batch script file: " << e.what() << std::endl;
        }

        std::cout << "Batch compression completed successfully." << std::endl;
  
    }
};
