#include <stdio.h>
#include <string.h>
int main(void)
{
	FILE	*inf = fopen("trans.exp", "r");
	struct info {
		char	spec[64];
		char	manu[64];
		double	v, i, w, f, g, p;
	}infos[1024];
	char	buf[1024];
	int	k=0;
	while (fgets(buf, 1024, inf)) {
		int	len = strlen(buf), i, f=0;
		if (!len)	break;
		char	*p1=infos[k].spec, *p2=infos[k].manu;
		for (i=0;i<len;i++)
			if (buf[i]==',')
				if (!f)		f=1;
				else		break;
			else
				if (!f)		*p1++ = buf[i];
				else		*p2++ = buf[i];
		if (i == len) {
			puts("database read fault");
			return -1;
		}
		i=sscanf(buf+i+1, "%lf,%lf,%lf,%lf,%lf,%lf", &infos[k].v,
		       &infos[k].i, &infos[k].w, &infos[k].f, &infos[k].g, &infos[k].p);
		if (i != 6) {
			puts("database read fault");
			return -1;
		}
		k++;
	}
	double	v, i, w, f, g;
	puts("please input requirements:");
	scanf("%lf%lf%lf%lf%lf", &v, &i, &w, &f, &g);
	int	l;
	for (l=0;l<k;l++)
		if (infos[l].v>=v && infos[l].i>=i && infos[l].w>=w && infos[l].f>=f && infos[l].g>=g)
			printf("%s %s %.3lf\n", infos[l].spec, infos[l].manu, infos[l].p);
	return 0;
}
