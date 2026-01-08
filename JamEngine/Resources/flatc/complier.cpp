#include <iostream>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

int main()
{
    std::wstring buf;

    for (const auto& entry: fs::directory_iterator(fs::current_path()))
    {
        const fs::path path = entry.path();
        const fs::path ext  = path.extension();

        if (ext == L".fbs")
        {
            fs::path filename = path.filename();
            std::cout << "Found FlatBuffers schema file: " << filename << std::endl;
            buf = L"flatc --cpp ";
            buf += filename;
            int ret = _wsystem(buf.c_str());
            if (ret != 0)
            {
                std::cout << "error: " << filename << " compilation failed!" << std::endl;
                // will be print error by flatc itself
            }
            else
            {
                std::cout << filename << " compiled successfully." << std::endl;
            }
        }
    }

    std::cout << "All done! Press any key to exit." << std::endl;
    std::cin.get();
    return 0;
}