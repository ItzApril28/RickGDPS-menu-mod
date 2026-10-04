#include "EqPresetStore.hpp"

#include "MusicPlayerManager.hpp"

#include <Geode/utils/file.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <algorithm>
#include <cctype>
#include <utility>

using namespace geode::prelude;

namespace rickgdps::music::eqpresets {

    namespace {
        constexpr char const* kFileName = "eq_presets.json";
        constexpr std::size_t kBandCount = 10;

        std::vector<Preset> s_presets;
        bool s_loaded = false;

        std::vector<Preset> builtinPresets() {
            return {
                {"Flat",          {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}, true},
                {"Bass Boost",    {5.5f, 6.0f, 4.0f, 2.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.5f, 2.0f}, true},
                {"Vocal Boost",   {-2.0f, -1.0f, 0.0f, 2.0f, 4.5f, 5.0f, 4.0f, 2.0f, 1.0f, 0.0f}, true},
                {"Treble Boost",  {0.0f, 0.0f, 0.5f, 1.0f, 1.5f, 3.0f, 4.5f, 5.5f, 6.0f, 6.0f}, true},
                {"EDM / V-Shape", {5.0f, 4.5f, 2.0f, -1.0f, -2.5f, -2.0f, 1.5f, 3.5f, 5.0f, 5.5f}, true},
            };
        }

        std::string sanitizeName(std::string name) {
            // Keep printable characters only, trim outer whitespace and cap the
            // length so preset buttons stay readable.
            std::string out;
            out.reserve(name.size());
            for (char c : name) {
                if (c == ' ' || std::isprint(static_cast<unsigned char>(c))) out.push_back(c);
            }
            auto notSpace = [](unsigned char c) { return !std::isspace(c); };
            out.erase(out.begin(), std::find_if(out.begin(), out.end(), notSpace));
            out.erase(std::find_if(out.rbegin(), out.rend(), notSpace).base(), out.end());
            if (out.size() > 40) out.resize(40);
            return out;
        }

        void loadFromDisk() {
            s_presets = builtinPresets();

            auto const path = getFilePath();
            std::error_code ec;
            if (!std::filesystem::exists(path, ec)) {
                s_loaded = true;
                return;
            }

            auto contents = file::readString(path);
            if (!contents) {
                log::warn("[RickGdps] Could not read EQ presets: {}", contents.unwrapErr());
                s_loaded = true;
                return;
            }

            auto parsed = matjson::parse(contents.unwrap());
            if (!parsed) {
                log::warn("[RickGdps] eq_presets.json is not valid JSON - ignoring custom presets");
                s_loaded = true;
                return;
            }

            auto root = parsed.unwrap();
            auto entries = root["presets"].asArray();
            if (!entries) {
                s_loaded = true;
                return;
            }

            for (auto const& entry : entries.unwrap()) {
                auto nameResult = entry["name"].asString();
                if (!nameResult) continue;
                std::string name = sanitizeName(nameResult.unwrap());
                if (name.empty()) continue;

                Preset preset;
                preset.name = std::move(name);
                preset.builtin = false;

                if (auto bands = entry["bands"].asArray()) {
                    auto const& values = bands.unwrap();
                    for (std::size_t i = 0; i < kBandCount && i < values.size(); ++i) {
                        preset.bands[i] = std::clamp(
                            static_cast<float>(values[i].asDouble().unwrapOr(0.0)), -9.f, 9.f
                        );
                    }
                }

                auto existing = std::find_if(s_presets.begin(), s_presets.end(),
                    [&](Preset const& p) { return !p.builtin && p.name == preset.name; });
                if (existing != s_presets.end()) {
                    *existing = std::move(preset);
                } else {
                    s_presets.push_back(std::move(preset));
                }
            }

            s_loaded = true;
        }

        void writeToDisk() {
            matjson::Value root = matjson::Value::object();
            matjson::Value entries = matjson::Value::array();

            for (auto const& preset : s_presets) {
                if (preset.builtin) continue;
                matjson::Value entry = matjson::Value::object();
                entry["name"] = matjson::Value(preset.name);
                matjson::Value bands = matjson::Value::array();
                for (float gain : preset.bands) {
                    bands.push(matjson::Value(static_cast<double>(gain)));
                }
                entry["bands"] = bands;
                entries.push(entry);
            }

            root["presets"] = entries;

            auto result = file::writeString(getFilePath(), root.dump(2));
            if (!result) {
                log::warn("[RickGdps] Could not write EQ presets: {}", result.unwrapErr());
            }
        }

        void ensureLoaded() {
            if (!s_loaded) loadFromDisk();
        }
    }

    std::filesystem::path getFilePath() {
        return dirs::getModPersistentDir() / kFileName;
    }

    std::vector<Preset> const& getPresets() {
        ensureLoaded();
        return s_presets;
    }

    void reload() {
        s_presets.clear();
        s_loaded = false;
        loadFromDisk();
    }

    bool applyPreset(std::size_t index) {
        ensureLoaded();
        if (index >= s_presets.size()) return false;
        auto& mgr = MusicPlayerManager::get();
        for (std::size_t i = 0; i < kBandCount; ++i) {
            mgr.setEqBand(static_cast<int>(i), s_presets[index].bands[i]);
        }
        return true;
    }

    bool saveCurrentAs(std::string const& name) {
        ensureLoaded();
        std::string clean = sanitizeName(name);
        if (clean.empty()) return false;

        Preset preset;
        preset.name = clean;
        preset.builtin = false;
        auto& mgr = MusicPlayerManager::get();
        for (std::size_t i = 0; i < kBandCount; ++i) {
            preset.bands[i] = mgr.getEqBand(static_cast<int>(i));
        }

        auto existing = std::find_if(s_presets.begin(), s_presets.end(),
            [&](Preset const& p) { return !p.builtin && p.name == preset.name; });
        if (existing != s_presets.end()) {
            *existing = std::move(preset);
        } else {
            s_presets.push_back(std::move(preset));
        }

        writeToDisk();
        return true;
    }

    namespace {
        class SaveEqPresetPopup : public geode::Popup {
        public:
            static SaveEqPresetPopup* create(std::function<void()> onSaved) {
                auto* ret = new SaveEqPresetPopup();
                if (ret && ret->initWith(std::move(onSaved))) {
                    ret->autorelease();
                    return ret;
                }
                CC_SAFE_DELETE(ret);
                return nullptr;
            }

        private:
            geode::TextInput* m_input = nullptr;
            std::function<void()> m_onSaved;

            bool initWith(std::function<void()> onSaved) {
                if (!Popup::init(250.f, 120.f)) return false;
                m_onSaved = std::move(onSaved);

                setTitle("SAVE EQ PRESET", "goldFont.fnt", 0.4f, 14.f);

                m_input = geode::TextInput::create(200.f, "Preset name");
                if (m_input) {
                    m_input->setCommonFilter(geode::CommonFilter::Any);
                    m_input->setMaxCharCount(40);
                    m_input->setPosition({125.f, 64.f});
                    m_mainLayer->addChild(m_input);
                }

                auto* saveSpr = ButtonSprite::create("SAVE", "bigFont.fnt", "GJ_button_01.png", 0.7f);
                auto* saveBtn = CCMenuItemExt::createSpriteExtra(saveSpr, [this](CCObject*) {
                    this->onSaveClicked();
                });
                saveBtn->setPosition({70.f, 24.f});
                m_buttonMenu->addChild(saveBtn);

                auto* cancelSpr = ButtonSprite::create("CANCEL", "bigFont.fnt", "GJ_button_02.png", 0.7f);
                auto* cancelBtn = CCMenuItemExt::createSpriteExtra(cancelSpr, [this](CCObject*) {
                    this->onClose(nullptr);
                });
                cancelBtn->setPosition({180.f, 24.f});
                m_buttonMenu->addChild(cancelBtn);

                return true;
            }

            void onSaveClicked() {
                std::string name;
                if (m_input) name = m_input->getString();
                if (sanitizeName(name).empty()) {
                    Notification::create("Enter a preset name", NotificationIcon::Warning, 1.5f)->show();
                    return;
                }
                if (!saveCurrentAs(name)) {
                    Notification::create("Could not save preset", NotificationIcon::Error, 1.5f)->show();
                    return;
                }
                Notification::create("EQ preset saved", NotificationIcon::Success, 1.5f)->show();
                if (m_onSaved) m_onSaved();
                this->onClose(nullptr);
            }
        };
    }

    void openSavePopup(std::function<void()> onSaved) {
        auto* popup = SaveEqPresetPopup::create(std::move(onSaved));
        if (!popup) {
            Notification::create("Could not open preset popup", NotificationIcon::Error, 1.5f)->show();
            return;
        }
        popup->show();
    }
} // namespace rickgdps::music::eqpresets
