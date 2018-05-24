#ifndef AUDIO_H
#define AUDIO_H
#define AUDIO_H_RINGBUFFER_SIZE 83160
class Audio{
  static float _RING_BUFFER[AUDIO_H_RINGBUFFER_SIZE];
  static SoundIo* _audio_soundio;
  static SoundIoDevice* _audio_device;
  static SoundIoOutStream* _audio_oustream;
  static int _audio_lock;
  static float* (*_audio_callback)(float,int);
  static int _play_head;
  static int _write_head;
  public:
  static void write_callback(struct SoundIoOutStream *_audio_oustream, int frame_count_min, int frame_count_max){
    const struct SoundIoChannelLayout *layout = &_audio_oustream->layout;
    float float_sample_rate = _audio_oustream->sample_rate;
    struct SoundIoChannelArea *areas;
    int frames_left = frame_count_max;
    int err;
    while (frames_left > 0) {
      int frame_count = frames_left;
      if ((err = soundio_outstream_begin_write(_audio_oustream, &areas, &frame_count))) exit(1);
      if (!frame_count) break;
      _audio_lock++;
      while(_audio_lock<1);
      for (int frame = 0; frame < frame_count; frame += 1) {
        for (int channel = 0; channel < layout->channel_count; channel += 1) {
          float *ptr = (float*)(areas[channel].ptr + areas[channel].step * frame);
          *ptr=_RING_BUFFER[_play_head%AUDIO_H_RINGBUFFER_SIZE];
          _play_head++;
        }
      }
      _audio_lock--;
      if ((err = soundio_outstream_end_write(_audio_oustream))) exit(1);
      frames_left -= frame_count;
    }
  }
  static void Init(float* (*callback)(float,int)){
    // --- LibSndIo Setup --- //  
    _play_head=AUDIO_H_RINGBUFFER_SIZE/2;
    _write_head=0;
    memset(_RING_BUFFER,0,AUDIO_H_RINGBUFFER_SIZE*sizeof(float));
    _audio_callback=callback;
    if(!(_audio_soundio=soundio_create())) exit(1);
    if(soundio_connect(_audio_soundio)) exit(1);
    soundio_flush_events(_audio_soundio);
    int default_out_device_index; if((default_out_device_index=soundio_default_output_device_index(_audio_soundio)<0)) exit(1);
    if(!(_audio_device=soundio_get_output_device(_audio_soundio, default_out_device_index))) exit(1);
    _audio_oustream = soundio_outstream_create(_audio_device);
    _audio_oustream->format = SoundIoFormatFloat32NE; _audio_oustream->write_callback = write_callback;
    if(soundio_outstream_open(_audio_oustream)||_audio_oustream->layout_error) exit(1);
    if(soundio_outstream_start(_audio_oustream)) exit(1);
  }
  static void Shutdown(){
    // --- LibSndIo Teardown --- //
    soundio_outstream_destroy(_audio_oustream);
    soundio_device_unref(_audio_device);
    soundio_destroy(_audio_soundio);  
  }
  static void EventLoop(){
    _audio_lock++;
    while(_audio_lock<1);
    const struct SoundIoChannelLayout *layout = &_audio_oustream->layout;
    float float_sample_rate = _audio_oustream->sample_rate;
    int channels = layout->channel_count;
    while(_write_head<_play_head){
      float* samples=_audio_callback(float_sample_rate,channels);
      for(int i=0;i<channels;i++){
        _RING_BUFFER[_write_head%AUDIO_H_RINGBUFFER_SIZE]=samples[i];
        _write_head++;
      }
    }
    _audio_lock--;
  }
};
float Audio::_RING_BUFFER[AUDIO_H_RINGBUFFER_SIZE];
int Audio::_play_head;
int Audio::_write_head;
SoundIo* Audio::_audio_soundio;
SoundIoDevice* Audio::_audio_device;
SoundIoOutStream* Audio::_audio_oustream;
float* (*Audio::_audio_callback)(float,int);
int Audio::_audio_lock;
#endif