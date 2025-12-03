## Snake Game (SDL3) – Quick Reference

- Single or two-player Snake built with SDL3. Grid uses `CELL_SIZE` over a `WIDTH x HEIGHT` window. Apples spawn off the snake; players grow by eating apples and lose on wall/self collisions.
- High score: `score.txt` stores one integer (best score). Hall of Fame: `hall_of_fame.txt` stores newline-separated scores; only top 5 (descending) are kept.

### Core Structures & Constants (test.c)
- `SnakeElement { int x, y; SnakeElement *pnext; }` singly linked list; head is first segment.
- `Direction { int dx, dy; }` movement per tick. `Apple { int x, y; }`.
- Constants: `WIDTH/HEIGHT`, `CELL_SIZE`, derived `ROWS/COLUMNS`, colors, `LINE_WIDTH`. Macros: `SNAKE(x,y)`/`APPLE(x,y)` call `fill_cell`; `DRAW_GRID` calls `draw_grid`.

### High Score / Hall of Fame
- `load_high_score(filename)`: read one int; missing file → 0. `save_high_score(filename)`: overwrite with best score.
- `load_hall_of_fame(filename, int **scores)`: read all scores into a dynamic array; returns count (0 if file missing).
- `save_hall_of_fame(filename, scores, count)`: write each score on its own line.
- `add_score_to_hall_of_fame(scores, count, new_score)`: append new score, bubble-sort descending, cap `count` at 5 (top 5 only).
- `print_hall_of_fame(scores, count)`: print up to 5 scores with ranks.

### Rendering Helpers
- `draw_grid(surface)`: draw horizontal/vertical lines spaced by `CELL_SIZE`.
- `fill_cell(surface, x, y, color)`: fill one grid cell.

### Snake Mechanics
- `move_snake(ppsnake, pdirection)`: size 1 → move head by `(dx,dy)`; larger → shift positions down the list so body follows head.
- `lengthen_snake(ppsnake, pdirection)`: add a new head one step forward, pointing to old head.
- `check_collision(ppsnake)`: head vs walls and head vs body; returns 1 if collision.
- `snake_collides_with(head, other)`: head vs any segment of other snake (2-player).
- `reset_apple(psnake, papple)`: place apple randomly; if on snake, retry.
- `free_snake(ppsnake)`: free entire list and null pointer.

### Input Controls (Snake)
- Player 1: Arrow keys (blocks 180° reversals).
- Player 2 (if selected): WASD (blocks 180° reversals).

### Game Loop (high level)
1) Poll input; update directions.  
2) Clear screen; move snake(s).  
3) Collisions: walls/self and inter-snake (2-player); on collision, end game.  
4) Eating apples: if head on apple, `reset_apple`, `lengthen_snake`, `score++`, update `high_score` if higher.  
5) Draw apple, snake(s), grid; update window; delay for tick timing.

### Startup / Exit
- Prompt for game mode (1 or 2); defaults to 1 if invalid.
- Load and print Hall of Fame before starting.
- On exit: save `score.txt`, update Hall of Fame with final scores (keep top 5), save `hall_of_fame.txt`, print list. Free snakes and SDL resources.

### Common Break/Fix Spots (if lines are deleted)
- Missing `count++` or top-5 cap in Hall of Fame add → new score ignored or too many entries.
- Removing self-collision loop in `check_collision` → snake passes through itself.
- Deleting 180° turn guards → instant self-collision when reversing.
- Dropping `save_high_score`/`save_hall_of_fame` on exit → files never update.
- Freeing snakes inside the loop → use-after-free; free after loop.
- Removing `snake_collides_with` calls → inter-snake collisions not detected.

### Quick Build/Run (example)
```bash
gcc test.c -Iinclude -Llib -lSDL3 -o test
./test   # choose 1 or 2 at prompt
```
