#pragma once

#define DISABLE_DEFALT_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName &) = delete; \
    void operator = (const TypeName &) = delete;

#include <functional>

// Guard class, using for RAII operations.
// e.g.
//      {
//          Guard g(constructor, destructor);
//          ...
//      }
class Guard
{
public:
    Guard(const std::function<void()> &enter,
        const std::function<void()> &leave)
        : leave_(leave)
    {
        enter();
    }

    ~Guard()
    {
        leave_();
    }

    DISABLE_DEFALT_COPY_AND_ASSIGN(Guard);

private:
    std::function<void()> leave_;
};