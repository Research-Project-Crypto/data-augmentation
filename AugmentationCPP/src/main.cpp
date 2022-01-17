#include "common.hpp"
#include "symbol_processor.hpp"
#include <exception>

using namespace program;

/**
 * @param {number} argc Argument count
 * @param {Array<char*>} argv Array of arguments
 */
int main(int argc, const char** argv)
{
#ifdef NDEBUG
    g_log->set_log_level(Logger::LogLevel::Info);
#endif

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

    try
    {
        if (TA_RetCode code = TA_Initialize(); code != TA_SUCCESS)
        {
            throw std::runtime_error("TA_Initialize did not return TA_SUCCESS.");
        }
    }
    catch(const std::exception& e)
    {
        TA_Shutdown();

        g_log->error("MAIN", "Error occurred while called TA_Initialize:\n%s", e.what());
        return 1;
    }

    std::chrono::time_point start_time = std::chrono::system_clock::now();
    g_log->info("MAIN", "Starting parsing of files...");
    for (const auto file : std::filesystem::directory_iterator(input_folder))
    {
        g_thread_pool->push([=]()
        {
            g_log->verbose("THREAD", "Processing file: %s", file.path().string().c_str());

            if (!file.is_directory())
            {
                symbol_processor processor(file, output_folder);
                processor.start();
            }
        });
    }

    while (thread_pool_instance->has_jobs())
    {
        std::this_thread::sleep_for(500ms);
    }

    std::chrono::duration seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start_time);
    std::chrono::duration minutes = std::chrono::duration_cast<std::chrono::minutes>(seconds);
    seconds -= minutes;
    g_log->info("MAIN", "Finished processing files in %dmin, %dsecs",
        minutes,
        seconds
    );

    g_log->info("MAIN", "Waiting for all threads to exit...");
    thread_pool_instance->destroy();
    thread_pool_instance.reset();

    g_log->info("MAIN", "Farewell!");

    return 0;
}
