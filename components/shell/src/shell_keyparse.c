#include "shell_keyparse.h"

const char *shell_keyparse_ss3_name(char final_byte)
{
    switch (final_byte)
    {
    case 'A':
        return "up";
    case 'B':
        return "down";
    case 'C':
        return "right";
    case 'D':
        return "left";
    case 'H':
        return "home";
    case 'F':
        return "end";
    case 'P':
        return "f1";
    case 'Q':
        return "f2";
    case 'R':
        return "f3";
    case 'S':
        return "f4";
    default:
        return NULL;
    }
}

const char *shell_keyparse_csi_name(int param, char final_byte)
{
    switch (final_byte)
    {
    case 'A':
        return "up";
    case 'B':
        return "down";
    case 'C':
        return "right";
    case 'D':
        return "left";
    case 'H':
        return "home";
    case 'F':
        return "end";
    case '~':
        switch (param)
        {
        case 1:
        case 7:
            return "home";
        case 2:
            return "insert";
        case 3:
            return "delete";
        case 4:
        case 8:
            return "end";
        case 5:
            return "pageup";
        case 6:
            return "pagedown";
        case 11:
            return "f1";
        case 12:
            return "f2";
        case 13:
            return "f3";
        case 14:
            return "f4";
        case 15:
            return "f5";
        case 17:
            return "f6";
        case 18:
            return "f7";
        case 19:
            return "f8";
        case 20:
            return "f9";
        case 21:
            return "f10";
        case 23:
            return "f11";
        case 24:
            return "f12";
        default:
            return NULL;
        }
    default:
        return NULL;
    }
}
