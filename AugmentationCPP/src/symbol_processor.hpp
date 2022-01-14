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

        void start()
        {
            if (!this->read_input_file()) return;

            this->allocate_arrays();

            // do our indicator calculation
            this->calculate_macd();
            this->calculate_mfi();

            this->write_to_out();
        }

        void write_to_out()
        {
            CSVWriter csv_output;
            csv_output.newRow()
                << "event_time" << "open" << "close" << "high" << "low" << "volume"
                << "macd" << "macd_signal" << "macd_hist"
                << "mfi";

            for (const std::unique_ptr<candle>& candle : m_candles)
            {
                csv_output.newRow()
                    << candle->m_timestamp << candle->m_open << candle->m_close << candle->m_high << candle->m_low << candle->m_volume
                    << candle->m_macd << candle->m_macd_signal << candle->m_macd_hist
                    << candle->m_mfi;
            }

            std::string out_dir = m_out_dir / m_input_file.filename();
            csv_output.writeToFile(out_dir.c_str(), false);
        }
    };
}
