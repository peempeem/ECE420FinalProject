#pragma once

#include <vector>
#include <string>

class MusicNote
{
    public:
        struct Data
        {
            char name[4];
            float frequency;
        };

        MusicNote();

        const Data* fromMIDI(int midi);
        const Data* fromName(const char* name);
        const Data* fromFrequency(float frequency);

    private:
        std::vector<Data> _data;
};

static MusicNote musicNote;
