#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "vadpcm.h"

void vencodeframe(FILE *ofile, s16 *inBuffer, s32 *state, s32 ***coefTable, s32 order, s32 npredictors, s32 nsam)
{
    s16 ix[16];
    s32 prediction[16];
    s32 inVector[16];
    s32 saveState[16];
    s32 optimalp;
    s32 scale;
    s32 llevel;
    s32 ulevel;
    s32 i;
    s32 j;
    s32 k;
    s32 ie[16];
    s32 nIter;
    s32 max;
    s32 cV;
    s32 maxClip;
    s8 header;
    s8 c;
    f32 e[16];
    f32 se;
    f32 min;

    for (i = nsam; i < 16; i++)
    {
        inBuffer[i] = 0;
    }

    llevel = -8;
    ulevel = -1 - llevel;
    min = 1e30;
    optimalp = 0;
    for (k = 0; k < npredictors; k++)
    {
        for (i = 0; i < order; i++)
        {
            inVector[i] = state[16 - order + i];
        }

        for (i = 0; i < 8; i++)
        {
            prediction[i] = inner_product(order + i, coefTable[k][i], inVector);
            inVector[i + order] = inBuffer[i] - prediction[i];
            e[i] = inVector[i + order];
        }

        for (i = 0; i < order; i++)
        {
            inVector[i] = prediction[8 - order + i] + inVector[i + 8];
        }

        for (i = 0; i < 8; i++)
        {
            prediction[i + 8] = inner_product(order + i, coefTable[k][i], inVector);
            inVector[i + order] = inBuffer[i + 8] - prediction[i + 8];
            e[i + 8] = (f32) inVector[i + order];
        }

        se = 0.0f;
        for (j = 0; j < 16; j++)
        {
            se += e[j] * e[j];
        }

        if (se < min)
        {
            min = se;
            optimalp = k;
        }
    }

    for (i = 0; i < order; i++)
    {
        inVector[i] = state[16 - order + i];
    }

    for (i = 0; i < 8; i++)
    {
        prediction[i] = inner_product(order + i, coefTable[optimalp][i], inVector);
        inVector[i + order] = inBuffer[i] - prediction[i];
        e[i] = (f32) inVector[i + order];
    }

    for (i = 0; i < order; i++)
    {
        inVector[i] = prediction[8 - order + i] + inVector[i + 8];
    }

    for (i = 0; i < 8; i++)
    {
        prediction[i + 8] = inner_product(order + i, coefTable[optimalp][i], inVector);
        inVector[i + order] = inBuffer[i + 8] - prediction[i + 8];
        e[i + 8] = (f32) inVector[i + order];
    }

    clamp(16, e, ie, 16);

    max = 0;
    for (i = 0; i < 16; i++)
    {
        if (fabs(ie[i]) > fabs(max))
        {
            max = ie[i];
        }
    }

    for (scale = 0; scale < 13; scale++)
    {
        if (max <= ulevel && max >= llevel)
        {
            goto out;
        }
        max /= 2;
    }
out:;

    for (i = 0; i < 16; i++)
    {
        saveState[i] = state[i];
    }

    scale--;
    nIter = 0;
    do
    {
        nIter++;
        maxClip = 0;
        scale++;
        if (scale > 12)
        {
            scale = 12;
        }

        for (i = 0; i < order; i++)
        {
            inVector[i] = saveState[16 - order + i];
        }

        for (i = 0; i < 8; i++)
        {
            prediction[i] = inner_product(order + i, coefTable[optimalp][i], inVector);
            se = (f32) inBuffer[i] - (f32) prediction[i];
            ix[i] = qsample(se, 1 << scale);
            cV = (s16) clip(ix[i], llevel, ulevel) - ix[i];
            if (maxClip < abs(cV))
            {
                maxClip = abs(cV);
            }
            ix[i] += cV;
            inVector[i + order] = ix[i] * (1 << scale);
            state[i] = prediction[i] + inVector[i + order];
        }

        for (i = 0; i < order; i++)
        {
            inVector[i] = state[8 - order + i];
        }

        for (i = 0; i < 8; i++)
        {
            prediction[i + 8] = inner_product(order + i, coefTable[optimalp][i], inVector);
            se = (f32) inBuffer[i + 8] - (f32) prediction[i + 8];
            ix[i + 8] = qsample(se, 1 << scale);
            cV = (s16) clip(ix[i + 8], llevel, ulevel) - ix[i + 8];
            if (maxClip < abs(cV))
            {
                maxClip = abs(cV);
            }
            ix[i + 8] += cV;
            inVector[i + order] = ix[i + 8] * (1 << scale);
            state[i + 8] = prediction[i + 8] + inVector[i + order];
        }
    }
    while (maxClip >= 2 && nIter < 2);

    header = (scale << 4) | (optimalp & 0xf);
    fwrite(&header, 1, 1, ofile);
    for (i = 0; i < 16; i += 2)
    {
        c = (ix[i] << 4) | (ix[i + 1] & 0xf);
        fwrite(&c, 1, 1, ofile);
    }
}
