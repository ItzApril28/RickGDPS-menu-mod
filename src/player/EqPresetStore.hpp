#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace rickgdps::music::eqpresets {

    struct Preset {
        std::string name;
        std::array<float, 10> bands{};
        bool builtin = false;
    };

    // Pluggable EQ presets: built-ins are always available, custom presets are
    // loaded from (and saved to) eq_presets.json in the mod's persistent
    // folder so users can add or share their own preset files.
    std::vector<Preset> const& getPresets();
    void reload();
    std::filesystem::path getFilePath();
    bool applyPreset(std::size_t index);
    bool saveCurrentAs(std::string const& name);
    void openSavePopup(std::function<void()> onSaved);

} // namespace rickgdps::music::eqpresets
