#include "common.hpp"
#include "symbol_processor.hpp"

using namespace program;

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

    thread_pool_instance->destroy();
    thread_pool_instance.reset();

    return 0;
}
