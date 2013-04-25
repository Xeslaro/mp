#include <stdio.h>
#include <string.h>
struct token {
	int	l, r;
};
#define ia(x) ((x)=='_' || ((x)>='a' && (x)<='z') || ((x)>='A' && (x)<='Z'))

int get_token(char *d, int l)
{
	if (ia(d[l])) {
		while (ia(d[l])) l++;
		return l-1;
	} else
		return l;
}
int token_type(char *d, struct token *p)
{
	char	*rw[] = {"volatile", "const", "int", "short", "float", "double",
			 "void", "char", "long", "signed", "unsigned"};
	if (!ia(d[p->l]))	return 0;
	else {
		int	k;
		for (k=0;k<sizeof(rw)/sizeof(char*);k++)
			if (strlen(rw[k]) == p->r - p->l + 1 && !memcmp(rw[k], d + p->l, p->r - p->l + 1))
				return 0;
		return 1;
	}
}
void proc_array(char *d, int l)
{
	printf(" array with size ");
	int	f=0;
	while (1) {
		while (d[l] == ' ' || d[l] == '\t') l++;
		if (!f)
			if (d[l] == '[')	f=1;
			else			break;
		else
			if (d[l] == ']')	f=0;
		putchar(d[l++]);
	}
}
void proc_func(char *d, int l)
{
	printf(" function with arg ");
	int	p=1;
	l++;
	while (1) {
		if (d[l] == '(')	p++;
		else if (d[l] == ')')	p--;
		if (!p)			break;
		putchar(d[l++]);
	}
	printf(" returning");
}
int match_parenthesis(char *d, int l)
{
	int	p=1;
	l++;
	while (1) {
		if (d[l] == '(')	p++;
		else if (d[l] == ')')	p--;
		if (!p)			break;
		l++;
	}
	return l;
}
void cdecl(struct token *stk, int top, char *d, int l)
{
	while (d[l] == ' ' || d[l] == '\t') l++;
	if (d[l] == '(')	proc_func(d, l);
	else if (d[l] == '[')	proc_array(d, l);
	while (top--)
		if (d[stk[top].l] == '*')	printf(" pointer to");
		else if (d[stk[top].l] == '(') {
			int	r=match_parenthesis(d, stk[top].l);
			cdecl(stk, top, d, r+1);
			return;
		} else {
			putchar(' ');
			int	k;
			for (k=stk[top].l;k<=stk[top].r;k++)
				putchar(d[k]);
		}
}
int main(void)
{
	char		d[256];
	struct token	stk[256];
	int		top=0, l=0;
	fgets(d, 256-1, stdin);
	while (1) {
		while (d[l] == ' ' || d[l] == '\t') l++;
		stk[top].l = l, stk[top].r = get_token(d, l);
		l=stk[top].r + 1;
		if (!token_type(d, stk+top))	top++;
		else {
			int	k;
			for (k=stk[top].l;k<=stk[top].r;k++)
				putchar(d[k]);
			printf(" is a(n)");
			cdecl(stk, top, d, l);
			putchar('\n');
			return 0;
		}
	}
}
