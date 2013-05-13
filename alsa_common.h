#define alsa_ok0(r, m) {\
	int	ans = r;\
	if (ans) {\
		printf("%s: %s\n", m, snd_strerror(ans));\
		exit(ans);\
	}\
}
