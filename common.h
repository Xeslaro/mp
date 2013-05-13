#define errn1(r, m) if ((r)==-1) {\
	perror(m);\
	exit(-1);\
}
#define ok0(r, m) if (r) {\
	perror(m);\
	exit(-1);\
}
#define rev2(x) ((((x)&0xff)<<8) + ((x)>>8&0xff))
#define unless(c) if (!(c))
#define pae(s) (puts(s), exit_wrapper(-1))
