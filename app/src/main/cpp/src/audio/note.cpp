#include "note.h"
#include <math.h>
#include "../util/log.h"

MusicNote::MusicNote()
{
    const static char name[12] = {'C', 'D', 'D', 'E', 'E', 'F', 'G', 'G', 'A', 'A', 'B', 'B'};
    const static bool flat[12] = {false, true, false, true, false, false, true, false, true, false, true, false};

    _data.resize(108);

    for (unsigned i = 0; i < _data.size(); ++i)
    {
        if (i < 57)
            _data[i].frequency = 440 / powf(2, (57 - i) / 12.0f);
        else
            _data[i].frequency = 440 * powf(2, (i - 57) / 12.0f);

        unsigned idx = i % 12;
        _data[i].name[0] = name[idx];
        if (flat[idx])
        {
            _data[i].name[1] = 'b';
            _data[i].name[2] = '0' + i / 12;
            _data[i].name[3] = '\0';
        }
        else
        {
            _data[i].name[1] = '0' + i / 12;
            _data[i].name[2] = '\0';
        }
    }
}

const MusicNote::Data* MusicNote::fromMIDI(int midi)
{
    if (midi < 12 || midi > 119)
        return NULL;
    return &_data[midi - 12];
}

const MusicNote::Data* MusicNote::fromName(const char* name)
{
    unsigned midi;
    switch (name[0])
    {
        case 'c':
        case 'C':
            midi = 0;
            break;

        case 'd':
        case 'D':
            midi = 2;
            break;

        case 'e':
        case 'E':
            midi = 4;
            break;

        case 'f':
        case 'F':
            midi = 5;
            break;

        case 'g':
        case 'G':
            midi = 7;
            break;

        case 'a':
        case 'A':
            midi = 9;
            break;

        case 'b':
        case 'B':
            midi = 11;
            break;

        default:
            return NULL;
    }

    bool modifier;
    switch (name[1])
    {
        case 'b':
            midi--;
            modifier = true;
            break;

        case '#':
            midi++;
            modifier = true;
            break;

        default:
            modifier = false;
    }

    midi += ((unsigned) (name[modifier ? 2 : 1] - '0') + 1) * 12;
    return fromMIDI(midi);
}

const MusicNote::Data* MusicNote::fromFrequency(float frequency)
{
    unsigned midi = 12 * log2f(frequency / 440.0f) + 69;
    return fromMIDI(roundf(midi));
}