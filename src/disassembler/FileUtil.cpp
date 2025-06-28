#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <stdexcept>

#include <totr/disassembler/FileUtil.hpp>

std::unordered_map<std::uint32_t, totr::Disassembler::ArmMode> totr::Disassembler::load_overrides(const std::string& filepath) {
    std::unordered_map<std::uint32_t, ArmMode> overrides;

    std::ifstream file_stream(filepath);
    if (!file_stream) return overrides;

    std::string line;
    while (std::getline(file_stream, line)) {
        if (auto comment_pos = line.find('#'); comment_pos != std::string::npos)
            line.erase(comment_pos);

        // trim leading / trailing whitespace
        auto is_not_space = [](unsigned char ch) { return !std::isspace(ch); };
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), is_not_space));

        line.erase(std::find_if(line.rbegin(), line.rend(), is_not_space).base(), line.end());
        if (line.empty()) continue;

        // split into address token and mode token
        std::istringstream token_stream(line);
        std::string addr_token, mode_token;
        if (!(token_stream >> addr_token >> mode_token)) continue; // malformed line

        std::uint32_t pc_address = static_cast<std::uint32_t>(std::stoul(addr_token, nullptr, 0));

        // normalise mode string to lower-case
        std::transform(mode_token.begin(), mode_token.end(), mode_token.begin(),
            [](unsigned char letter) { return std::tolower(letter); });

        ArmMode decoded_mode = (mode_token == "thumb") ? ArmMode::THUMB : ArmMode::ARM;

        overrides[pc_address] = decoded_mode;
    }
    return overrides;
}

std::vector<uint8_t> totr::Disassembler::load_rom(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Cannot open ROM file: " + path);

    std::streamsize size = file.tellg();
    if (size <= 0) throw std::runtime_error("Empty or unreadable file.");

    std::vector<uint8_t> rom(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(rom.data()), size))
        throw std::runtime_error("Failed while reading ROM.");

    return rom;
}
