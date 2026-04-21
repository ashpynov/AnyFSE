#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <filesystem>

#include "Steam.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Unicode.hpp"
#include <algorithm>

namespace AnyFSE::Tools::Steam
{
    struct VDFNode
    {
        std::string name;
        std::string value;
        std::unordered_map<std::string, std::shared_ptr<VDFNode>> children;

        VDFNode() = default;
        VDFNode(const std::string &n) : name(n) {}
    };
    class VDFParser
    {
    private:
        std::string content;
        size_t pos = 0;

        void skipWhitespace()
        {
            while (pos < content.length() && std::isspace(static_cast<unsigned char>(content[pos])))
            {
                pos++;
            }
        }

        void skipComments()
        {
            while (pos < content.length())
            {
                if (content[pos] == '/' && pos + 1 < content.length() && content[pos + 1] == '/')
                {
                    // Skip until end of line
                    while (pos < content.length() && content[pos] != '\n')
                    {
                        pos++;
                    }
                    pos++; // Skip newline
                }
                else
                {
                    break;
                }
            }
        }

        std::string parseToken()
        {
            skipWhitespace();
            skipComments();
            skipWhitespace();

            if (pos >= content.length())
            {
                return "";
            }

            std::string token;

            // Check for quoted token
            if (content[pos] == '"')
            {
                pos++; // Skip opening quote
                while (pos < content.length() && content[pos] != '"')
                {
                    if (content[pos] == '\\' && pos + 1 < content.length())
                    {
                        // Handle escape sequences
                        pos++;
                        switch (content[pos])
                        {
                        case 'n':
                            token += '\n';
                            break;
                        case 't':
                            token += '\t';
                            break;
                        case '\\':
                            token += '\\';
                            break;
                        case '"':
                            token += '"';
                            break;
                        default:
                            token += content[pos];
                            break;
                        }
                    }
                    else
                    {
                        token += content[pos];
                    }
                    pos++;
                }
                if (pos < content.length() && content[pos] == '"')
                {
                    pos++; // Skip closing quote
                }
            }
            else
            {
                // Unquoted token - read until whitespace or control character
                while (pos < content.length())
                {
                    char c = content[pos];
                    if (std::isspace(static_cast<unsigned char>(c)) || c == '{' || c == '}')
                    {
                        break;
                    }
                    token += c;
                    pos++;
                }
            }

            return token;
        }

        // Parse a node and its children recursively
        std::shared_ptr<VDFNode> parseNode()
        {
            std::string nodeName = parseToken();
            if (nodeName.empty())
            {
                return nullptr;
            }

            auto node = std::make_shared<VDFNode>(nodeName);

            skipWhitespace();
            skipComments();

            // Check if this node has children (starts with '{')
            if (pos < content.length() && content[pos] == '{')
            {
                pos++; // Skip '{'

                while (true)
                {
                    skipWhitespace();
                    skipComments();

                    if (pos >= content.length())
                    {
                        break;
                    }

                    if (content[pos] == '}')
                    {
                        pos++; // Skip '}'
                        break;
                    }

                    // Try to parse child node
                    auto child = parseNode();
                    if (child)
                    {
                        node->children[child->name] = child;
                    }
                    else
                    {
                        std::string value = parseToken();
                        node->value = value;
                    }
                }
            }
            else
            {
                std::string value = parseToken();
                node->value = value;
            }

            return node;
        }

    public:
        // Parse from string
        std::shared_ptr<VDFNode> parseString(const std::string &input)
        {
            content = input;
            pos = 0;

            auto root = std::make_shared<VDFNode>("root");

            while (pos < content.length())
            {
                auto node = parseNode();
                if (node)
                {
                    root->children[node->name] = node;
                }
                else
                {
                    break;
                }
            }

            return root;
        }

        // Parse from file
        std::shared_ptr<VDFNode> parseFile(const std::string &filename)
        {
            std::ifstream file(filename);
            if (!file.is_open())
            {
                throw std::runtime_error("Cannot open file: " + filename);
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            return parseString(buffer.str());
        }
    };

    std::wstring GetConfigPath()
    {
        namespace fs = std::filesystem;
        try
        {
            std::wstring steamPath = Tools::Registry::ReadString(L"HKLM\\SOFTWARE\\Valve\\Steam", L"InstallPath");
            if(steamPath.empty())
            {
                throw std::exception("Steam is not installed");
            }

            std::wstring loginUsersVdf = steamPath + L"\\config\\loginusers.vdf";

            if (!fs::exists(loginUsersVdf))
            {
                throw std::exception("loginusers.vdf file not found");
            }
            std::string userId;
            VDFParser loginParser;
            auto root = loginParser.parseFile(Unicode::to_string(loginUsersVdf));
            for (auto c: root->children["users"]->children)
            {
                if (c.second->children.find("MostRecent") != c.second->children.end())
                {
                    if (c.second->children["MostRecent"]->value == "1")
                    {
                        userId = c.first;
                        break;
                    }
                }
            }
            if (userId.empty())
            {
                throw std::exception("UserId not found");
            }

            uint64_t STEAM_ID_OFFSET = 76561197960265728ULL;
            uint64_t fullId = atoll(userId.c_str());
            if (STEAM_ID_OFFSET > fullId)
            {
                throw std::exception("Id is too small");
            }
            fullId = fullId - STEAM_ID_OFFSET;

            std::wstring configPath = steamPath + L"\\userdata\\" + std::to_wstring(fullId) + L"\\config\\localconfig.vdf";
            if (!fs::exists(configPath))
            {
                throw std::exception("localconfig.vdf file not found");
            }
            return configPath;
        }
        catch (...) {}

        return L"";
    }

    std::vector<std::string> SplitString(const std::string &key, const char *delim)
    {
        std::vector<std::string> result;
        const char *end, *start = key.c_str();
        do
        {
            end = strpbrk(start, delim); // find separator
            result.push_back(end ? std::string(start, end - start) : std::string(start));

            if(end) start = end + strspn(end, delim); // skip separator

        } while (end && *end);

        return result;
    }

    std::wstring GetConfigValue(const std::wstring &key, const std::wstring &defValue)
    {
        std::wstring configPath = GetConfigPath();
        if (configPath.empty())
        {
            return defValue;
        }

        VDFParser configParser;
        auto root = configParser.parseFile(Unicode::to_string(configPath));

        auto path = SplitString(Unicode::to_string(key), ".");

        auto node = root;
        try
        {
            for (auto p: path)
            {
                node = node->children[p];
            }
            return Unicode::to_wstring(node->value);
        }
        catch(...) {}

        return defValue;
    }

    WORD GetVKeyByName(const std::string &keyName)
    {
        if (keyName.length() == 1
            && ( (keyName[0] >= '0' || keyName[0] <= '9')
                || (keyName[0] >= 'A' || keyName[0] <= 'Z')))
        {
            return (WORD)keyName[0];
        }

        if (keyName.length() > 1 && keyName[0] == 'F')
        {
            int i = atoi(keyName.c_str() + 1);
            if (i >= 1 && i <=24)
            {
                return (WORD)VK_F1 + i - 1;
            }
        }

        std::unordered_map<std::string, WORD> nameMap =
        {
            {"TAB", VK_TAB},
            {"ENTER", VK_RETURN},
            {"RETURN", VK_RETURN},
            {"SPACE", VK_SPACE},
            {"BACKSPACE", VK_BACK},
            {"DELETE", VK_DELETE},
            {"INSERT", VK_INSERT},
            {"ESC", VK_ESCAPE},
            {"ESCAPE", VK_ESCAPE},
            {"UP", VK_UP},
            {"DOWN", VK_DOWN},
            {"LEFT", VK_LEFT},
            {"RIGHT", VK_RIGHT},
            {"CTRL", VK_LCONTROL},
            {"CONTROL", VK_LCONTROL},
            {"ALT", VK_LMENU},
            {"SHIFT", VK_LSHIFT},
            {"HOME", VK_HOME},
            {"END", VK_END},
            {"PGUP", VK_PRIOR},
            {"PAGEUP",VK_PRIOR },
            {"PAGEDOWN", VK_NEXT},
            {"PGDN",VK_NEXT},
            {"INSERT", VK_INSERT},
            {"DELETE", VK_DELETE},
            {"DEL", VK_DELETE},
            {";",VK_OEM_1},      // ;:
            {"/",VK_OEM_2},      // /?
            {"`",VK_OEM_3},      // `~
            {"[",VK_OEM_4},      // [{
            {"\\",VK_OEM_5},     // \|
            {"]",VK_OEM_6},      // ]}
            {"'",VK_OEM_7},      // '"
            {"=",VK_OEM_PLUS},   // =+
            {"-",VK_OEM_MINUS},  // -_
            {",",VK_OEM_COMMA},  // ,<
            {".",VK_OEM_PERIOD}  // .>
        };

        if (nameMap.find(keyName.c_str()) != nameMap.end())
        {
            return nameMap[keyName.c_str()];
        }

        return 0;
    }

    std::vector<WORD> ParseKeySequence(const std::string &steamSequence)
    {
        std::vector<WORD> result;
        std::vector<std::string> keyNames = SplitString(steamSequence, "\t +");
        for( auto k : keyNames)
        {
            std::string name = Unicode::to_upper(k);
            if (strstr(name.c_str(), "KEY_") == name.c_str())
            {
                name = name.substr(4);
            }
            WORD vkKey = GetVKeyByName(name);
            if (vkKey)
            {
                result.push_back(vkKey);
            }
        }
        return result;
    }

    std::vector<WORD> GetOverlaySequence()
    {
        std::wstring settings = AnyFSE::Tools::Steam::GetConfigValue(
            L"UserLocalConfigStore.system.InGameOverlayShortcutKey",
            L"Shift+Tab"
        );
        if (!settings.empty())
        {
            return AnyFSE::Tools::Steam::ParseKeySequence(Unicode::to_string(settings));
        }
        return std::vector<WORD>();
    }
};