#include "vadpcm.h"

s16 qsample(f32 x, s32 scale)
{
    if (x > 0.0f)
    {
        return (s16) ((x / scale) + 0.4999999);
    }
    else
    {
        return (s16) ((x / scale) - 0.4999999);
    }
}

void clamp(s32 fs, f32 *e, s32 *ie, s32 bits)
{
    s32 i;
    f32 ulevel;
    f32 llevel;

    llevel = -(f32) (1 << (bits - 1));
    ulevel = -llevel - 1.0f;
    for (i = 0; i < fs; i++)
    {
        if (e[i] > ulevel)
        {
            e[i] = ulevel;
        }
        if (e[i] < llevel)
        {
            e[i] = llevel;
        }

        if (e[i] > 0.0f)
        {
            ie[i] = (s32) (e[i] + 0.5);
        }
        else
        {
            ie[i] = (s32) (e[i] - 0.5);
        }
    }
}

s32 clip(s32 ix, s32 llevel, s32 ulevel)
{
    if (ix < llevel || ulevel < ix)
    {
        if (ix < llevel)
        {
            return llevel;
        }
        if (ulevel < ix)
        {
            return ulevel;
        }
    }
    return ix;
}
