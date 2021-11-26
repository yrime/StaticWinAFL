#include <stdio.h>
#include <cstdlib>


double fact(double n) {
    if (n <= 0) return 1;
    else return n * fact(n - 1);
}

int main(){
	printf("f");
	char *b = (char*)malloc(5);
	char *f = (char*)malloc(5);
	b[0] = 52;
	f[0] = 0;
	free(b);
	free(f);
	free(b);
		printf("fact(5) = %g\n", fact(5));
		printf("fact(10) = %g\n", fact(10));
		printf("fact(100) = %g\n", fact(100));
		printf("fact(1000) = %g\n", fact(1000));
		printf("fact(1000000) = %g\n", fact(1000000));
	printf("b %c", b[0]);
	return 0;
}