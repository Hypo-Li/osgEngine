#include <ThirdParty/spdlog/spdlog.h>
#include <ThirdParty/spdlog/sinks/stdout_color_sinks.h>

#include <filesystem>

#define LOG_TRACE(...) 

#if 0
namespace xxx
{
    class Logger
    {
    public:
        Logger()
        {
            mSpdLogger = spdlog::stdout_color_mt("log test");
            setLevel(Level::Trace);
        }

        enum class Level
        {
            Trace,
            Debug,
            Info,
            Warn,
            Error,
            Critical,
        };

        void setLevel(Level level)
        {
            mLevel = level;
            switch (level)
            {
            case Level::Trace:
                mSpdLogger->set_level(spdlog::level::trace);
                break;
            case Level::Debug:
                mSpdLogger->set_level(spdlog::level::debug);
                break;
            case Level::Info:
                mSpdLogger->set_level(spdlog::level::info);
                break;
            case Level::Warn:
                mSpdLogger->set_level(spdlog::level::warn);
                break;
            case Level::Error:
                mSpdLogger->set_level(spdlog::level::err);
                break;
            case Level::Critical:
                mSpdLogger->set_level(spdlog::level::critical);
                break;
            default:
                break;
            }
        }

        auto getLogger()
        {
            return mSpdLogger;
        }

    protected:
        Level mLevel;
        std::shared_ptr<spdlog::logger> mSpdLogger;
        std::filesystem::path mLogDir;
    };
}
#endif // 0

