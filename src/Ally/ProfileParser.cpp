#include <filesystem>
#include <fstream>
#include <iterator>
#include "Ally/ProfileParser.hpp"
#include "App/Constants.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/nlohmann/json.hpp"
#include "Tools/nlohmann/adl_serializer_wstring.hpp"

namespace Ally::ProfileParser
{
    enum class MappingType
    {
        Unknown = -1,
        None = 0,
        Gamepad = 1,
        Keyboard = 2,
        Mouse = 3,
        Shortcut = 4,
        Media = 5,
        KeyboardPreset = 11,
        KeyboardPresetAlt = 12,
        GamepadStick = 13,
        ModeModifier = 14,
        MouseCursor = 15,
        KeyboardDirections = 16,
        MouseDirections = 17,
        MediaDirections = 18,
        CustomShortcut = 19,
        FirstPresetShortcut = 32,
        LastPresetShortcut = 56
    };

    namespace c = AnyFSE::App::Constants;
    static auto log = AnyFSE::Logging::LogManager::GetLogger("ProfileParser");

    static nlohmann::json LoadProfileFile(const std::filesystem::path& root, const wchar_t* directory, const std::wstring& id)
    {
        const std::filesystem::path name(id);
        if (name.empty() || name.has_parent_path() || id.find_first_of(L"/\\:") != std::wstring::npos)
            throw std::runtime_error("Invalid profile file identifier");
        std::ifstream file(root / directory / (id + L".json"));
        return nlohmann::json::parse(file);
    }

    static Hid::ButtonAction LoadButtonAction(const nlohmann::json& data, const std::filesystem::path& root, bool modeM1)
    {
        Hid::ButtonAction action {};
        MappingType type = data.at("Type").get<MappingType>();
        const BYTE argument = data.value("Arg", BYTE(0));
        if (type >= MappingType::FirstPresetShortcut && type <= MappingType::LastPresetShortcut)
            type = MappingType::Shortcut;
        switch (type)
        {
        case MappingType::None:
            break;
        case MappingType::Gamepad:
            action.type = Hid::MCUMappingType::Gamepad;
            switch (static_cast<Hid::GamepadButtonCode>(argument))
            {
            case Hid::GamepadButtonCode::LegacyView:
                action.gamepadButton = static_cast<BYTE>(Hid::GamepadButtonCode::View);
                break;
            case Hid::GamepadButtonCode::LegacyMenu:
                action.gamepadButton = static_cast<BYTE>(Hid::GamepadButtonCode::Menu);
                break;
            default:
                action.gamepadButton = argument;
                break;
            }
            break;
        case MappingType::Keyboard:
            action.type = Hid::MCUMappingType::Keyboard;
            action.keyboardKey = argument;
            break;
        case MappingType::Mouse:
            action.type = Hid::MCUMappingType::Mouse;
            action.mouseAction = static_cast<Hid::MappingActions>(argument);
            break;
        case MappingType::Media:
            action.type = Hid::MCUMappingType::Media;
            action.mediaAction = argument;
            break;
        case MappingType::ModeModifier:
            action.type = Hid::MCUMappingType::Keyboard;
            action.keyboardKey = static_cast<BYTE>(modeM1 ? Hid::KeyboardCode::ModeM1 : Hid::KeyboardCode::ModeM2);
            break;
        case MappingType::Shortcut:
            if (argument < 2 || argument > sizeof(action.shortcutKeys))
                throw std::runtime_error("Unsupported shortcut length");
            action.type = Hid::MCUMappingType::Shortcut;
            action.shortcutLength = argument;
            for (size_t key = 0; key < action.shortcutLength; ++key)
                action.shortcutKeys[key] = data.at("Args").at(key).get<BYTE>();
            break;
        case MappingType::CustomShortcut:
        {
            const nlohmann::json shortcut = LoadProfileFile(root, c::AllyCombKeys, data.at("Cid").get<std::wstring>());
            action.type = Hid::MCUMappingType::Shortcut;
            for (const nlohmann::json& entry : shortcut.at("Entries"))
            {
                if (entry.at("Type").get<MappingType>() != MappingType::Keyboard)
                    continue;
                action.shortcutKeys[action.shortcutLength++] = entry.at("Arg").get<BYTE>();
                if (action.shortcutLength == sizeof(action.shortcutKeys))
                    break;
            }
            if (!action.shortcutLength)
                throw std::runtime_error("Custom shortcut contains no keyboard actions");
            break;
        }
        default:
            throw std::runtime_error("Unsupported profile button binding");
        }
        return action;
    }

    static Hid::ButtonBinding LoadButtonBinding(const nlohmann::json& buttons, const char* name, const std::filesystem::path& root)
    {
        const nlohmann::json& button = buttons.at(name);
        const bool modeM1 = std::strcmp(name, "M1") == 0;
        Hid::ButtonBinding binding {};
        if (button.value("is_toggle", false))
        {
            binding.primary.type = Hid::MCUMappingType::Keyboard;
            binding.primary.keyboardKey = static_cast<BYTE>(modeM1 ? Hid::KeyboardCode::ModeM1 : Hid::KeyboardCode::ModeM2);
            binding.secondary = binding.primary;
        }
        else
        {
            binding.primary = LoadButtonAction(button.at("typ_set"), root, modeM1);
            binding.secondary = LoadButtonAction(button.at("tog_set"), root, modeM1);
        }
        return binding;
    }

    bool Load(Profile::Mode mode, Hid::Mapping& mapping)
    {
        namespace fs = std::filesystem;
        using jp = nlohmann::json::json_pointer;

        wchar_t root[MAX_PATH] = {0};
        ExpandEnvironmentStrings(c::AllyProfileRootPath, root, MAX_PATH);

        const wchar_t* profilePath = nullptr;
        const wchar_t* templateId = nullptr;
        switch (mode)
        {
        case Profile::Mode::Desktop:
            profilePath = c::AllyDesktopProfilePath;
            templateId = c::AllyDesktopTemplateId;
            break;
        case Profile::Mode::Gamepad:
            profilePath = c::AllyGamepadProfilePath;
            templateId = c::AllyGamepadTemplateId;
            break;
        default:
            log.Warn("Auto has no standalone controller profile");
            return false;
        }

        try
        {
            std::ifstream file(fs::path(root) / profilePath);
            nlohmann::json profile = file ? nlohmann::json::parse(file) : LoadProfileFile(root, c::AllySystemTemplates, templateId);
            if (!profile.value(jp("/IsEdited"), false))
            {
                const std::wstring id = profile.value(jp("/TemplateId"), std::wstring());
                const bool system = id == c::AllyDesktopTemplateId || id == c::AllyGamepadTemplateId;
                profile = LoadProfileFile(root, system ? c::AllySystemTemplates : c::AllyTemplates, id);
            }

            Hid::Mapping result {};
            const char* buttons[][2] = {
                {"DU", "DD"}, {"DL", "DR"}, {"J0B", "J1B"}, {"LB", "RB"}, {"A", "B"},
                {"X", "Y"}, {"View", "Menu"}, {"M2", "M1"}, {"LT", "RT"}
            };
            for (size_t block = 0; block < std::size(buttons); ++block)
            {
                result.buttons.blocks[block].first = LoadButtonBinding(profile.at("Button"), buttons[block][0], root);
                result.buttons.blocks[block].second = LoadButtonBinding(profile.at("Button"), buttons[block][1], root);
            }

            for (size_t s = 0; s < 2; ++s)
            {
                const std::string side = s ? "/Button/J1" : "/Button/J0";
                const MappingType type = profile.value(jp(side + "/typ_set/Type"), MappingType::Unknown);

                Hid::StickMapping& stick = s ? result.rightStick : result.leftStick;
                BYTE* codes = nullptr;
                switch (type)
                {
                case MappingType::MouseCursor:
                    stick.type = Hid::MCUMappingType::Mouse;
                    stick.mouseAction = Hid::MappingActions::MouseCursor;
                    continue;

                case MappingType::Gamepad:
                case MappingType::GamepadStick:
                    stick.type = Hid::MCUMappingType::Gamepad;
                    stick.gamepadAxis = profile.at(jp(side + "/typ_set/Arg")).get<BYTE>();
                    continue;

                case MappingType::KeyboardPreset:
                case MappingType::KeyboardPresetAlt:
                case MappingType::KeyboardDirections:
                    stick.type = Hid::MCUMappingType::Keyboard;
                    codes = stick.keyboardDirections;
                    break;

                case MappingType::MouseDirections:
                    stick.type = Hid::MCUMappingType::Mouse;
                    codes = stick.mouseDirections;
                    break;

                case MappingType::MediaDirections:
                    stick.type = Hid::MCUMappingType::Media;
                    codes = stick.mediaDirections;
                    break;

                default:
                    throw std::runtime_error("Unsupported profile stick binding");
                }

                const char* directions[] = {"U", "D", "L", "R"};
                for (size_t direction = 0; direction < 4; ++direction)
                {
                    const std::string path = side + directions[direction] + "/typ_set";
                    if (profile.value(jp(path + "/Type"), -1) != (int)stick.type)
                        throw std::runtime_error("Unsupported profile direction binding");
                    codes[direction] = profile.at(jp(path + "/Arg")).get<BYTE>();
                }
            }
            mapping = result;
            return true;
        }
        catch (const std::exception& error)
        {
            log.Warn("Could not load controller profile %lu: %s", static_cast<DWORD>(mode), error.what());
            return false;
        }
    }
}
