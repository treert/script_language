#include "mstate.h"
#include "mexception.h"
#include "mlib_base.h"
#include "mlib_io.h"
#include "mlib_math.h"
#include "mlib_string.h"
#include "mlib_table.h"
#include <stdio.h>

void Repl(oms::State &state)
{
    printf("Luna 2.0 Copyright (C) 2014\n");

    for (;;)
    {
        try
        {
            printf("> ");

            char s[1024] = { 0 };
            fgets(s, sizeof(s), stdin);
            state.DoString(s, "stdin");
        }
        catch (const oms::Exception &exp)
        {
            printf("%s\n", exp.What().c_str());
        }
    }
}

void ExecuteFile(const char **argv, oms::State &state)
{
    try
    {
        state.DoModule(argv[1]);
    }
    catch (const oms::OpenFileFail &exp)
    {
        printf("%s: can not open file %s\n", argv[0], exp.What().c_str());
    }
    catch (const oms::Exception &exp)
    {
        printf("%s\n", exp.What().c_str());
    }
}

int main(int argc, const char **argv)
{
    oms::State state;

    lib::base::RegisterLibBase(&state);
    lib::io::RegisterLibIO(&state);
    lib::math::RegisterLibMath(&state);
    lib::string::RegisterLibString(&state);
    lib::table::RegisterLibTable(&state);

    if (argc < 2)
    {
        Repl(state);
    }
    else
    {
        ExecuteFile(argv, state);
    }

    return 0;
}
