#pragma once
#include <string>
#include <sstream>
#include <utility>

namespace oms
{
    // Base exception for oms, all exceptions throwed by oms
    // are derived from this class
    class Exception
    {
    public:
        const std::string& What() const { return what_; }
        Exception() = default;
        template<typename... Args>
        Exception(Args&&... args)
        {
            SetWhat(std::forward<Args>(args)...);
        }

    protected:
        // Helper functions for format string of exception
        void SetWhat(std::ostringstream &) { }

        template<typename Arg, typename... Args>
        void SetWhat(std::ostringstream &oss, Arg&& arg, Args&&... args)
        {
            oss << std::forward<Arg>(arg);
            SetWhat(oss, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void SetWhat(Args&&... args)
        {
            std::ostringstream oss;
            SetWhat(oss, std::forward<Args>(args)...);
            what_ = oss.str();
        }

    private:
        std::string what_;
    };
} // namespace oms
