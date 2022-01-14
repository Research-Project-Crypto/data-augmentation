#pragma once
#include "common.hpp"

struct candle
{
    std::string event_time;
    double open;
    double close;
    double high;
    double low;
    double volume;

    // indicators
    double mfi;
    double macd;
    double rsi;

    candle() = default;
    candle(std::string timestamp, double open, double close, double high, double low, double volume)
    {
        event_time = timestamp;
        open = open;
        close = close;
        high = high;
        low = low;
        volume = volume;
    }
};