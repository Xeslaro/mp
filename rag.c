#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
int main(void)
{
	char	*s = "abcdefghijklmnopqrstuvwxyz1234567890";
	int	u_len = 10, p_len = 10, i, s_len = strlen(s);
	srand(time(NULL));
	putchar(s[rand()%26]);
	for (i=1;i<u_len;i++)
		putchar(s[rand()%s_len]);
	putchar(' ');
	for (i=0;i<p_len;i++)
		putchar(s[rand()%s_len]);
	putchar('\n');
	return 0;
}
