#include "common.hpp"

using namespace program;

struct candle
{
    std::string event_time;
    double open;
    double close;
    double high;
    double low;
    double volume;

    candle calculate(candle& other, candle& processedother)
    {
        candle candle;

        candle.event_time = other.event_time;

        return candle;
    }
};

void process_csv(const char* in_file, const char* out)
{
    CSVWriter csv;
    io::CSVReader<6> in(in_file);
    
    in.read_header(io::ignore_missing_column | io::ignore_extra_column, "event_time", "open", "close", "high", "low", "volume");
    csv.newRow() << "event_time" << "open" << "close" << "high" << "low" << "volume";

    std::stack<candle> candles;
    std::stack<candle> processed_candles;

    candle first_candle;
    if (!in.read_row(first_candle.event_time, first_candle.open, first_candle.close, first_candle.high, first_candle.low, first_candle.volume)) return;
    candles.push(first_candle);
    candle next_candle;
    while (in.read_row(next_candle.event_time, next_candle.open, next_candle.close, next_candle.high, next_candle.low, next_candle.volume))
    {
        candle copy = next_candle;
        candles.push(copy);
    }

    // g_log->info("MAIN", "stack size %s", candles.size());

    // candle candlesArray[candles.size()];
    

    
    // csv.newRow() << change_candle.event_time << change_candle.open << change_candle.close << change_candle.high << change_candle.low << change_candle.volume << change_candle.differencelowhigh << change_candle.differenceopenclose << change_candle.maxprofitclose << change_candle.maxprofitlowhigh; 
    // write to file https://github.com/al-eax/CSVWriter
    // char outpath[100];
    // strcpy(outpath, out);
    // std::filesystem::path path{in_file};
    // strcat(outpath, path.filename().string().c_str());
    // csv.writeToFile(outpath, true);
}

/**
 * @param {number} argc Argument count
 * @param {Array<char*>} argv Array of arguments
 */
int main(int argc, const char** argv)
{
    g_log->info("MAIN", "Initiating thread pool.");
    auto thread_pool_instance = std::make_unique<thread_pool>();

    if (argc < 3)
    {
        g_log->error("MAIN", "Missing arguments, input_folder and/or output_folder");

        return 1;
    }

    const char* input_folder = argv[1];
    const char* output_folder = argv[2];

    if (!std::filesystem::exists(input_folder) || !std::filesystem::exists(output_folder))
    {
        g_log->error("MAIN", "Input and/or output folder do not exist.");

        return 1;
    }

    for (const auto file : std::filesystem::directory_iterator(input_folder))
    {
        g_thread_pool->push([=]()
        {
            g_log->info("THREAD", "Processing file: %s", file.path().string().c_str());

            if (!file.is_directory())
                process_csv(file.path().string().c_str(), output_folder);
        });
    }

    while (thread_pool_instance->has_jobs())
    {
        std::this_thread::sleep_for(500ms);
    }

    thread_pool_instance->destroy();
    thread_pool_instance.reset();

    system("pause");

    return 0;
}