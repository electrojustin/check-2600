#include <stdio.h>

int main() {
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 16; j++) {
			printf("\033[48;5;%dm \033[m", i << 8 | j);
		}
		printf("\n");
	}

	return 0;
}
