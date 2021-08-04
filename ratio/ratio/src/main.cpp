#include <iostream>

// <CORE>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <stddef.h>


typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef double float64;

typedef size_t Size;


#define internal static
#define local_persist static

template<typename T>
struct __Defer {
    T t;
    __Defer(T t) : t(t) { }
    ~__Defer() { t(); }
};

template<typename T>
__Defer<T> __DoDefer(T t) { return __Defer<T>(t); }

#define __DEFER_MERGE(x, y) x##y
#define __DEFER_MERGE_1(x, y) __DEFER_MERGE(x, y)
#define __DEFER_COUNTER(x) __DEFER_MERGE_1(x, __COUNTER__)
#define defer(code) auto __DEFER_COUNTER(__defer) = __DoDefer([&](){code;})

#define assert(x) { if (!(x)) { __debugbreak(); } }
// </CORE>

// SDL is annoying by doing this
#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>

#include <math.h>
#include <glad/glad.h>



struct Window {
    SDL_Window* window;
    SDL_GLContext context;
    void init_window(const char* title, u32 width, u32 height);
    void destroy_window();

    void update();  

    void get_size(u32* w, u32* h);

    void swap_buffers();
};




enum CellType {
    CELL_EMPTY = 0,
    CELL_SQUARE = 1,
    CELL_TRIANGLE = 2,
    CELL_CIRCLE = 3,
};

struct GridCell {
    u16 type;
    s32 placement_id;
    bool visited;
};

struct Level {
    GridCell* grid;
    u32 grid_width, grid_height;

    u8 rat_square, rat_triangle, rat_circle;

    u32 start_x, start_y;
    u32 end_x, end_y;

    void create_level(u32 grid_width, u32 grid_height);
    void destroy_level();
    void draw_level();
    void load_from_file(const char* name);
};

void Level::load_from_file(const char* name)
{
    printf("Loading level from file: %s\n", name);

    std::ifstream infile(name);

    u32 width = 0;
    u32 height = 0;
    u32 i = 0;
    
    {
        std::string temp_line;
        std::getline(infile, temp_line);
        sscanf_s(temp_line.c_str(), "%u,%u", &width, &height);
    }

    this->create_level(width, height);


    {
        std::string temp_yeet;
        std::getline(infile, temp_yeet);
        sscanf_s(temp_yeet.c_str(), "%hhu,%hhu,%hhu", &this->rat_square, &this->rat_triangle, &this->rat_circle);

        printf("%d-%d-%d\n", this->rat_square, this->rat_triangle, this->rat_circle);
    }

    {
        std::string temp_yote;
        std::getline(infile, temp_yote);
        sscanf_s(temp_yote.c_str(), "%u,%u,%u,%u", &this->start_x, &this->start_y, &this->end_x, &this->end_y);
    }

    for( std::string line; std::getline( infile, line ); )
    {  
        std::stringstream str;
        str << line;

        for(int q =0; q < width; q++)
        {
            GridCell gc;
        
            str>>gc.type;
        
            // printf("Cell %d,%d: %d\n", i, q, gc.type);

            // piece of poo
            char _;
            str>>_;
            assert(_==',');

            // write level
            this->grid[q + (this->grid_height - i - 1) * this->grid_width] = gc;
        }

        i++;
    }

    // std::cout<<width<<"x"<<height<<std::endl;
}


constexpr u32 MAX_LEVELS = 64;

struct Line_Point {
    u32 x, y;
};

struct Game_State {
    Window window;
    bool is_running;

    Level* current_level;
    
    u32 next_level;
    u32 level_count;
    Level level_list[MAX_LEVELS];

    float render_width;
    float render_height;

    bool mouse_down;
    bool mouse_held;
    bool mouse_up;

    float mouse_x;
    float mouse_y;

    Line_Point line_points[1024];
    u32 line_point_count;

} game_state;


u32 add_level(Level level) {
    assert(game_state.level_count < MAX_LEVELS);

    game_state.level_list[game_state.level_count] = level;

    defer(game_state.level_count++);

    return game_state.level_count;
}

void load_level(u32 id) {
    game_state.current_level = &game_state.level_list[id];
    game_state.line_point_count = 0;
    game_state.next_level = id + 1;
}


void Window::init_window(const char* title, u32 width, u32 height) {
    this->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
    this->context = SDL_GL_CreateContext(this->window);

    gladLoadGLLoader(SDL_GL_GetProcAddress);
}

void Window::destroy_window() {
    SDL_GL_DeleteContext(this->context);
    SDL_DestroyWindow(this->window);
}

void Window::update() {
    SDL_Event event;

    game_state.mouse_down = false;
    game_state.mouse_up = false;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            game_state.is_running = false;
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                game_state.mouse_down = true;
                game_state.mouse_held = true;
            }
        }
        else if (event.type == SDL_MOUSEBUTTONUP) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                game_state.mouse_up = true;
                game_state.mouse_held = false;
            }
        }else if (event.type == SDL_KEYDOWN) {
            game_state.line_point_count = 0;
        }
    }

    u32 mouse_x, mouse_y;
    SDL_GetMouseState((s32*)&mouse_x, (s32*)&mouse_y);

    u32 win_width, win_height;
    this->get_size(&win_width, &win_height);

    game_state.mouse_x = (mouse_x * game_state.render_width / win_width - game_state.render_width / 2);
    game_state.mouse_y = (game_state.render_height / 2 - mouse_y * game_state.render_height / win_height);
}

void Window::get_size(u32* w, u32* h) {
    SDL_GL_GetDrawableSize(window, (s32*)w, (s32*)h);


    // u32 ww, hh;
    // SDL_GetWindowSize(window, (s32*)&ww, (s32*)&hh);
    // printf("\nwindow size: def(%d, %d) vs ret(%d, %d)\n", ww, hh, *w, *h);
}

void Window::swap_buffers() {
    SDL_GL_SwapWindow(window);
}


// draw utility functions
constexpr float FIXED_HEIGHT = 720;
constexpr float ZFAR = 32000;

const float PI = 3.14159265358979;
constexpr u32 CIRCLE_POLYS = 64;

void draw_tri(float x, float y, float w, float h, float r, float g, float b)
{
    glBegin(GL_TRIANGLES);
    glColor3f(r, g, b);
 
    glVertex2f(x,y+h/2);
    glVertex2f(x+w/2,y-h/2);
    glVertex2f(x-w/2,y-h/2);
    glEnd();
}

void draw_rect(float x, float y, float w, float h, float r, float g, float b)
{
    glBegin(GL_QUADS);
    glColor3f(r, g, b);
 
    glVertex2f(x-w/2,y-h/2);
    glVertex2f(x+w/2,y-h/2);
    glVertex2f(x+w/2,y+h/2);
    glVertex2f(x-w/2,y+h/2);
    glEnd();
}


void draw_circle(float x, float y, float rad, float r, float g, float b)
{
    glBegin(GL_POLYGON);
    glColor3f(r, g, b);

    for (u32 i = 0; i < CIRCLE_POLYS; i++) {
        float rot = 2 * i * PI / CIRCLE_POLYS;
        glVertex2f(cos(rot) * rad + x, sin(rot) * rad + y);
    }

    glEnd();
}






/// useless comment


void Level::create_level(u32 grid_width, u32 grid_height) {
    this->grid_width = grid_width;
    this->grid_height = grid_height;
    this->grid = (GridCell*)malloc(sizeof(GridCell) * grid_width * grid_height);
}

void Level::destroy_level() {
    free(this->grid);
}


void draw_cell(GridCell cell, float x, float y, float w, float h) {
    draw_rect(x, y, w, h, 0.5, 0.5, 0.5);

    switch (cell.type) {
        case CELL_SQUARE: {
            draw_rect(x, y, w / 2, h / 2, 0, 0, 1); 
            break;   
        }
        case CELL_TRIANGLE: {
            draw_tri(x, y, w / 2, h / 2, 1, 0, 1); // @Todo triangle
            break;   
        }
        case CELL_CIRCLE: {
            draw_circle(x, y, w / 4, 1, 0, 0); // @Todo circle
            break;   
        }
    }
}

float sign(float x) {
    if (x == 0)
        return 0;
    if (x > 0)
        return 1;
    if (x < 0)
        return -1;
}

void flood_fill_id(u32 id, u32 x, u32 y, Level* level) {
    auto* grid = &level->grid[x + y * level->grid_width];
    grid->visited = true;
    grid->placement_id = id;

    for (s32 i = -1; i <= 1; i++) {
        for (s32 j = -1; j <= 1; j++) {
            if (abs(i) + abs(j) == 1) {
                s32 xx = x + i;
                s32 yy = y + j;


                bool is_fillable = true;
                for (u32 lid = 1; lid < game_state.line_point_count; lid++) {
                    Line_Point lp0 = game_state.line_points[lid - 1];
                    Line_Point lp1 = game_state.line_points[lid];
                    
                    float dx = sign(lp1.x - lp0.x);
                    float dy = sign(lp1.x - lp0.x);

                    float max = (dx * (lp1.x - lp0.x)) + (dy * (lp1.y - lp0.y));
                    float amt = (dx * (x - lp0.x)) + (dy * (y - lp0.y));

                    if (amt > 0 && amt < max) {
                        
                        float cx = -dy;
                        float cy = dx;

                        float si_tar = cx * (xx - lp0.x) + cy * (yy - lp0.y);
                        float si_src = cx * (x - lp0.x) + cy * (y - lp0.y);
                        if (sign(si_tar) != sign(si_src))
                            is_fillable = false;
                    }
                }

                if (level->grid[xx + yy * level->grid_width].visited == false && is_fillable) {
                    flood_fill_id(id, xx, yy, level);
                }
            }
        }
    }
}

void check_if_complete(Level* level) {
    //for (u32 j = 0; j < level->grid_height; j++) {
    //    for (u32 i = 0; i < level->grid_width; i++) {
    //        level->grid[i + j * level->grid_width].placement_id = -1;
    //    }
    //}

    //u32 np = 0;
    //for (u32 j = 0; j < level->grid_height; j++) {
    //    for (u32 i = 0; i < level->grid_width; i++) {
    //        if (level->grid[i + j * level->grid_width].placement_id == -1)
    //            flood_fill_id(np, i, j);

    //        np++;
    //    }
    //}

    //GridCell** cells = (GridCell**)alloca(np * sizeof(GridCell*));
    //u32* cell_count = (u32*)alloca(np * sizeof(u32));

    //for (u32 i = 0; i < np; i++) {
    //    cells[i] = *(GridCell*)malloc(sizeof(GridCell) * level->grid_width * level->grid_height);
    //    cell_count[i] = 0;
    //}

    //for (u32 j = 0; j < level->grid_height; j++) {
    //    for (u32 i = 0; i < level->grid_width; i++) {
    //        auto* cell = &level->grid[i + j * level->grid_width];
    //        auto* cell_arr = cells[cell->placement_id];
    //        
    //        cell_arr[cell_count[cell->placement_id]++] = *cell;
    //    }
    //}

    //// bool success = true;

    //// for (u32 i = 0; i < np; i++) {
    ////     u32 square = 0;
    ////     u32 triangle = 0;
    ////     u32 circle = 0;

    ////     auto* subcells = cells[i];
    ////     for (u32 q = 0; q < cell_count[i]; q++) {
    //        
    ////     }
    //// }


    if (game_state.next_level != game_state.level_count) {
        load_level(game_state.next_level);
    }
    else {
        printf("Thanks for playing!\n");
        exit(0);
    }
}

void Level::draw_level() {
    const float level_height = 500;
    const float cell_margin = 20;
    
    float cell_size = (level_height - (this->grid_height - 1) * cell_margin) / this->grid_height;


    auto* window = &game_state.window;


    // float aspect = float(this->grid_width) / float(this->grid_height);
    // @Todo understand this
    float level_width = (cell_size * this->grid_width + (this->grid_width - 1) * cell_margin);

    float lx = 0;
    float ly = 0;

    float origin_x = lx - level_width / 2;
    float origin_y = ly - level_height / 2;
    
    float x = origin_x, y = origin_y;

    // draw the background

    draw_rect(lx, ly, cell_margin * 2 + level_width, cell_margin * 2 + level_height, 0, 0, 0);

    for (u32 row = 0; row < this->grid_height; row++) {
        if (row != 0)
            y += cell_margin;

        for (u32 col = 0; col < this->grid_width; col++) {
            if (col != 0)
                x += cell_margin;            

            draw_cell(this->grid[col + row * this->grid_width], x + cell_size / 2, y + cell_size / 2, cell_size, cell_size);
            

            x += cell_size;
        }
        x = origin_x;
        y += cell_size;
    }



    draw_circle(lx - level_width / 2 - cell_margin / 2 + this->start_x * (cell_size + cell_margin), 
                ly - level_height / 2 - cell_margin / 2 + this->start_y * (cell_size + cell_margin),
                cell_margin, 0, 0, 0);

    draw_rect(lx - level_width / 2 - cell_margin / 2 + this->end_x * (cell_size + cell_margin), 
                ly - level_height / 2 - cell_margin / 2 + this->end_y * (cell_size + cell_margin),
                cell_margin * 1.5, cell_margin * 1.5, 0, 0, 0);


    float cell_bboxx = (level_width / this->grid_width);
    float cell_bboxy = (level_height / this->grid_width);
    
    float mouse_cell_id_x = round((game_state.mouse_x - origin_x) / cell_bboxx);
    float mouse_cell_id_y = round((game_state.mouse_y - origin_y) / cell_bboxy);
    float isx = mouse_cell_id_x * cell_bboxx + origin_x;
    float isy = mouse_cell_id_y * cell_bboxy + origin_y;
    


    float max_mouse_dist = 30;

    if (( (isx - game_state.mouse_x) * (isx - game_state.mouse_x) + (isy - game_state.mouse_y) * (isy - game_state.mouse_y) ) < max_mouse_dist * max_mouse_dist) {
        draw_rect(isx, isy, 30, 30, 1, 0, 1);

        if (game_state.mouse_down) {
            Line_Point lp = { mouse_cell_id_x, mouse_cell_id_y };
            
            if (lp.x == this->end_x && lp.y == this->end_y) {
                check_if_complete(this);
                game_state.line_point_count = 0;
            }

            bool valid = true;
            if (game_state.line_point_count > 0) {
                Line_Point lp1 = game_state.line_points[game_state.line_point_count - 1];
                valid = false;
                if ((lp.x == lp1.x || lp.y == lp1.y) && (lp.x != lp1.x || lp.y != lp1.y)) {
                    valid = true;
                }
            }
            else {
                valid = lp.x == this->start_x && lp.y == this->start_y;
            }

            if (valid)
                game_state.line_points[game_state.line_point_count++] = lp;
        }
    }

    for (u32 i = 1; i < game_state.line_point_count; i++) {
        Line_Point lp0 = game_state.line_points[i - 1];
        Line_Point lp1 = game_state.line_points[i];
        
        float ax = lp0.x * cell_bboxx + origin_x;
        float ay = lp0.y * cell_bboxy + origin_y;

        float bx = lp1.x * cell_bboxx + origin_x;
        float by = lp1.y * cell_bboxy + origin_y;

        draw_rect((ax + bx) / 2, (ay + by) / 2, abs(ax - bx) + 10, abs(ay - by) + 10, 1, 1, 1);
    }

    //     switch (cell.type) {
    //     case CELL_SQUARE: {
    //         draw_rect(x, y, w / 2, h / 2, 0, 0, 1); 
    //         break;   
    //     }
    //     case CELL_TRIANGLE: {
    //         draw_tri(x, y, w / 2, h / 2, 1, 0, 1); // @Todo triangle
    //         break;   
    //     }
    //     case CELL_CIRCLE: {
    //         draw_circle(x, y, w / 4, 1, 0, 0); // @Todo circle
    //         break;   
    //     }
    // }


    for(int i = 0; i < 3; i++)
    {
        float currentxpos=0;
        switch (i)
        {
            case 0:
            for(int x = 0; x < rat_square; x++)
            {
                draw_rect(currentxpos - game_state.render_width/2 + 55,game_state.render_height/2 - 55,50,50, 0, 0, 1);
                currentxpos+=52;
            }
            break;
             case 1:
            for(int x = 0; x < rat_triangle; x++)
            {
                draw_tri(currentxpos - game_state.render_width/2 + 55,game_state.render_height/2 - 110,50,50, 1, 0, 1);
                currentxpos+=52;
            }
            break;
             case 2:
            for(int x = 0; x < rat_circle; x++)
            {
                draw_circle(currentxpos - game_state.render_width/2 + 55,game_state.render_height/2 - 165,25, 1, 0, 0);
                currentxpos+=52;
            }
            break;
        }
    }
}






void render_game()
{
    auto* window = &game_state.window;


    //do stuff here
    u32 win_width, win_height;
    window->get_size(&win_width, &win_height);
    glViewport(0, 0, win_width, win_height);


    float aspect = float(win_width) / float(win_height);
    float width = FIXED_HEIGHT * aspect;
    float height = FIXED_HEIGHT;

    game_state.render_width = width;
    game_state.render_height = height;

    glLoadIdentity(); // Stupid old opengl functions carry over from previous frame
    glOrtho(-width / 2, width / 2, -height / 2, height / 2, 0, ZFAR);
    

    glClearColor(.9,.9,.9,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear depth & color buffer


    game_state.current_level->draw_level();


    window->swap_buffers();
}





int main()
{

    game_state = Game_State {};

    auto* window = &game_state.window;
    window->init_window("Ratio - sssoftware llc", 1280, 720);
    defer(window->destroy_window());
    
    // init game
    
        Level l;
        defer(l.destroy_level());
        l.load_from_file("L1.map");
        add_level(l);
    
    
        
        Level l2;
        defer(l2.destroy_level());
        l2.load_from_file("L2.map");
        add_level(l2);
    
    load_level(0);
    
    game_state.is_running = true;
    while (game_state.is_running) {
        window->update();

        render_game();
    }

}