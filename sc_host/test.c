#include <options.h>

int main(int argc, char** argv)
{
    struct option_args_t args;
    return parse_args(argc, argv, &args);
}
