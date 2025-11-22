#include <stdio.h>
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <assert.h>

#define WIDTH 900
#define HEIGHT 600

#define CELL_SIZE 30
#define ROWS HEIGHT/CELL_SIZE
#define COLUMNS WIDTH/CELL_SIZE
#define LINE_WIDTH 2

#define COLOR_BLACK 0x00000000
#define COLOR_GRID 0x1f1f1f1f
#define COLOR_WHITE 0xffffffff
#define COLOR_APPLE 0x00ff0000

#define SNAKE(x,y) fill_cell(psurface, x, y, COLOR_WHITE)
#define APPLE(x,y) fill_cell(psurface, x, y, COLOR_APPLE)
#define DRAW_GRID draw_grid(psurface)

struct SnakeElement
{
    int x,y;

    /* Can be NULL (in case of last element) */
    struct SnakeElement *pnext;

};

struct Direction
{
    int dx, dy;
};

struct Apple
{
    int x,y;
};

int choose_game_mode()
{
    int mode = 0;

    printf("Choose Game Mode:\n");
    printf("1. Single Player\n");
    printf("2. Two Player\n");
    printf("Enter choice (1 or 2): ");

    scanf("%d", &mode);

    if(mode != 1 && mode != 2)
    {
        printf("Invalid choice! Single Player (Default)");
        mode = 1;
    }

    return mode;
}

int load_high_score(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
        return 0; /* high score = 0 */

    int score = 0;
    fscanf(fp, "%d", &score);
    fclose(fp);

    return score;
}

void save_high_score(const char *filename, int score)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("Error opening file for writing!\n");
        return;
    }

    fprintf(fp, "%d", score);
    fclose(fp);
}

int draw_grid(SDL_Surface *psurface)
{
    SDL_Rect row_line = {0,0,900,LINE_WIDTH};
    for(row_line.y = 0; row_line.y < HEIGHT; row_line.y += CELL_SIZE)
    SDL_FillSurfaceRect(psurface, &row_line, COLOR_GRID);
    
    SDL_Rect col_line = {0,0,LINE_WIDTH,HEIGHT};
    for(col_line.y = 0, col_line.x = 0; col_line.x < WIDTH; col_line.x += CELL_SIZE)
        SDL_FillSurfaceRect(psurface, &col_line, COLOR_GRID);
    return 0;
}

void fill_cell(SDL_Surface *psurface, int x, int y, Uint32 color)
{
    SDL_Rect rect = {x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE, CELL_SIZE};

    SDL_FillSurfaceRect(psurface, &rect, color);
}

size_t snake_size(struct SnakeElement **ppsnake)
{
    assert(ppsnake != NULL);
    assert(*ppsnake != NULL);

    size_t list_size = 1;
    struct SnakeElement *current = *ppsnake;
    while (current->pnext != NULL)
    {
       current = current->pnext;
       list_size++;
    }
    return list_size;
    
}
void draw_snake(SDL_Surface *psurface, struct SnakeElement **ppsnake)
{
    assert(psurface != NULL);
    assert(ppsnake != NULL);
    assert(*ppsnake != NULL);
    
    struct SnakeElement *psnake = *ppsnake;
    while (psnake != NULL)
    {
        SNAKE(psnake->x, psnake->y);
        psnake = psnake->pnext;
    }
}

void print_snake(struct SnakeElement **ppsnake)
{
    struct SnakeElement *psnake = *ppsnake;
    int counter = 0;
    while (psnake != NULL)
    {
        printf("Element %d\n", counter);
        printf("Snake x: %d\n", psnake->x);
        printf("Snake y: %d\n", psnake->y);
        printf("Snake self: %p\n", psnake);
        printf("Snake next: %p\n", psnake->pnext);
        psnake = psnake->pnext;
        counter++;
    }
}


void move_snake(struct SnakeElement **ppsnake, struct Direction *pdirection)
{
    assert(ppsnake != NULL);
    assert(*ppsnake != NULL);

    if(pdirection->dx == 0 && pdirection->dy == 0)
        return;

    size_t size = snake_size(ppsnake);
    if (size == 1)
    {
        (*ppsnake)->x += pdirection->dx;
        (*ppsnake)->y += pdirection->dy;
    }
    else
    {
        struct SnakeElement *pcurrent = *ppsnake;
        int previous_x = pcurrent->x;
        int previous_y = pcurrent->y;

        pcurrent->x += pdirection->dx;
        pcurrent->y += pdirection->dy;
        while (pcurrent->pnext != NULL)
        {
            pcurrent = pcurrent->pnext;
            int temp_x = pcurrent->x;
            int temp_y = pcurrent->y;

            pcurrent->x = previous_x;
            pcurrent->y = previous_y;

            previous_x = temp_x;
            previous_y = temp_y;
        }
    }
}



void reset_apple(struct SnakeElement *psnake, struct Apple *papple)
{
    papple->x = COLUMNS * ((double)rand() / RAND_MAX);
    papple->y = ROWS * ((double)rand() / RAND_MAX);

    /* If apple coordinates collide with snake, try again */
    struct SnakeElement *pcurrent = psnake;
    do
    {
        if(pcurrent->x == papple->x && pcurrent->y == papple->y)
        {
            reset_apple(psnake, papple);
            break;
        }
        pcurrent = pcurrent->pnext;
    }
    while(pcurrent != NULL);
}

void lengthen_snake(struct SnakeElement **ppsnake, struct Direction *pdirection)
{
    struct SnakeElement *head = malloc(sizeof(struct SnakeElement));
    head->x = (*ppsnake)->x + pdirection->dx;
    head->y = (*ppsnake)->y + pdirection->dy;
    head->pnext = *ppsnake;
    
    *ppsnake = head;
}   

int check_collision(struct SnakeElement **ppsnake)
{
    assert(ppsnake != NULL);
    assert(*ppsnake != NULL);

    /* Check wall collision */
    struct SnakeElement snake = **ppsnake;
    if(snake.x < 0 || snake.y < 0 || snake.x >= COLUMNS || snake.y >= ROWS)
        return 1;

    /* Check self collision */
    struct SnakeElement *psnake = *ppsnake;
    while (psnake->pnext != NULL)
    {
        psnake = psnake->pnext;
        if(snake.x == psnake->x && snake.y == psnake->y)
            return 1;
    }
    return 0;
}

int snake_collides_with(struct SnakeElement *head, struct SnakeElement *other)
{
    while(other != NULL)
    {
        if (head->x == other->x && head->y == other->y)
            return 1; /* Collision detected */
        
        other = other->pnext;
    }
    return 0; /* No collision */

}

void free_snake(struct SnakeElement **ppsnake)
{
    struct SnakeElement *pcurrent = *ppsnake;
    struct SnakeElement *pnext;

    while(pcurrent != NULL)
    {
        pnext = pcurrent->pnext;
        free(pcurrent);
        pcurrent = pnext;
    }
    *ppsnake = NULL;
}

int main ()
{
    int mode = choose_game_mode(); /* 1 or 2 players */

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Snake Game", WIDTH, HEIGHT, 0);

    SDL_Surface *psurface = SDL_GetWindowSurface(window);
    
    SDL_Event event;

    struct SnakeElement *psnake = malloc(sizeof(struct SnakeElement));
    struct SnakeElement *psnakeTail = malloc(sizeof(struct SnakeElement));
    struct SnakeElement *psnakeTail2 = malloc(sizeof(struct SnakeElement));



    psnake->x = 5;
    psnake->y = 5;
    psnake->pnext = psnakeTail;

    psnakeTail->x = 5;
    psnakeTail->y = 6;
    psnakeTail->pnext = psnakeTail2;

    psnakeTail2->x = 5;
    psnakeTail2->y = 7;
    psnakeTail2->pnext = NULL;

    

    struct SnakeElement **ppsnake = &psnake; 
    struct Direction direction = {0,0};
    struct Direction *pdirection = &direction;
    
    /* For 2nd player in future */
    struct SnakeElement *psnake2 = NULL; 
    struct SnakeElement **ppsnake2 = &psnake2;
    struct Direction direction2 = {0,0};
    struct Direction *pdirection2 = &direction2;
    if (mode == 2)
    {
        psnake2 = malloc(sizeof(struct SnakeElement));
        struct SnakeElement *psnake2Tail = malloc(sizeof(struct SnakeElement));
        struct SnakeElement *psnake2Tail2 = malloc(sizeof(struct SnakeElement));

        psnake2->x = 20;
        psnake2->y = 10;
        psnake2->pnext = psnake2Tail;

        psnake2Tail->x = 20;
        psnake2Tail->y = 11;
        psnake2Tail->pnext = psnake2Tail2;

        psnake2Tail2->x = 20;
        psnake2Tail2->y = 12;
        psnake2Tail2->pnext = NULL;

        direction2.dx = -1;   /* Start moving left */
        direction2.dy = 0;
    }
    struct Apple apple;
    struct Apple *papple = &apple;
    reset_apple(psnake, papple);


    SDL_Rect override_rect = {0,0,WIDTH,HEIGHT};

    int game = 1;
    int score = 0;
    int high_score = load_high_score("score.txt");

    direction.dx = 1;   /* Start moving right */
    direction.dy = 0;
    while(game)
    {

        while(SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
                game = 0;
            if (event.type == SDL_EVENT_KEY_DOWN)
            {   /* Player 1 Controls (Arrows) */
                if (event.key.key == SDLK_RIGHT && direction.dx != -1) {direction.dx = 1; direction.dy = 0;}
                if (event.key.key == SDLK_LEFT && direction.dx != 1) {direction.dx = -1; direction.dy = 0;}
                if (event.key.key == SDLK_UP && direction.dy != 1) {direction.dy = -1; direction.dx = 0;}
                if (event.key.key == SDLK_DOWN && direction.dy != -1) {direction.dy = 1; direction.dx = 0;}

                /* Player 2 Controls (WASD) */
                if (mode == 2)
                {
                    if (event.key.key == SDLK_D && direction2.dx != -1) {direction2.dx = 1; direction2.dy = 0;}
                    if (event.key.key == SDLK_A && direction2.dx != 1) {direction2.dx = -1; direction2.dy = 0;}
                    if (event.key.key == SDLK_W && direction2.dy != 1) {direction2.dy = -1; direction2.dx = 0;}
                    if (event.key.key == SDLK_S && direction2.dy != -1) {direction2.dy = 1; direction2.dx = 0;}
                }
            }
        }
        
        /* print_snake(ppsnake); */
        
        SDL_FillSurfaceRect(psurface, &override_rect, COLOR_BLACK);
        
        move_snake(ppsnake, pdirection);
        if (mode == 2)
            move_snake(ppsnake2, pdirection2);


        if(check_collision(ppsnake))
        {
            printf("Collision! Game Over!\n");
            /* Free Memory */
            free_snake(ppsnake);
            save_high_score("score.txt", high_score);
            game = 0;
        }

        /* Player 1 eats apple */
        if (psnake->x == papple->x && psnake->y == papple->y)
        {
            reset_apple(psnake, papple);
            lengthen_snake(ppsnake, pdirection);
            score++;

            /* Updating high score */
            if (score > high_score) high_score = score;
        }

        /* Player 2 eats apple */
        if (mode == 2 && psnake2->x == papple->x && psnake2->y == papple->y)
        {
            reset_apple(psnake2, papple);
            lengthen_snake(ppsnake2, pdirection2);
        }

        /* Player 1 hits Player 2 */
        if (mode == 2 && snake_collides_with(psnake, psnake2))
        {
            printf("Player 1 crashed into Player 2!\n");
            game = 0;
        }

        /* Player 2 hits Player 1*/
        if (mode == 2 && snake_collides_with(psnake2, psnake))
        {
            printf("Player 2 crashed into Player 1!\n");
            game = 0;
        }

        APPLE(papple->x, papple->y);
        draw_snake(psurface, ppsnake);
        if (mode == 2)
            draw_snake(psurface, ppsnake2);
        DRAW_GRID;

        SDL_UpdateWindowSurface(window);
        SDL_Delay(200);
    }
    save_high_score("score.txt", high_score);
    free_snake(ppsnake);
    if (mode == 2)
        free_snake(ppsnake2);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}