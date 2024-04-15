#include "audio.h"
#include <aaudio/AAudio.h>
#include <mutex>
#include "../../kiss_fft/kiss_fft.h"
#include "../util/log.h"
#include "note.h"

#define AUDIO_SAMPLING_RATE 48000
#define AUDIO_SAMPLES_PER_BUFFER 1024 * 8
#define KFFT_SIZE AUDIO_SAMPLES_PER_BUFFER * 2
#define PEAK_VALUES 3

unsigned audioInSize = 0;
float audioIn[AUDIO_SAMPLES_PER_BUFFER];
kiss_fft_cfg kissFFTConfig;
kiss_fft_cfg kissIFFTConfig;
float noiseThreshold = 1e2 * AUDIO_SAMPLES_PER_BUFFER;
AAudioStream* audioStream = NULL;
std::mutex fftMtx;
std::mutex autoCorrMtx;
kiss_fft_cpx buf1[KFFT_SIZE];
kiss_fft_cpx fftBuf[KFFT_SIZE];
kiss_fft_cpx autoCorrBuf[KFFT_SIZE];

typedef aaudio_data_callback_result_t(*AAudioStream_dataCallback)(
        AAudioStream *stream,
        void *userData,
        void *audioData,
        int32_t numFrames);

void processBuffer()
{
    float energy = 0;
    for (unsigned i = 0; i < AUDIO_SAMPLES_PER_BUFFER; ++i)
        energy += fabsf(audioIn[i]);

    if (energy < noiseThreshold)
        return;

    // copy audio data into buf1
    for (unsigned i = 0; i < AUDIO_SAMPLES_PER_BUFFER; ++i)
        buf1[i] = kiss_fft_cpx{ .r=audioIn[i], .i=0 };

    // zero pad
    for (unsigned i = AUDIO_SAMPLES_PER_BUFFER; i < KFFT_SIZE; ++i)
        buf1[i] = kiss_fft_cpx{ .r=0, .i=0 };

    // fftBuf = fft(inBuf)
    fftMtx.lock();
    kiss_fft(kissFFTConfig, buf1, fftBuf);
    fftMtx.unlock();

    // buf1 = fftBuf * conj(fftBuf)
    for (unsigned i = 0; i < KFFT_SIZE; ++i)
        buf1[i] = kiss_fft_cpx{ .r=fftBuf[i].r * fftBuf[i].r, .i=fftBuf[i].i * -fftBuf[i].i };

    // buf2 = ifft(buf1)
    autoCorrMtx.lock();
    kiss_fft(kissIFFTConfig, buf1, autoCorrBuf);

    // use peak detection algorithm
    int peak = -1;
    for (unsigned i = PEAK_VALUES; i < KFFT_SIZE / 2; ++i)
    {
        bool isMax = true;
        for (unsigned j = 1; j <= PEAK_VALUES; ++j) {
            if (autoCorrBuf[i].r <= autoCorrBuf[i - j].r || autoCorrBuf[i].r <= autoCorrBuf[i + j].r)
            {
                isMax = false;
                break;
            }
        }

        if (isMax)
        {
            if (peak == -1)
                peak = i;
            else if (autoCorrBuf[i].r > autoCorrBuf[peak].r)
                peak = i;
        }
    }
    autoCorrMtx.unlock();

    float frequency = AUDIO_SAMPLING_RATE / (float) peak;
    const MusicNote::Data* note = musicNote.fromFrequency(frequency);
    if (!note)
        return;
    LOGD(TAG, "%d: %f, %s, %f", peak, frequency, note->name, note->frequency);
}

aaudio_data_callback_result_t inputAudioDataCallback(
        AAudioStream* stream,
        void* userData,
        void* audioData,
        int32_t numFrames)
{
    for (unsigned i = 0; i < numFrames; ++i)
    {
        audioIn[audioInSize++] = ((int16_t *) audioData)[i];
        if (audioInSize == AUDIO_SAMPLES_PER_BUFFER)
        {
            processBuffer();
            audioInSize = 0;
        }
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

bool AudioAnalyzer::init()
{
    AAudioStreamBuilder* builder;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK)
    {
        LOGE(TAG, "AAudio_createStreamBuilder returned error (%d)", result);
        return false;
    }

    AAudioStreamBuilder_setDeviceId(builder, AAUDIO_UNSPECIFIED);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setSampleRate(builder, AUDIO_SAMPLING_RATE);
    AAudioStreamBuilder_setChannelCount(builder, AAUDIO_CHANNEL_MONO);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_POWER_SAVING);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, AAUDIO_UNSPECIFIED);
    AAudioStreamBuilder_setDataCallback(builder, inputAudioDataCallback, NULL);

    if (audioStream)
        AAudioStream_close(audioStream);

    result = AAudioStreamBuilder_openStream(builder, &audioStream);
    AAudioStreamBuilder_delete(builder);

    kissFFTConfig = kiss_fft_alloc(KFFT_SIZE, false, NULL, NULL);
    kissIFFTConfig = kiss_fft_alloc(KFFT_SIZE, true, NULL, NULL);

    if (result != AAUDIO_OK)
    {
        LOGE(TAG, "AAudioStreamBuilder_openStream returned error (%d)", result);
        return false;
    }

    result = AAudioStream_requestStart(audioStream);
    if (result != AAUDIO_OK)
    {
        LOGE(TAG, "AAudioStream_requestStart returned error (%d)", result);
        return false;
    }

    return true;
}

void AudioAnalyzer::pause()
{
    deinit();
}

void AudioAnalyzer::resume()
{
    init();
}

void AudioAnalyzer::deinit()
{
    if (audioStream)
    {
        AAudioStream_close(audioStream);
        audioStream = NULL;
        kiss_fft_free(kissFFTConfig);
        kiss_fft_free(kissIFFTConfig);
    }
}

void AudioAnalyzer::getFFT(std::vector<float>& fft)
{
    fft.reserve(KFFT_SIZE);
    fftMtx.lock();
    for (unsigned i = 0; i < KFFT_SIZE; ++i)
        fft.emplace_back(fftBuf[i].r);
    fftMtx.unlock();
}

void AudioAnalyzer::getAutoCorrelation(std::vector<float>& autoCorr)
{
    autoCorr.reserve(KFFT_SIZE);
    autoCorrMtx.lock();
    for (unsigned i = 0; i < KFFT_SIZE; ++i)
        autoCorr.emplace_back(autoCorrBuf[i].r);
    autoCorrMtx.unlock();
}
