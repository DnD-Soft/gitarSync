// playback until end of file is reached
// all parameters are hardcoded

// compile with "g++ playback.cpp -o playback -lasound"
// run with "./playback < wavfile.wav"

#include <alsa/asoundlib.h>
#include <stdio.h>

using namespace std;

int main()
{
    // Handle for the PCM device
    snd_pcm_t *pcm_handle;
	
    // Playback stream
    snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;

    // Hardware params
    snd_pcm_hw_params_t *hwparams;

    // PCM device name
    char* pcm_name;

    // initialize
    pcm_name = strdup("plughw:0,0");
    snd_pcm_hw_params_alloca(&hwparams);

    // Open the PCM device in playback mode
    if(snd_pcm_open(&pcm_handle, pcm_name, stream, 0) < 0)
    {
        fprintf(stderr, "Error opening PCM device %s\n", pcm_name);
        return -1 ;
    }

    // Initialize hwparams with full configuration space
    if(snd_pcm_hw_params_any(pcm_handle, hwparams) < 0)
    {
        fprintf(stderr, "Can not configure this PCM device.\n");
        return -1;
    }

    // Set hwparams
    unsigned int rate = 44100;              // Sample rate
    unsigned int exact_rate;                // Sample rate returned by snd_pcm_hw_params_set_rate_near 
    unsigned int channels = 2;              // Number of audio channels
    int periods = 2;                        // Number of periods
    snd_pcm_uframes_t periodsize = 8192;    // Periodsize (bytes)
    if(snd_pcm_hw_params_set_access(pcm_handle, hwparams,  SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
    {
        fprintf(stderr, "Error setting access.\n");
        return -1;
    }
        
    if(snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE) < 0)
    {
        fprintf(stderr, "Error setting format.\n");
        return -1;
    }

    exact_rate = rate;
    if(snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &exact_rate, 0) < 0)
    {
        fprintf(stderr, "Error setting rate.\n");
        return -1;
    }
    if(rate != exact_rate)
    {
        fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n Using %d Hz instead.\n", rate, exact_rate);
    }

    if(snd_pcm_hw_params_set_channels(pcm_handle, hwparams, channels) < 0)
    {
        fprintf(stderr, "Error setting channels.\n");
        return -1;
    }

    if(snd_pcm_hw_params_set_periods(pcm_handle, hwparams, periods, 0) < 0)
    {
        fprintf(stderr, "Error setting periods.\n");
        return -1;
    }

    snd_pcm_uframes_t appr_val = (periodsize * periods) >> 2;
    if(snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hwparams, &appr_val) < 0)
    {
        fprintf(stderr, "Error setting buffersize.\n");
        return -1;
    }

    // Apply HW parameter settings to PCM device and prepare device
    if(snd_pcm_hw_params(pcm_handle, hwparams) < 0)
    {
        fprintf(stderr, "Error setting HW params.\n");
        return -1;
    }

    // Playback
        
    int buff_size, frames, pcm;
    unsigned char* data;
    frames = 4096; // periods * periodsize / 4
    data = (unsigned char *)malloc(periodsize);

    while(true)
    {
        if(read(0, data, (periodsize * periods)) == 0)
        {
            printf("End of file.\n");
            snd_pcm_writei(pcm_handle, data, frames);
            return 0;
        }
        if(pcm = snd_pcm_writei(pcm_handle, data, frames) == -EPIPE)
        {
            printf("XRUN.\n");
            snd_pcm_prepare(pcm_handle);
        }
        else if(pcm < 0)
        {
            printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
        }
    }

    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    free(data);
    return 0;	
}
