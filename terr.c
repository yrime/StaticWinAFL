#include <stdio.h>
#include <cstdlib>

int main(){
	printf("f");
	char *b = (char*)malloc(5);
	char *f = (char*)malloc(5);
	b[0] = 0;
	f[0] = 0;
	free(b);
	free(f);
	free(b);
	printf("b %c", b[0]);
	return 0;
}