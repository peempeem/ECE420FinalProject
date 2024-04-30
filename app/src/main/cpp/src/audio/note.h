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
            unsigned midi;
        };

        enum Key
        {
            C_MAJOR,
            G_MAJOR,
            D_MAJOR,
            A_MAJOR,
            E_MAJOR,
            B_MAJOR,
            G_FLAT_MAJOR,
            D_FLAT_MAJOR,
            A_FLAT_MAJOR,
            E_FLAT_MAJOR,
            B_FLAT_MAJOR,
            F_MAJOR
        };

        class MajorScaleIterator
        {
            public:
                MajorScaleIterator() {};
                MajorScaleIterator(const Data* note, std::vector<Data>* table);

                const Data& operator*();
                const Data* operator->();
                MajorScaleIterator& operator++();
                MajorScaleIterator& operator--();

            private:
                const Data* _note;
                std::vector<Data>* _data;
                int _idx;
        };

        MusicNote();

        const Data* fromMIDI(int midi);
        const Data* fromName(const char* name);
        const Data* fromFrequency(float frequency);
        Key transposeKey(Key key, int transpose);
        const Data* convertToKey(const Data* note, Key key);
        const char* keyToName(Key key);
        void keyedNoteName(char* buf, const Data* note, Key key);

        MajorScaleIterator beginScale(Key key, unsigned octave);

    private:
        std::vector<Data> _data;
};

static MusicNote musicNote;
