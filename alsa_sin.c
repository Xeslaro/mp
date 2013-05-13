#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include "alsa_common.h"
#include "common.h"
#define device_name "default"
#define format SND_PCM_FORMAT_S16_LE
#define channels 2
#define rate 44100
#define period_time 100000
#define buffer_time 400000
#define sin_freq 441
#define DEBUG

snd_pcm_uframes_t	period_size, buffer_size;

int write_loop(snd_pcm_t *snd_pcm_p)
{
	double	x = 0;
	int	first = 1, ans;
	while (1) {
		int	pcm_state = snd_pcm_state(snd_pcm_p);
		if (pcm_state == SND_PCM_STATE_XRUN)
			(first = 1, !xrun_recover(snd_pcm_p, -EPIPE)) || pae("can't recover");
		else if (pcm_state == SND_PCM_STATE_SUSPENDED)
			!xrun_recover(snd_pcm_p, -ESTRPIPE) || pae("can't recover");
		int	frame_avail = snd_pcm_avail_update(snd_pcm_p);
		#ifdef DEBUG
		printf("snd_pcm_avail_update returned: %d\n", frame_avail);
		#endif
		unless (frame_avail < period_size) {
			int	size = period_size;
			while (size) {
				const snd_pcm_channel_area_t	*areas;
				snd_pcm_uframes_t		offset, frames = size;
				ans = snd_pcm_mmap_begin(snd_pcm_p, &areas, &offset, &frames);
				!ans || !xrun_recover(snd_pcm_p, ans) || pae("snd_pcm_mmap_begin error");
				gen_sin(areas, offset, frames, &x);
				ans = snd_pcm_mmap_commit(snd_pcm_p, offset, frames);
				#ifdef DEBUG
				printf("snd_pcm_mmap_commit returned: %d\n", ans);
				#endif
				(ans < 0 || ans != frames) && (!xrun_recover(snd_pcm_p, ans < 0 ? ans : -EPIPE) || pae("snd_pcm_mmap_commit error"));
				size -= ans;
			}
		} else if (frame_avail < 0)
			(first = 1, !xrun_recover(snd_pcm_p, frame_avail)) || pae("can't recover");
		else
			if (first) {
				first = 0;
				#ifdef DEBUG
				puts("going to call snd_pcm_start");
				#endif
				alsa_ok0(snd_pcm_start(snd_pcm_p), "snd_pcm_start error");
			} else {
				#ifdef DEBUG
				puts("going to call snd_pcm_wait");
				#endif
				ans = snd_pcm_wait(snd_pcm_p, -1);
				if (ans < 0)
					!xrun_recover(snd_pcm_p, ans) || pae("recover failed");
			}
	}
}
int xrun_recover(snd_pcm_t *snd_pcm_p, int err_code)
{
	#ifdef DEBUG
	puts("xrun_recover called");
	#endif
	if (err_code == -EPIPE) {
		alsa_ok0(snd_pcm_prepare(snd_pcm_p), "snd_pcm_prepare error");
		return 0;
	} else if (err_code == -ESTRPIPE) {
		int	ans;
		while ((ans = snd_pcm_resume(snd_pcm_p)) == -EAGAIN)	sleep(1);
		if (ans < 0)	alsa_ok0(snd_pcm_prepare(snd_pcm_p), "snd_pcm_prepare error");
		return 0;
	}
	return 1;
}
int gen_sin(const snd_pcm_channel_area_t *areas, int offset, int frames, double *x)
{
	double	sample_diff_in_x = M_PI * 2 * sin_freq / rate;
	int	step = areas->step, max_val = 1 << (step - 1);
	short*	channel_addr[2];
	channel_addr[0] = (short*)((char*)(areas+0)->addr + (areas+0)->first/8 + offset * step / 8);
	channel_addr[1] = (short*)((char*)(areas+1)->addr + (areas+1)->first/8 + offset * step / 8);
	while (frames--) {
		double	sin_ans = sin(*x);
		int	act_val_to_write;
		if (sin_ans < 0)	act_val_to_write = sin_ans * max_val;
		else			act_val_to_write = sin_ans * (max_val - 1);
		*channel_addr[1]++ = *channel_addr[0]++ = act_val_to_write;
		*x += sample_diff_in_x;
		if (*x > M_PI * 2) *x -= M_PI * 2;
	}
}
int set_hw_params(snd_pcm_t *snd_pcm_p)
{
	snd_pcm_hw_params_t	*hw_params_p;
	alsa_ok0(snd_pcm_hw_params_malloc(&hw_params_p), "snd_pcm_hw_params_malloc error");
	alsa_ok0(snd_pcm_hw_params_any(snd_pcm_p, hw_params_p), "snd_pcm_hw_params_any error");
	alsa_ok0(snd_pcm_hw_params_set_rate_resample(snd_pcm_p, hw_params_p, 1), "snd_pcm_hw_params_set_rate_resample error");
	alsa_ok0(snd_pcm_hw_params_set_access(snd_pcm_p, hw_params_p, SND_PCM_ACCESS_MMAP_NONINTERLEAVED), "snd_pcm_hw_params_set_access error");
	alsa_ok0(snd_pcm_hw_params_set_format(snd_pcm_p, hw_params_p, format), "snd_pcm_hw_params_set_format error");
	alsa_ok0(snd_pcm_hw_params_set_channels(snd_pcm_p, hw_params_p, channels), "snd_pcm_hw_params_set_channels error");
	int	direction, val;
	val = rate;
	alsa_ok0(snd_pcm_hw_params_set_rate_near(snd_pcm_p, hw_params_p, &val, &direction), "snd_pcm_hw_params_set_rate_near error");
	#ifdef DEBUG
	printf("returned rate: %d\n", val);
	#endif
	val = period_time;
	alsa_ok0(snd_pcm_hw_params_set_period_time_near(snd_pcm_p, hw_params_p, &val, &direction), "snd_pcm_hw_params_set_period_time_near error");
	#ifdef DEBUG
	printf("returned period_time: %d\n", val);
	#endif
	alsa_ok0(snd_pcm_hw_params_get_period_size(hw_params_p, &period_size, &direction), "snd_pcm_hw_params_get_period_size error");
	val = buffer_time;
	alsa_ok0(snd_pcm_hw_params_set_buffer_time_near(snd_pcm_p, hw_params_p, &val, &direction), "snd_pcm_hw_params_set_buffer_time_near error");
	alsa_ok0(snd_pcm_hw_params_get_buffer_size(hw_params_p, &buffer_size), "snd_pcm_hw_params_get_buffer_size error");
	#ifdef DEBUG
	printf("period size: %d buffer size: %d\n", period_size, buffer_size);
	#endif
	alsa_ok0(snd_pcm_hw_params(snd_pcm_p, hw_params_p), "snd_pcm_hw_params error");
}
int set_sw_params(snd_pcm_t *snd_pcm_p)
{
	snd_pcm_sw_params_t	*sw_params_p;
	alsa_ok0(snd_pcm_sw_params_malloc(&sw_params_p), "snd_pcm_sw_params_malloc error");
	alsa_ok0(snd_pcm_sw_params_current(snd_pcm_p, sw_params_p), "snd_pcm_sw_params_current error");
	alsa_ok0(snd_pcm_sw_params_set_start_threshold(snd_pcm_p, sw_params_p, buffer_size / period_size * period_size), "snd_pcm_sw_params_set_start_threshold error");
	alsa_ok0(snd_pcm_sw_params_set_avail_min(snd_pcm_p, sw_params_p, period_size), "snd_pcm_sw_params_set_avail_min error");
}
int main(void)
{
	snd_pcm_t	*snd_pcm_p;
	alsa_ok0(snd_pcm_open(&snd_pcm_p, device_name, SND_PCM_STREAM_PLAYBACK, 0), "snd_pcm_open error");
	set_hw_params(snd_pcm_p);
	set_sw_params(snd_pcm_p);
	write_loop(snd_pcm_p);
}
