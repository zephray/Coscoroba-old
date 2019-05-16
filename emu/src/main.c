#include "main.h"
#include "frontend.h"

int main(int argc, char *argv[]) {
	printf("Coscoroba Emulator\nVersion %s\n", VERSION);
	frontend_init();

	while (frontend_pollevent()) {
		// ?
	}

	frontend_deinit();
}