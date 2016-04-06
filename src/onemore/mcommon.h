#pragma once

#define DISABLE_DEFALT_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName &) = delete; \
    void operator = (const TypeName &) = delete;