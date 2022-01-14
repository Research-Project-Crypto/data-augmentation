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

    public:
        symbol_processor(std::filesystem::path file_path, const char* out_dir) :
            m_input_file(file_path), m_out_dir(out_dir)
        {

        }
        ~symbol_processor()
        {

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

            g_log->verbose("SYMBOL_PROCESSOR", "Loaded %d candles from %s", m_candles.size(), m_input_file.filename().string().c_str());

            return true;
        }

        void calculate_mfi(const size_t period_range = 30)
        {
            g_log->verbose("SYMBOL_PROCESSOR", "Starting processing of MFI.");

            const size_t alloc_size = m_candles.size();

            g_log->verbose("SYMBOL_PROCESSOR", "Allocating double arrays of %d.", alloc_size);

            double* close = new double[alloc_size];
            double* high = new double[alloc_size];
            double* low = new double[alloc_size];
            double* volume = new double[alloc_size];

            double mfi_out[alloc_size - period_range];

            for (size_t i = 0; i < alloc_size; i++)
            {
                const std::unique_ptr<candle>& candle = m_candles.at(i);

                close[i] = candle->m_close;
                high[i] = candle->m_high;
                low[i] = candle->m_low;
                volume[i] = candle->m_volume;
            }

            int beginIdx, endIdx;
            for (size_t i = period_range; i < alloc_size; i++)
            {
                double tmp_mfi[period_range];
                TA_MFI(i, i + period_range, high, low, close, volume, period_range, &beginIdx, &endIdx, tmp_mfi);

                m_candles.at(i)->m_mfi = tmp_mfi[period_range - 1];
            }

            delete[] close;
            delete[] high;
            delete[] low;
            delete[] volume;

            g_log->verbose("SYMBOL_PROCESSOR", "Finished processing MFI on data.");
        }

        void start()
        {
            this->read_input_file();

            // do our indicator calculation
            this->calculate_mfi();

            this->write_to_out();
        }

        void write_to_out()
        {
            CSVWriter csv_output;
            csv_output.newRow() << "event_time" << "open" << "close" << "high" << "low" << "volume" << "mfi";

            for (const std::unique_ptr<candle>& candle : m_candles)
            {
                csv_output.newRow()
                    << candle->m_timestamp << candle->m_open << candle->m_close << candle->m_high << candle->m_low << candle->m_volume
                    << candle->m_mfi;
            }

            std::string out_dir = m_out_dir / m_input_file.filename();
            csv_output.writeToFile(out_dir.c_str(), false);
        }
    };
}
