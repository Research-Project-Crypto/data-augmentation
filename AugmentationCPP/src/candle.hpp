#pragma once
#include "common.hpp"

struct candle
{
    //std::string m_timestamp;
    uint64_t m_timestamp;
    double m_open;
    double m_close;
    double m_high;
    double m_low;
    double m_volume;

    // indicators
    double m_adosc;

    double m_atr;

    double m_macd;
    double m_macd_signal;
    double m_macd_hist;

    double m_mfi;

    double m_upper_band;
    double m_middle_band;
    double m_lower_band;

    double m_rsi;



    candle() = default;
    candle(double timestamp, double open, double close, double high, double low, double volume)
    {
        m_timestamp = (uint64_t)timestamp;
        m_open = open;
        m_close = close;
        m_high = high;
        m_low = low;
        m_volume = volume;
    }
};
