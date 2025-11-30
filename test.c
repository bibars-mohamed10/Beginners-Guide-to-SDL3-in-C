#include <stdio.h>
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <assert.h>
// #include <debug_malloc.h>

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

/* Linked-List node representing one snake segment */
struct SnakeElement
{
    int x,y;

    /* Can be NULL (in case of last element) */
    struct SnakeElement *pnext;

};

/* Direction structure for movement */
struct Direction
{
    int dx, dy;
};

/* Apple structure for position */
struct Apple
{
    int x,y;
};

/* Ask user for game mode (single or two-player) */
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

/* Load best high score from file (single number) */
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

/* Save best high score to file */
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

/* ==== HALL OF FAME (TOP 5 SCORES) */

/* Load all scores from hall_of_fame.txt into dynamic array */
int load_hall_of_fame(const char *filename, int **scores)
{
    FILE *fp = fopen(filename, "r");

    if (fp == NULL)
    {
        *scores = NULL;
        return 0; /* No scores yet */
    }

    int capacity = 10;
    *scores = (int *)malloc(sizeof(int) * capacity);
    int count = 0;

    while (fscanf(fp, "%d", &((*scores)[count])) == 1)
    {
        count++;
        if (count >= capacity)
        {
            capacity *= 2;
            int *temp = (int *)realloc(*scores, sizeof(int) * capacity);
            if (temp == NULL)
            {
                /* If realloc fails, keep the old pointer */
                fclose(fp);
                return count; /* Return what we have */
            }
            *scores = temp;
        }
    }

    fclose(fp);
    return count;
}


/* Save all scores to hall_of_fame.txt from dynamic array */
void save_hall_of_fame(const char *filename, int *scores, int count)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("Error opening file for writing!\n");
        return;
    }

    for (int i = 0; i < count; i++)
        fprintf(fp, "%d\n", scores[i]);

    fclose(fp);
}


/* Add new score to hall of fame, sort descending, keeping only top 5 scores */
void add_score_to_hall_of_fame(int **scores, int *count, int new_score)
{

    if (*scores == NULL)
    {
        *scores = (int *)malloc(sizeof(int) * 1);
        (*scores)[0] = new_score;
        *count = 1;
        return;
    }

    /* Add new score to dynamic array */
    int *temp = (int *)realloc(*scores, sizeof(int) * (*count + 1));
    if (temp == NULL) return;
    *scores = temp;

    (*scores)[*count] = new_score;
    (*count)++;


    /* Sort scores in descending order (simple bubble sort) */
    for (int i = 0; i < *count - 1; i++)
    {
        for (int j = 0; j < *count - i - 1; j++)
        {
            if ((*scores)[j] < (*scores)[j + 1])
            {
                int temp = (*scores)[j];
                (*scores)[j] = (*scores)[j + 1];
                (*scores)[j + 1] = temp;
            }
        }
    }

    /* Keep only top 5 scores */
    if (*count > 5)
        *count = 5;
    
    /* Resize array to keep only top 5 scores */
    *scores = (int *)realloc(*scores, sizeof(int) * (*count));
}

/* Print Hall of Fame scores to console */
void print_hall_of_fame(int *scores, int count)
{
    if (scores == NULL || count == 0)
    {
        printf("No Hall of Fame scores yet!\n");
        return;
    }

    printf("=== Hall of Fame ===\n");
    for (int i = 0; i < count && i < 5; i++)
    {
        printf("%d. %d\n", i + 1, scores[i]);
    }
    printf("====================\n");
}

/* ==== GAME LOGIC FUNCTIONS ==== */

/* Draw background grid */
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

/* Fill a cell at (x,y) with a specific color */
void fill_cell(SDL_Surface *psurface, int x, int y, Uint32 color)
{
    SDL_Rect rect = {x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE, CELL_SIZE};

    SDL_FillSurfaceRect(psurface, &rect, color);
}

/* Count snake length (linked list traversal)*/
size_t snake_size(struct SnakeElement **ppsnake)
{
    size_t list_size = 1;
    struct SnakeElement *current = *ppsnake;
    while (current->pnext != NULL)
    {
       current = current->pnext;
       list_size++;
    }
    return list_size;
}

/* Draw the snake on the surface */
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

/* Print snake details for debugging */
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

/* Move snake by shifting position along the linked list */
void move_snake(struct SnakeElement **ppsnake, struct Direction *pdirection)
{
    if(pdirection->dx == 0 && pdirection->dy == 0) return;

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


/* Place apple randomly, avoiding snake body */
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

/* Lengthen snake by adding new head in the movement direction */
void lengthen_snake(struct SnakeElement **ppsnake, struct Direction *pdirection)
{
    struct SnakeElement *head = malloc(sizeof(struct SnakeElement));
    head->x = (*ppsnake)->x + pdirection->dx;
    head->y = (*ppsnake)->y + pdirection->dy;
    head->pnext = *ppsnake;
    
    *ppsnake = head;
}   

/* Check for collisions with walls or self */
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

/* Two snake collision case (2-player mode) */
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

/* Free whole linked list */
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

/* ==== MAIN GAME LOOP ====*/
int main ()
{
    int mode = choose_game_mode(); /* 1 or 2 players */
    printf("Starting Game in %d-Player Mode!\n", mode);

    /* Load and print Hall of Fame */
    int *hof_scores = NULL;
    int hof_count = load_hall_of_fame("hall_of_fame.txt", &hof_scores);
    print_hall_of_fame(hof_scores, hof_count);


    /* Initialize SDL */
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Snake Game", WIDTH, HEIGHT, 0);
    SDL_Surface *psurface = SDL_GetWindowSurface(window);
    SDL_Event event;

    /* Initial Snake (3 segments) */
    struct SnakeElement *psnake = malloc(sizeof(struct SnakeElement));
    struct SnakeElement *psnakeTail = malloc(sizeof(struct SnakeElement));
    struct SnakeElement *psnakeTail2 = malloc(sizeof(struct SnakeElement));



    psnake->x = 5; psnake->y = 5; psnake->pnext = psnakeTail;
    psnakeTail->x = 5; psnakeTail->y = 6; psnakeTail->pnext = psnakeTail2;
    psnakeTail2->x = 5; psnakeTail2->y = 7; psnakeTail2->pnext = NULL;

    

    struct SnakeElement **ppsnake = &psnake; 

    struct Direction direction = {0,0};
    struct Direction *pdirection = &direction;
    
    /* Second Snake for 2-player mode */
    struct SnakeElement *psnake2 = NULL; 
    struct SnakeElement **ppsnake2 = &psnake2;
    struct Direction direction2 = {0,0};
    struct Direction *pdirection2 = &direction2;

    /* Initialize second snake if in 2-player mode */
    if (mode == 2)
    {
        psnake2 = malloc(sizeof(struct SnakeElement));
        struct SnakeElement *psnake2Tail = malloc(sizeof(struct SnakeElement));
        struct SnakeElement *psnake2Tail2 = malloc(sizeof(struct SnakeElement));

        psnake2->x = 20; psnake2->y = 10; psnake2->pnext = psnake2Tail;

        psnake2Tail->x = 20; psnake2Tail->y = 11; psnake2Tail->pnext = psnake2Tail2;

        psnake2Tail2->x = 20; psnake2Tail2->y = 12; psnake2Tail2->pnext = NULL;

        direction2.dx = -1;   /* Start moving left */
        direction2.dy = 0;
    }

    struct Apple apple;
    struct Apple *papple = &apple;
    reset_apple(psnake, papple);

    
    SDL_Rect override_rect = {0, 0, WIDTH, HEIGHT};

    int game = 1;
    int score = 0;
    int score2 = 0;
    int high_score = load_high_score("score.txt");

    direction.dx = 1;   /* Start moving right */
    direction.dy = 0;

    /* Game Loop */
    while(game)
    {
        /* Handle Keyboard */
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

        /* Check collisions for Player 1 */
        if(check_collision(ppsnake))
        {
            printf("Collision! Game Over!\n");
            /* Free Memory */
            save_high_score("score.txt", high_score);
            game = 0;
        }

        /* Check collisions for Player 2 */
        if (mode == 2 && check_collision(ppsnake2))
        {
            printf("Player 2 collision! Game Over!\n");
            /* Free Memory */
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
            score2++;

            /* Updating high score */
            if (score2 > high_score) high_score = score2;
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

    /* Free Snakes */
    free_snake(ppsnake);
    if (mode == 2) free_snake(ppsnake2);


    /* Save best high score */
    save_high_score("score.txt", high_score);

    /* Update Hall of Fame with game's score */
    add_score_to_hall_of_fame(&hof_scores, &hof_count, score);
    if (mode == 2)
        add_score_to_hall_of_fame(&hof_scores, &hof_count, score2);
    save_hall_of_fame("hall_of_fame.txt", hof_scores, hof_count);
    print_hall_of_fame(hof_scores, hof_count);
    printf("Final Score: %d\n", score);
    if (mode == 2)
        printf("Player 2 Final Score: %d\n", score2);

    /* Free Memory */
    free(hof_scores);

    /* Cleanup SDL */
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}