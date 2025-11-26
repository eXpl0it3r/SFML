#pragma once
#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/JoystickImpl.hpp>
#include <unordered_map>

namespace sf::priv
{
struct GameMapping
{
    std::array<int, 11> buttonMap{}; // -1 if unmapped
    std::array<int, 6> axisMap{};    // -1 if unmapped
    std::array<int, 6> axisSign{};   // 1 or -1
};

struct VidPidKey
{
    unsigned int vid{};
    unsigned int pid{};
    bool operator==(const VidPidKey& o) const { return vid == o.vid && pid == o.pid; }
};

struct VidPidHash
{
    std::size_t operator()(const VidPidKey& k) const noexcept
    {
        return (static_cast<std::size_t>(k.vid) << 32) ^ static_cast<std::size_t>(k.pid);
    }
};

bool loadGameControllerDBFile(const char* path);
bool loadGameControllerDBMemory(const char* data);
void apply(const GameMapping& m, JoystickState& state);
bool getMappingFor(unsigned int vid, unsigned int pid, GameMapping& out);
} // namespace sf::priv
