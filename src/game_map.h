#ifndef _GAME_MAP_H_
#define _GAME_MAP_H_

#include <stdlib.h>
#include <stdio.h>

typedef struct {
    int dim;
    int* map;
    bool initialized;
} GameMap;

int gameMap_init(GameMap* gm, const char* map_filename) {
    FILE* file = fopen(map_filename, "r");
    if (file == NULL) {
        printf("Failed to open file\n");
        return -1;
    }

    if (fscanf(file, "%d", &gm->dim) != 1) {
        printf("Failed to read map dimensions\n");
        return -1;
    }

    gm->map = (int*)malloc(gm->dim * gm->dim * sizeof(int));
    if (!gm->map) {
        printf("malloc failed\n");
        return -1;
    }

    int i = 0;

    for (int row = 0; row < gm->dim; row++) {
        for (int col = 0; col < gm->dim; col++) {
            if (fscanf(file, "%d", &gm->map[i]) != 1)
            {
                printf("Failed to read number at row %d, column %d\n", row, col);
                return -1;
            }
            i++;
        }
    }

    fclose(file);

    for (int row = 0; row < gm->dim; row++) {
        for (int col = 0; col < gm->dim; col++) {
            printf("%d ", gm->map[col + row * gm->dim]);
        }
        printf("\n");
    }

    gm->initialized = true;
    return 0;
}

#endif // _GAME_MAP_H_