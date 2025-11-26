#include "GameControllerMapping.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace sf::priv
{
static std::unordered_map<VidPidKey, GameMapping, VidPidHash> g_mappings;

static bool parseLine(const std::string& line, GameMapping& out, unsigned int& vid, unsigned int& pid)
{
    if (line.empty() || line[0] == '#')
        return false;
    if (line.find("platform:Windows") == std::string::npos)
        return false;

    std::stringstream ss(line);
    std::string guid, name;
    if (!std::getline(ss, guid, ',')) return false;
    if (!std::getline(ss, name, ',')) return false;

    out.buttonMap.fill(-1);
    out.axisMap.fill(-1);
    out.axisSign.fill(1);

    vid = 0;
    pid = 0;
    if (guid.size() >= 32)
    {
        auto vidHex = guid.substr(16, 8);
        auto pidHex = guid.substr(24, 8);
        try
        {
            vid = std::stoul(vidHex, nullptr, 16);
            pid = std::stoul(pidHex, nullptr, 16);
        }
        catch (...)
        {
            vid = 0;
            pid = 0;
        }
    }

    // Skip to after third comma
    std::size_t thirdComma = std::string::npos;
    int commas = 0;
    for (std::size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == ',')
        {
            ++commas;
            if (commas == 3)
            {
                thirdComma = i;
                break;
            }
        }
    }
    if (thirdComma == std::string::npos) return false;
    std::string rest = line.substr(thirdComma + 1);

    std::stringstream restSS(rest);
    std::string token;
    while (std::getline(restSS, token, ','))
    {
        auto colon = token.find(':');
        if (colon == std::string::npos) continue;
        std::string key = token.substr(0, colon);
        std::string val = token.substr(colon + 1);

        auto parseButtonIndex = [&](const std::string& v) -> int {
            if (v.empty() || v[0] != 'b') return -1;
            try { return std::stoi(v.substr(1)); } catch (...) { return -1; }
        };
        auto parseAxisIndex = [&](const std::string& v, int& signOut) -> int {
            signOut = 1;
            bool invert = false;
            std::string s = v;
            auto tilde = s.find('~');
            if (tilde != std::string::npos)
            {
                invert = true;
                s.erase(tilde, 1);
            }
            if (s.empty() || s[0] != 'a') return -1;
            try {
                int idx = std::stoi(s.substr(1));
                signOut = invert ? -1 : 1;
                return idx;
            } catch (...) { return -1; }
        };

        if (key == "a") out.buttonMap[0] = parseButtonIndex(val);
        else if (key == "b") out.buttonMap[1] = parseButtonIndex(val);
        else if (key == "x") out.buttonMap[2] = parseButtonIndex(val);
        else if (key == "y") out.buttonMap[3] = parseButtonIndex(val);
        else if (key == "back") out.buttonMap[4] = parseButtonIndex(val);
        else if (key == "guide") out.buttonMap[5] = parseButtonIndex(val);
        else if (key == "start") out.buttonMap[6] = parseButtonIndex(val);
        else if (key == "leftstick") out.buttonMap[7] = parseButtonIndex(val);
        else if (key == "rightstick") out.buttonMap[8] = parseButtonIndex(val);
        else if (key == "leftshoulder") out.buttonMap[9] = parseButtonIndex(val);
        else if (key == "rightshoulder") out.buttonMap[10] = parseButtonIndex(val);
        else if (key == "leftx") { int sgn; out.axisMap[0] = parseAxisIndex(val, sgn); out.axisSign[0] = sgn; }
        else if (key == "lefty") { int sgn; out.axisMap[1] = parseAxisIndex(val, sgn); out.axisSign[1] = sgn; }
        else if (key == "rightx") { int sgn; out.axisMap[2] = parseAxisIndex(val, sgn); out.axisSign[2] = sgn; }
        else if (key == "righty") { int sgn; out.axisMap[3] = parseAxisIndex(val, sgn); out.axisSign[3] = sgn; }
        else if (key == "lefttrigger") { int sgn; out.axisMap[4] = parseAxisIndex(val, sgn); out.axisSign[4] = sgn; }
        else if (key == "righttrigger") { int sgn; out.axisMap[5] = parseAxisIndex(val, sgn); out.axisSign[5] = sgn; }
    }

    return true;
}

bool loadGameControllerDBFile(const char* path)
{
    if (!path) return false;
    std::ifstream ifs(path);
    if (!ifs) return false;
    std::string line;
    unsigned int vid, pid;
    GameMapping m;
    bool any = false;
    while (std::getline(ifs, line))
    {
        if (parseLine(line, m, vid, pid))
        {
            g_mappings[{vid, pid}] = m;
            any = true;
        }
    }
    return any;
}

bool loadGameControllerDBMemory(const char* data)
{
    if (!data) return false;
    std::stringstream ss(data);
    std::string line;
    unsigned int vid, pid;
    GameMapping m;
    bool any = false;
    while (std::getline(ss, line))
    {
        if (parseLine(line, m, vid, pid))
        {
            g_mappings[{vid, pid}] = m;
            any = true;
        }
    }
    return any;
}

bool getMappingFor(unsigned int vid, unsigned int pid, GameMapping& out)
{
    auto it = g_mappings.find(VidPidKey{vid, pid});
    if (it == g_mappings.end()) return false;
    out = it->second;
    return true;
}

void apply(const GameMapping& m, JoystickState& state)
{
    std::array<bool, Joystick::ButtonCount> newButtons{};
    newButtons.fill(false);
    for (std::size_t i = 0; i < m.buttonMap.size(); ++i)
    {
        int src = m.buttonMap[i];
        if (src >= 0 && src < static_cast<int>(Joystick::ButtonCount))
            newButtons[i] = state.buttons[static_cast<unsigned int>(src)];
    }
    for (std::size_t i = 0; i < m.buttonMap.size() && i < newButtons.size(); ++i)
        state.buttons[i] = newButtons[i];

    auto setAxis = [&](Joystick::Axis dst, int srcIdx, int sgn) {
        if (srcIdx >= 0 && srcIdx < static_cast<int>(Joystick::AxisCount))
        {
            Joystick::Axis src = static_cast<Joystick::Axis>(srcIdx);
            float val = state.axes[src] * static_cast<float>(sgn);
            state.axes[dst] = val;
        }
    };
    setAxis(Joystick::Axis::X, m.axisMap[0], m.axisSign[0]);
    setAxis(Joystick::Axis::Y, m.axisMap[1], m.axisSign[1]);
    setAxis(Joystick::Axis::U, m.axisMap[2], m.axisSign[2]);
    setAxis(Joystick::Axis::V, m.axisMap[3], m.axisSign[3]);
    setAxis(Joystick::Axis::Z, m.axisMap[4], m.axisSign[4]);
    setAxis(Joystick::Axis::R, m.axisMap[5], m.axisSign[5]);
}
} // namespace sf::priv
