#pragma once
#include "common.hpp"
#include "candle.hpp"

namespace program
{
    class symbol_processor final
    {
    private:
        std::filesystem::path m_input_file;
        const char* m_out_dir;

        std::vector<std::unique_ptr<candle>> m_candles;
        size_t m_alloc_size;
        double* m_open;
        double* m_close;
        double* m_high;
        double* m_low;
        double* m_volume;
        const int m_columns = 5;

    public:
        symbol_processor(std::filesystem::path file_path, const char* out_dir) :
            m_input_file(file_path), m_out_dir(out_dir)
        {

        }
        virtual ~symbol_processor()
        {
            delete[] m_open;
            delete[] m_close;
            delete[] m_high;
            delete[] m_low;
            delete[] m_volume;
        }

        void allocate_arrays()
        {
            m_alloc_size = m_candles.size();

            m_open = new double[m_alloc_size];
            m_close = new double[m_alloc_size];
            m_high = new double[m_alloc_size];
            m_low = new double[m_alloc_size];
            m_volume = new double[m_alloc_size];

            for (size_t i = 0; i < m_alloc_size; i++)
            {
                const std::unique_ptr<candle>& candle = m_candles.at(i);

                m_open[i] = candle->m_close;
                m_close[i] = candle->m_close;
                m_high[i] = candle->m_high;
                m_low[i] = candle->m_low;
                m_volume[i] = candle->m_volume;
            }

            g_log->verbose("SYMBOL_PROCESSOR", "Allocated double arrays of %d bytes for %s.", m_alloc_size * sizeof(double) * m_columns, m_input_file.filename().c_str());
        }

        const char* const file_name()
        {
            return m_input_file.filename().c_str();
        }

        bool read_input_file()
        {
            io::CSVReader<6> input_stream(m_input_file.string().c_str());
            input_stream.read_header(io::ignore_missing_column | io::ignore_extra_column, "event_time", "open", "close", "high", "low", "volume");

            try
            {
                std::string timestamp;
                double open, close, high, low, volume;
                while (input_stream.read_row(timestamp, open, close, high, low, volume))
                {
                    std::unique_ptr<candle> new_candle = std::make_unique<candle>(timestamp, open, close, high, low, volume);

                    m_candles.push_back(std::move(new_candle));
                }

            }
            catch(const std::exception& e)
            {
                g_log->error("SYMBOL_PROCESSOR", "Failure while reading csv:\n%s", e.what());

                return false;
            }

            g_log->verbose("SYMBOL_PROCESSOR", "Loaded %d candles from %s", m_candles.size(), this->file_name());

            return true;
        }

        void calculate_adosc(const size_t fast_period = 24, const size_t slow_period = 45)
        {
            g_log->verbose("SYMBOL_PROCESSOR", "Starting processing of ADOSC for %s", this->file_name());

            double* tmp_adosc = new double[m_alloc_size];

            int beginIdx, endIdx;
            TA_ADOSC(0, m_alloc_size, m_high, m_low, m_close, m_volume, fast_period, slow_period, &beginIdx, &endIdx, tmp_adosc);

            for (size_t i = beginIdx; i < endIdx; i++)
                m_candles.at(i)->m_adosc = tmp_adosc[i];

            delete[] tmp_adosc;

            g_log->verbose("SYMBOL_PROCESSOR", "Finished processing ADOSC on data for %s", this->file_name());
        }

        void calculate_atr(const size_t period_range = 24)
        {
            g_log->verbose("SYMBOL_PROCESSOR", "Starting processing of ATR for %s", this->file_name());

            double* tmp_atr = new double[m_alloc_size];

            int beginIdx, endIdx;
            TA_ATR(0, m_alloc_size, m_high, m_low, m_close, period_range, &beginIdx, &endIdx, tmp_atr);

            for (size_t i = beginIdx; i < endIdx; i++)
                m_candles.at(i)->m_atr = tmp_atr[i];

            delete[] tmp_atr;

            g_log->verbose("SYMBOL_PROCESSOR", "Finished processing ATR on data for %s", this->file_name());
        }

        void calculate_bollinger_bands(const size_t period_range = 20, const size_t optInNbDevUp = 2, const size_t optInNbDevDown = 2)
        {
            // optInNbDevUp & optInNbDevDown = standard deviation for upper and lower band, usually 2 is used
            g_log->verbose("SYMBOL_PROCESSOR", "Starting processing of Bollinger Bands for %s", this->file_name());

            double* tmp_upper_band = new double[m_alloc_size];
            double* tmp_middle_band = new double[m_alloc_size];
            double* tmp_lower_band = new double[m_alloc_size];

            int beginIdx, endIdx;
            TA_MAType MAtype = TA_MAType_SMA;
            TA_BBANDS(0, m_alloc_size, m_close, period_range, optInNbDevUp, optInNbDevDown, MAtype, &beginIdx, &endIdx, tmp_upper_band, tmp_middle_band, tmp_lower_band);

            for (size_t i = beginIdx; i < endIdx; i++)
            {
                const std::unique_ptr<candle>& candle = m_candles.at(i);

                candle->m_upper_band = tmp_upper_band[i];
                candle->m_middle_band = tmp_middle_band[i];
                candle->m_lower_band = tmp_lower_band[i];
            }

            delete[] tmp_upper_band;
            delete[] tmp_middle_band;
            delete[] tmp_lower_band;

            g_log->verbose("SYMBOL_PROCESSOR", "Finished processing BBANDS on data for %s", this->file_name());
        }

        void calculate_macd(const size_t fast_period = 12, const size_t slow_period = 26, const size_t signal_period = 9)
        {
            g_log->verbose("SYMBOL_PROCESSOR", "Starting processing of MACD for %s", this->file_name());

            double* tmp_macd = new double[m_alloc_size];
            double* tmp_macd_signal = new double[m_alloc_size];
            double* tmp_macd_hist = new double[m_alloc_size];

            int beginIdx, endIdx;
            TA_MACD(0, m_alloc_size, m_close, fast_period, slow_period, signal_period, &beginIdx, &endIdx, tmp_macd, tmp_macd_signal, tmp_macd_hist);

            for (size_t i = beginIdx; i < endIdx; i++)
            {
                const std::unique_ptr<candle>& candle = m_candles.at(i);

                candle->m_macd = tmp_macd[i];
                candle->m_macd_signal = tmp_macd_signal[i];
                candle->m_macd_hist = tmp_macd_hist[i];
            }

            delete[] tmp_macd;
            delete[] tmp_macd_signal;
            delete[] tmp_macd_hist;

            g_log->verbose("SYMBOL_PROCESSOR", "Finished processing MFI on data for %s", this->file_name());
        }

        void calculate_mfi(const size_t period_range = 30)
        {
            g_log->verbose("SYMBOL_PROCESSOR", "Starting processing of MFI for %s", this->file_name());

            double* tmp_mfi = new double[m_alloc_size];

            int beginIdx, endIdx;
            TA_MFI(0, m_alloc_size, m_high, m_low, m_close, m_volume, period_range, &beginIdx, &endIdx, tmp_mfi);

            for (size_t i = beginIdx; i < endIdx; i++)
                m_candles.at(i)->m_mfi = tmp_mfi[i];

            delete[] tmp_mfi;

            g_log->verbose("SYMBOL_PROCESSOR", "Finished processing MFI on data for %s", this->file_name());
        }

        void calculate_rsi(const size_t period_range = 14)
        {
            g_log->verbose("SYMBOL_PROCESSOR", "Starting processing of RSI for %s", this->file_name());

            double* tmp_rsi = new double[m_alloc_size];

            int beginIdx, endIdx;
            TA_RSI(0, m_alloc_size, m_close, period_range, &beginIdx, &endIdx, tmp_rsi);

            for (size_t i = beginIdx; i < endIdx; i++)
                m_candles.at(i)->m_rsi = tmp_rsi[i];

            delete[] tmp_rsi;

            g_log->verbose("SYMBOL_PROCESSOR", "Finished processing RSI on data for %s", this->file_name());
        }


        void start()
        {
            if (!this->read_input_file()) return;

            this->allocate_arrays();

            // do our indicator calculation
            this->calculate_adosc();            
            this->calculate_atr();
            this->calculate_bollinger_bands();
            this->calculate_macd();
            this->calculate_mfi();
            this->calculate_rsi();

            this->write_to_out();
        }

        void write_to_out()
        {
            CSVWriter csv_output;
            csv_output.newRow()
                << "event_time" << "open" << "close" << "high" << "low" << "volume"
                << "adosc"
                << "atr"
                << "upper_band" << "middle_band" << "lower_band"
                << "macd" << "macd_signal" << "macd_hist"
                << "mfi"
                << "rsi";

            for (const std::unique_ptr<candle>& candle : m_candles)
            {
                csv_output.newRow()
                    << candle->m_timestamp << candle->m_open << candle->m_close << candle->m_high << candle->m_low << candle->m_volume
                    << candle->m_adosc
                    << candle->m_atr
                    << candle->m_upper_band << candle->m_middle_band << candle->m_lower_band
                    << candle->m_macd << candle->m_macd_signal << candle->m_macd_hist
                    << candle->m_mfi
                    << candle->m_rsi;
            }

            std::string out_dir = m_out_dir / m_input_file.filename();
            csv_output.writeToFile(out_dir.c_str(), false);
        }
    };
}
