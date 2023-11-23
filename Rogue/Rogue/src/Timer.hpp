#include <iostream>
#include <chrono>
#include <unordered_map>
#include <format>

struct TimerResult {
    float allTimes = 0;
    float sampleCount = 0;
};
inline std::unordered_map<const char*, TimerResult> _timerResults;

struct Timer {

    std::chrono::time_point<std::chrono::steady_clock> _startTime;
    const char* _name;

    Timer(const char* name) {
        _startTime = std::chrono::steady_clock::now();
        _name = name;
    }

    ~Timer() {
        std::chrono::duration<float> duration = std::chrono::steady_clock::now() - _startTime;
        float time = duration.count() * 1000.0f;
        std::string spacing;

        for (int i = 0; i < (40 - std::string(_name).length()); i++)
            spacing += " ";

        _timerResults[_name].allTimes += time;
        _timerResults[_name].sampleCount++;
     
        std::cout << _name << ": " << spacing << std::format("{:.4f}", time) << "ms      average: " << std::format("{:.4f}", _timerResults[_name].allTimes / _timerResults[_name].sampleCount) << "ms\n";
    }
};