#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef void (*lp_handler_t)(char *path);

/* used to initialize and free the traverser resources */
bool lp_traverser_init(uint32_t threads, char **target, char **ignore);
void lp_traverser_free(void);

/* configuration for the traverser */
void lp_traverser_set_mode(int mode);
void lp_traverser_set_handler(lp_handler_t handler);

/* other operations */
void lp_traverser_wait(bool display);
bool lp_traverser_run(char *path);
