#include "note.h"
#include <math.h>
#include <sstream>
#include "../util/log.h"

MusicNote::MajorScaleIterator::MajorScaleIterator(const Data* note, std::vector<Data>* table) : _note(note), _data(table), _idx(0)
{

}

const MusicNote::Data& MusicNote::MajorScaleIterator::operator*()
{
    return *_note;
}

const MusicNote::Data* MusicNote::MajorScaleIterator::operator->()
{
    return _note;
}

MusicNote::MajorScaleIterator& MusicNote::MajorScaleIterator::operator++()
{
    if (_idx == 2 || _idx == 6)
        _note = &(*_data)[_note->midi + 1 - 12];
    else
        _note = &(*_data)[_note->midi + 2 - 12];
    _idx = (_idx + 1) % 7;
    return *this;
}

MusicNote::MajorScaleIterator& MusicNote::MajorScaleIterator::operator--()
{
    if (_idx == 0 || _idx == 3)
        _note = &(*_data)[_note->midi - 1 - 12];
    else
        _note = &(*_data)[_note->midi - 2 - 12];

    --_idx;
    if (_idx < 0)
        _idx += 7;
    return *this;
}

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

        _data[i].midi = i + 12;

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
    if (strlen(name) < 2)
        return NULL;

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

MusicNote::Key MusicNote::transposeKey(Key key, int transpose)
{
    int newKey = (((int) key) - transpose * 5);
    if (newKey >= 0)
        return (Key) (newKey % 12);
    return (Key) (12 + newKey % 12);
}

const MusicNote::Data* MusicNote::convertToKey(const Data* note, Key key)
{
    if (note->name[1] < '0' || note->name[1] > '9')
        return NULL;

    switch (key)
    {
        case C_MAJOR:
            return note;

        case G_MAJOR:
            switch (note->name[0])
            {
                case 'F':
                    return fromMIDI(note->midi + 1);

                default:
                    return note;
            }

        case D_MAJOR:
            switch(note->name[0])
            {
                case 'F':
                case 'C':
                    return fromMIDI(note->midi + 1);

                default:
                    return note;
            }

        case A_MAJOR:
            switch(note->name[0])
            {
                case 'C':
                case 'F':
                case 'G':
                    return fromMIDI(note->midi + 1);

                default:
                    return note;
            }

        case E_MAJOR:
            switch(note->name[0])
            {
                case 'F':
                case 'G':
                case 'C':
                case 'D':
                    return fromMIDI(note->midi + 1);

                default:
                    return note;
            }

        case B_MAJOR:
            switch(note->name[0])
            {
                case 'C':
                case 'D':
                case 'F':
                case 'G':
                case 'A':
                    return fromMIDI(note->midi + 1);

                default:
                    return note;
            }

        case G_FLAT_MAJOR:
            switch(note->name[0])
            {
                case 'G':
                case 'A':
                case 'B':
                case 'C':
                case 'D':
                case 'E':
                    return fromMIDI(note->midi - 1);

                default:
                    return note;
            }

        case D_FLAT_MAJOR:
            switch(note->name[0])
            {
                case 'D':
                case 'E':
                case 'G':
                case 'A':
                case 'B':
                    return fromMIDI(note->midi - 1);

                default:
                    return note;
            }

        case A_FLAT_MAJOR:
            switch(note->name[0])
            {
                case 'A':
                case 'B':
                case 'D':
                case 'E':
                    return fromMIDI(note->midi - 1);

                default:
                    return note;
            }

        case E_FLAT_MAJOR:
            switch(note->name[0])
            {
                case 'E':
                case 'A':
                case 'B':
                    return fromMIDI(note->midi - 1);

                default:
                    return note;
            }

        case B_FLAT_MAJOR:
            switch(note->name[0])
            {
                case 'B':
                case 'E':
                    return fromMIDI(note->midi - 1);

                default:
                    return note;
            }

        case F_MAJOR:
            switch(note->name[0])
            {
                case 'B':
                    return fromMIDI(note->midi - 1);

                default:
                    return note;
            }
    }
}

const char* MusicNote::keyToName(Key key)
{
    switch (key)
    {
        case C_MAJOR:
            return "C Major";

        case G_MAJOR:
            return "G Major";

        case D_MAJOR:
            return "D Major";

        case A_MAJOR:
            return "A Major";

        case E_MAJOR:
            return "E Major";

        case B_MAJOR:
            return "B Major";

        case G_FLAT_MAJOR:
            return "Gb Major";

        case D_FLAT_MAJOR:
            return "Db Major";

        case A_FLAT_MAJOR:
            return "Ab Major";

        case E_FLAT_MAJOR:
            return "Eb Major";

        case B_FLAT_MAJOR:
            return "Bb Major";

        case F_MAJOR:
            return "F Major";

        default:
            return "Unknown Key";
    }
}

void MusicNote::keyedNoteName(char* buf, const Data* note, Key key)
{
    strcpy(buf, note->name);
    if (key <= B_MAJOR && buf[1] == 'b')
    {
        buf[1] = '#';

        switch (buf[0])
        {
            case 'D':
                buf[0] = 'C';
                break;

            case 'E':
                buf[0] = 'D';
                break;

            case 'G':
                buf[0] = 'F';
                break;

            case 'A':
                buf[0] = 'G';
                break;

            case 'B':
                buf[0] = 'A';
                break;
        }
    }
}

MusicNote::MajorScaleIterator MusicNote::beginScale(Key key, unsigned octave)
{
    std::stringstream ss;
    switch (key)
    {
        case C_MAJOR:
            ss << "C" << octave;
            break;

        case G_MAJOR:
            ss << "G" << octave;
            break;

        case D_MAJOR:
            ss << "D" << octave;
            break;

        case A_MAJOR:
            ss << "A" << octave;
            break;

        case E_MAJOR:
            ss << "E" << octave;
            break;

        case B_MAJOR:
            ss << "B" << octave;
            break;

        case G_FLAT_MAJOR:
            ss << "Gb" << octave;
            break;

        case D_FLAT_MAJOR:
            ss << "Db" << octave;
            break;

        case A_FLAT_MAJOR:
            ss << "Ab" << octave;
            break;

        case E_FLAT_MAJOR:
            ss << "Eb" << octave;
            break;

        case B_FLAT_MAJOR:
            ss << "Bb" << octave;
            break;

        case F_MAJOR:
            ss << "F" << octave;
            break;
    }

    return MajorScaleIterator(fromName(ss.str().c_str()), &_data);
}