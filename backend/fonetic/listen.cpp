#include <thread>
#include <mutex>
#include <string>
#include <iostream>
#include <regex>
#include <boost/circular_buffer.hpp>
#define ALSA_PCM_NEW_HW_PARAMS_API // Use the newer ALSA API
#include <alsa/asoundlib.h>
#include <wiringPi.h>
#include <stdlib.h>

const int LED0_BLUE = 15;

const int LED1_RED = 0;
const int LED1_GREEN = 3;
const int LED1_BLUE = 2;

const int LED2_RED = 6;
const int LED2_GREEN = 10;
const int LED2_BLUE = 11;

// ----------------------------------------------------------------------------
// Listener class, monitors audio and starts recording

class TListener {
    public:
        // constructor, also starts the worker thread
        TListener(const std::string &Device, int Channels, uint16_t Gate, int SpikeThreshold, int BufferSize) {
            __device = Device;
            __channels = Channels;
            __gate = Gate;
            __avg = 0;
            __avgbuffer.set_capacity(BufferSize);
            __spikeThreshold = SpikeThreshold;
            __stop = false;
            __recording = false;

            __t = new std::thread(TListener::__listen, this);
        }

        // stop the worker 
        void stop() {
            __stop = true;

            __t->join();
        }

        // return the current level of the audio range 0.0 - 1.0
        double vu() {
            const double max = 0x7fff;

            return (double)__avg / max;            
        }

        // sound detection
        bool soundDetect() {
            std::lock_guard<std::mutex> guard(__avgbuffer_mutex);

            // buffer must be full
            if (!__avgbuffer.full())
                return false;

            // loop the buffer and count the sound spikes;
            int spikes = 0;

            for (boost::circular_buffer<uint16_t>::const_iterator i = __avgbuffer.begin(); i != __avgbuffer.end(); i++) {
                if (*i > __gate)
                    spikes++;
            }

            if (spikes > __spikeThreshold)
                return true;

            return false;
        }

        // start recording
        void startRecording() {
            if (__recording)
                stopRecording();

            std::time_t now = std::time(nullptr);
            std::tm *tm = std::localtime(&now);

            // Yikes!
            char fname[256];
            sprintf(fname, "queue/in/audio-%04d%02d%02d-%02d%02d%02d.tmp", tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

            __recordingname = fname;
            __recordingfh = open(fname, O_RDWR|O_CREAT, 0666);
            __recording = true;

            std::cout << "Recording to " << fname << "\n";
        }

        // stop recording
        void stopRecording() {
            if (!__recording)
                return;
            
            __recording = false;
            close(__recordingfh);

            std::string newname = std::regex_replace(__recordingname, std::regex(".tmp"), ".raw");
            rename(__recordingname.c_str(), newname.c_str());
        }

    private:
        std::string __device;
        int __channels;
        uint16_t __gate;
        uint16_t __avg;
        int __avgbufferSize = 1024;
        boost::circular_buffer<uint16_t> __avgbuffer;
        std::thread *__t;
        std::mutex __avgbuffer_mutex;
        int __spikeThreshold;
        bool __stop;
        bool __recording;
        int __recordingfh;
        std::string __recordingname;

        static void __listen(TListener *me) {
            int64_t loops;
            int32_t rc;
            int32_t size;
            snd_pcm_t *handle;
            snd_pcm_hw_params_t *params;
            uint32_t val;
            int32_t dir;
            snd_pcm_uframes_t frames;
            uint8_t *buffer;

            /* Open PCM device for recording (capture). */
            rc = snd_pcm_open(&handle, me->__device.c_str(), SND_PCM_STREAM_CAPTURE, 0);

            if (rc < 0) {
                std::cerr << "Unable to open pcm device:" <<  snd_strerror(rc) << "\n";
                exit(-1);
            }

            /* Allocate a hardware parameters object. */
            snd_pcm_hw_params_alloca(&params);

            /* Fill it in with default values. */
            snd_pcm_hw_params_any(handle, params);

            /* Set the desired hardware parameters. */
            /* Interleaved mode */
            snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

            /* Signed 16-bit little-endian format */
            snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

            /* One channel (mono) or two channels */
            snd_pcm_hw_params_set_channels(handle, params, me->__channels);                                 

            /* 48000 bits/second sampling rate */
            val = 48000;                                                     
            snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

            /* Set period size */
            frames = 128;
            snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

            /* Write the parameters to the driver */
            rc = snd_pcm_hw_params(handle, params);

            if (rc < 0) {
                std::cerr << "unable to set hw parameters: " << snd_strerror(rc) << "\n";
                exit(-1);
            }

            /* Use a buffer large enough to hold one period */
            snd_pcm_hw_params_get_period_size(params, &frames, &dir);
            size = frames * 2 * me->__channels;  /* 2 bytes/sample, 1 or 2 channels */
            buffer = (uint8_t *)malloc(size);

            std::cout << "Listening thread started\n";

            digitalWrite(LED2_RED, HIGH);
            digitalWrite(LED2_GREEN, LOW);

            while (!me->__stop) {
                rc = snd_pcm_readi(handle, buffer, frames);

                if (rc == -EPIPE) {
                    /* EPIPE means overrun */
                    fprintf(stderr, "overrun occurred\n");
                    snd_pcm_prepare(handle);
                } 
                else 
                if (rc < 0) {
                    std::cerr << "error from read:" << snd_strerror(rc) << "\n";
                    exit(-1);
                }
                else 
                if (rc != (int)frames) {
                    std::cerr << "short read, read " << rc << " frames\n";
                    exit(-1);
                }

                if (me->__recording) {
                    rc = write(me->__recordingfh, buffer, size);

                    if (rc != size) {
                        std::cerr << "short write: wrote " << rc << " bytes\n";
                        exit(-1);
                    }
                }

                /* calculate the average of this buffer */                
                int32_t sum = 0;
                int16_t *p = (int16_t *)buffer;

                for (int i = 0; i < frames; i++) {
                    int16_t sample = *p;

                    p += me->__channels;

                    /* we want absolute values */
                    if (sample < 0)
                        sample = -sample;

                    /* sum the all samples in the buffer */
                    sum += sample;
                }

                uint16_t avg = sum / frames;

                // std::cout << avg << "\n";

                me->__avg = avg;

                // protect and push
                {
                    std::lock_guard<std::mutex> guard(me->__avgbuffer_mutex);
                    me->__avgbuffer.push_back(avg);
                }
            }

            snd_pcm_drain(handle);
            snd_pcm_close(handle);
            free(buffer);
        }
};

// ----------------------------------------------------------------------------
int main(void) {
    // Status leds initialization
    wiringPiSetup() ;

    // LED1 is common cathode

    // pinMode(LED1_RED, OUTPUT);
    // pinMode(LED1_GREEN, OUTPUT);
    // pinMode(LED1_BLUE, OUTPUT);

    // digitalWrite(LED1_RED, HIGH);
    // digitalWrite(LED1_GREEN, HIGH);
    // digitalWrite(LED1_BLUE, HIGH);

    pinMode(LED2_RED, OUTPUT);
    pinMode(LED2_GREEN, OUTPUT);
    pinMode(LED2_BLUE, OUTPUT);

    // LED2 is common anode

    digitalWrite(LED2_RED, HIGH);
    digitalWrite(LED2_GREEN, HIGH);
    digitalWrite(LED2_BLUE, HIGH);

    // default settings
    std::string device = "hw:Device";
    int channels = 1;
    int gate = 4096;
    int spikes = 15;
    int buffer = 512;

    const char *env;
    env = getenv("FONETIC_DEVICE");

    if (env != nullptr)
        device = env;

    env = getenv("FONETIC_CHANNELS");

    if (env != nullptr)
        channels = atoi(env);

    env = getenv("FONETIC_GATE");

    if (env != nullptr)
        gate = atoi(env);

    env = getenv("FONETIC_SPIKES");

    if (env != nullptr)
        spikes = atoi(env);

    env = getenv("FONETIC_BUFFER");

    if (env != nullptr)
        buffer = atoi(env);

    std::cout 
        << "Starting Fonetic listener on device: " << device
        << "," << channels << " channels"
        << ", gate=" << gate
        << ", spikes=" << spikes
        << ", buffer=" << buffer
        << "\n";

    digitalWrite(LED2_RED, LOW);

    TListener listener(device, channels, gate, spikes, buffer);

    bool previousDetection = false;

    for (;;) {
        bool detection = listener.soundDetect();

        if (previousDetection != detection) {
            previousDetection = detection;

            if (detection) {
                digitalWrite(LED2_RED, LOW);
                digitalWrite(LED2_GREEN, HIGH);
                digitalWrite(LED2_BLUE, HIGH);

                std::cout << "Recording\n";
                listener.startRecording();
            }
            else {
                digitalWrite(LED2_RED, HIGH);
                digitalWrite(LED2_GREEN, LOW);
                digitalWrite(LED2_BLUE, HIGH);

                std::cout << "Stopped recording\n";
                listener.stopRecording();
            }
        }
    }

    // never gets here
    // listener.stop();
}
