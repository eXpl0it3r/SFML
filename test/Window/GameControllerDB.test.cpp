#include <SFML/Window/Joystick.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("[Window] sf::Joystick GameControllerDB")
{
    SECTION("Load from memory - empty")
    {
        CHECK_FALSE(sf::Joystick::loadGameControllerDBFromMemory(nullptr));
        CHECK_FALSE(sf::Joystick::loadGameControllerDBFromMemory(""));
    }

    SECTION("Load from memory - minimal Windows line")
    {
        // Minimal SDL mapping line (GUID length may vary; this is synthetic for parser acceptance)
        const char* db = "03000000000000000000000000000000,Test Controller,platform:Windows,a:b0,b:b1,leftx:a0,lefty:a1";
        CHECK(sf::Joystick::loadGameControllerDBFromMemory(db));
    }

    SECTION("Load from file - non-existent path")
    {
        CHECK_FALSE(sf::Joystick::loadGameControllerDB("Z:/nonexistent/path/gamecontrollerdb.txt"));
    }
}
