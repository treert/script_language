#pragma once
#include <cstdint>

namespace oms{
    class String
    {
    public:
        String();
        explicit String(const char *str);
        ~String();

        String(const String &) = delete;
        void operator = (const String &) = delete;

        double GetHash()const{ return _hash; }
    private:
        void _Hash();
        double _hash;
        uint32_t _length;
        char *str;
    };
}