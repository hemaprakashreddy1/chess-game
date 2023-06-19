#include <stdio.h>
#include <stdlib.h>

struct pos_node
{
    int pos;
    struct pos_node *next;
};

struct log_node
{
    int from;
    int from_coin;
    int to;
    int to_coin;
    int move_type;
    struct log_node *next;
};

struct rook_info
{
    int pos;
    int moves;
    int captured;
};

struct move_count
{
    int count;
    struct move_count *next;
};

struct move_node
{
    int from;
    int to;
    int move_type;
    struct move_node *next;
};

struct queue
{
    struct move_node *front;
    struct move_node *rear;   
};

struct log_node *move_log;
struct pos_node *black_positions[6], *white_positions[6];
struct rook_info rooks[4];
struct move_count *head;

int board[8][8];
int pawn_shape[7][7];
int rook_shape[7][7];
int bishop_shape[7][7];
int knight_shape[7][7];
int queen_shape[7][7];
int king_shape[7][7];

int black_captured[16], white_captured[16], check_path[8], captured_rooks[4], crt = -1, cpt = -1, wct = -1, bct = -1;
const int MAX_CAPTURED_INDEX = 15;
int black_king_pos = 04, white_king_pos = 74, autosave = 0, file_id = 0, white_move = 1;
char file_name[100];
const int WHITE = 1, BLACK = 2, PAWN = 6, ROOK = 7, BISHOP = 8, KNIGHT = 5, KING = 3, QUEEN = 4;
int white_material = 144, black_material = 144, black_square_bishops = 2, white_square_bishops = 2, other_coins = 26;
const int N = -10, S = 10, E = 1, W = -1, NW = -11, NE = -9, SW = 9, SE = 11;
const int NORMAL = 1, SHORT_CASTLE = 2, LONG_CASTLE = 3, EN_PASSANT = 4;
int black_king_moves = 0, white_king_moves = 0;
int steps_to_edges[8][8][8];
const int direction_off_set[] = {N, E, W, S, NE, NW, SE, SW};

void rook();
void queen();
void bishop();
void print_empty_row();
void display_name_board();
void display_captured(int *captured, int top);
void pawn();
void print_white_space();
void knight();
void king();
void print_line();
void display_row(int board[][7], int row, int color);
void coin_shape(int coin, int row, int color);
void display_board();
void num_to_char(int num);
void chess_board();
void display_positions();
int is_news_path_clear(int from, int to);
int is_check_after_move(int from, int to, int move_type);
int can_promote_pawn(int pos);
void promote_pawn(int pos);
int can_castle(int color, int from, int to);
void construct();
void destruct();
int is_news_move(int from, int to);
void add_rook_info(int current_position, int previous_position, int captured, int undone);
void un_capture_rook();
void add_piece(int coin, int position);
void remove_piece(int to);
int validate_move(int from, int to);
int* generate_knight_moves(int pos);
int is_valid_pos(int pos);
int can_move_news(int coin);
int move(struct move_node *validated_move);
int undo();

int row(int pos)
{
    return pos / 10;
}

int column(int pos)
{
    return pos % 10;
}

int Coin(int pos)
{
    return board[row(pos)][column(pos)];
}

int color(int coin)
{
    return coin / 10;
}

int coin_type(int coin)
{
    return coin % 10;
}

int pos_hash(int coin)
{
    return coin % 6;
}

void swap(int *x, int *y)
{
    int t = *x;
    *x = *y;
    *y = t;
}

struct pos_node** get_positions(int color)
{
    if(color == BLACK)
    {
        return black_positions;
    }
    return white_positions;
}

void add_position(int coin, int current_position, int previous_position)
{
    int hash = pos_hash(coin);
    struct pos_node *new_node, *current_node, **positions;
    positions = get_positions(color(coin));

    if(positions[hash] == NULL)
    {
        new_node = (struct pos_node*)malloc(sizeof(struct pos_node));
        if(new_node == NULL)
        {
            return;
        }
        new_node->pos = current_position;
        new_node->next = NULL;
        positions[hash] = new_node;
    }
    else
    {
        current_node = positions[hash];
        while(current_node->pos != previous_position && current_node->next != NULL)
        {
            current_node = current_node->next;
        }
        if(current_node->pos == previous_position)
        {
            current_node->pos = current_position;
        }
        else if(current_node->next == NULL)
        {
            new_node = (struct pos_node*)malloc(sizeof(struct pos_node));
            if(new_node == NULL)
            {
                return;
            }
            new_node->pos = current_position;
            new_node->next = NULL;
            current_node->next = new_node;
        }
    }
}

void delete_position(int coin, int pos)
{
    struct pos_node **positions;
    int hash = pos_hash(coin);
    positions = get_positions(color(coin));

    if(positions[hash] == NULL)
    {
        return;
    }

    struct pos_node *current = positions[hash], *previous = NULL;

    while(current != NULL && current->pos != pos)
    {
        previous = current;
        current = current->next;
    }

    if(current == NULL)
    {
        return;
    }

    if(previous == NULL)
    {
        positions[hash] = current->next;
    }
    else
    {
        previous->next = current->next;
    }

    free(current);
}

void init_hash_table()
{
    for(int i = 0; i <= 7; i++)
    {
        for(int j = 0; j <= 7; j++)
        {
            if(board[i][j] != 0)
            {
                add_position(board[i][j], i * 10 + j, -1);
            }
        }
    }
}

struct log_node* create_log_node(int from, int from_coin, int to, int to_coin, int move_type)
{
    struct log_node *new_node = (struct log_node*)malloc(sizeof(struct log_node));

    if(new_node == NULL)
    {
        return NULL;
    }
    
    new_node->from = from;
    new_node->to = to;
    new_node->from_coin = from_coin;
    new_node->to_coin = to_coin;
    new_node->move_type = move_type;

    return new_node;
}

void push_log(struct log_node **move_log, struct log_node *new_node)
{
    if(new_node == NULL)
    {
        return;
    }

    new_node->next = *move_log;
    *move_log = new_node;
}

struct log_node* pop_log()
{
    if(move_log == NULL)
    {
        return NULL;
    }

    struct log_node *temp = move_log;
    move_log = move_log->next;
    temp->next = NULL;

    return temp;
}

void push_captured(int color, int coin)
{
    if(color == WHITE)
    {
        if(wct < MAX_CAPTURED_INDEX)
        {
            white_captured[++wct] = coin;
        }
        else
        {
            printf("captured stack overflow\n");
            exit(0);
        }
    }
    else
    {
        if(bct < MAX_CAPTURED_INDEX)
        {
            black_captured[++bct] = coin;
        }
        else
        {
            printf("captured stack overflow\n");
            exit(0);
        }
    }
}

void pop_captured(int color)
{
    if(color == WHITE)
    {
        if(wct > -1)
        {
            wct--;
        }
        else
        {
            printf("No white coins captured\n");
        }
    }
    else
    {
        if(bct > -1)
        {
            bct--;
        }
        else
        {
            printf("No black coins captured\n");
        }
    }
}

void add_move_count_node(int count)
{
    struct move_count *new_node = (struct move_count*)malloc(sizeof(struct move_count));

    if(new_node == NULL)
    {
        return;
    }

    new_node->count = count;
    new_node->next = head;
    head = new_node;
}

void increment_move_count()
{
    if(head == NULL)
    {
        add_move_count_node(1);
    }
    else
    {
        head->count++;
    }
}

void reset_move_count()
{
    add_move_count_node(0);
}

void pop_move_count()
{
    if(head == NULL)
    {
        return;
    }
    else if(head->count == 0)
    {
        struct move_count *temp = head;
        head = head->next;
        free(temp);
    }
    else
    {
        head->count--;
    }
}

void add_coin_count(int coin_type, int pos)
{
    if(coin_type == BISHOP)
    {
        if((row(pos) + column(pos)) % 2 == 0)
        {
            white_square_bishops++;
        }
        else
        {
            black_square_bishops++;
        }
    }
    else
    {
        other_coins++;
    }
}

void sub_coin_count(int coin_type, int pos)
{
    if(coin_type == BISHOP)
    {
        if((row(pos) + column(pos)) % 2 == 0)
        {
            white_square_bishops--;
        }
        else
        {
            black_square_bishops--;
        }
    }
    else
    {
        other_coins--;
    }
}

int value(int coin_type)
{
    if(coin_type == QUEEN || coin_type == ROOK || coin_type == PAWN)
    {
        return 10;
    }
    else if(coin_type == KNIGHT)
    {
        return 9;
    }
    else if(coin_type == BISHOP)
    {
        return 8;
    }
    return 0;
}

void add_material(int coin)
{
    if(color(coin) == BLACK)
    {
        black_material += value(coin_type(coin));
    }
    else
    {
        white_material += value(coin_type(coin));
    }
}

void sub_material(int coin)
{
    if(color(coin) == BLACK)
    {
        black_material -= value(coin_type(coin));
    }
    else
    {
        white_material -= value(coin_type(coin));
    }
}

struct move_node* create_move_node(int from, int to, int move_type)
{
    struct move_node *new_node = (struct move_node*)malloc(sizeof(struct move_node));
    if(new_node == NULL)
    {
        printf("memory not allocated\n");
        return NULL;
    }

    new_node->from = from;
    new_node->to = to;
    new_node->move_type = move_type;
    new_node->next = NULL;
    return new_node;
}

struct queue* init_queue()
{
    struct queue *q = (struct queue*)malloc(sizeof(struct queue));
    if(q == NULL)
    {
        printf("memory not allocated\n");
        return NULL;
    }
    q->front = q->rear = NULL;
    return q;
}

void enqueue(struct queue *q, struct move_node *new_node)
{
    if(new_node == NULL)
    {
        return;
    }

    if(q->rear == NULL)
    {
        q->front = q->rear = new_node;
    }
    else
    {
        q->rear->next = new_node;
        q->rear = q->rear->next;
    }
}

struct move_node* dequeue(struct queue *q)
{
    if(q->front == NULL)
    {
        return NULL;
    }

    if(q->front->next == NULL)
    {
        struct move_node *temp = q->front;
        q->front = q->rear = NULL;
        return temp;
    }

    struct move_node *temp = q->front;
    q->front = q->front->next;
    temp->next = NULL;
    return temp;
}

void free_queue(struct queue *q)
{
    struct move_node *current_node = q->front;
    struct move_node *previous_node;
    while(current_node != NULL)
    {
        previous_node = current_node;
        current_node = current_node->next;
        free(previous_node);
    }
    free(q);
}

int min(int x, int y)
{
    if(x > y)
    {
        return y;
    }
    return x;
}

void generate_steps_to_edges()
{
    int north, east, west, south, north_east, north_west, south_east, south_west;
    for(int row = 0; row < 8; row++)
    {
        for(int col = 0; col < 8; col++)
        {
            north = row;
            east = 7 - col;
            west = col;
            south = 7 - row;
            north_east = min(north, east);
            north_west = min(north, west);
            south_east = min(south, east);
            south_west = min(south, west);
            
            steps_to_edges[row][col][0] = north;
            steps_to_edges[row][col][1] = east;
            steps_to_edges[row][col][2] = west;
            steps_to_edges[row][col][3] = south;
            steps_to_edges[row][col][4] = north_east;
            steps_to_edges[row][col][5] = north_west;
            steps_to_edges[row][col][6] = south_east;
            steps_to_edges[row][col][7] = south_west;
        }
    }
}

void generate_valid_pawn_moves(int color, int position, struct queue *q)
{
    int move_type;
    if(color == WHITE)
    {
        if(move_type = validate_move(position, position + N))
        {
            enqueue(q, create_move_node(position, position + N, move_type));
        }
        if(move_type = validate_move(position, position + NE))
        {
            enqueue(q, create_move_node(position, position + NE, move_type));
        }
        if(move_type = validate_move(position, position + NW))
        {
            enqueue(q, create_move_node(position, position + NW, move_type));
        }
        if(row(position) == 6 && validate_move(position, position + N * 2))
        {
            enqueue(q, create_move_node(position, position + N * 2, NORMAL));
        }
    }
    else
    {
        if(move_type = validate_move(position, position + S))
        {
            enqueue(q, create_move_node(position, position + S, move_type));
        }
        if(move_type = validate_move(position, position + SE))
        {
            enqueue(q, create_move_node(position, position + SE, move_type));
        }
        if(move_type = validate_move(position, position + SW))
        {
            enqueue(q, create_move_node(position, position + SW, move_type));
        }
        if(row(position) == 1 && validate_move(position, position + S * 2))
        {
            enqueue(q, create_move_node(position, position + S * 2, NORMAL));
        }
    }
}

void generate_valid_king_moves(int color, int position, struct queue *q)
{
    for(int i = 0; i < 8; i++)
    {
        if(validate_move(position, position + direction_off_set[i]))
        {
            enqueue(q, create_move_node(position, position + direction_off_set[i], NORMAL));
        }
    }
    
    int move_type;
    if((color == WHITE && position == 74) || (color == BLACK && position == 4))
    {
        if(move_type = validate_move(position, position + 2 * E))
        {
            enqueue(q, create_move_node(position, position + 2 * E, move_type));
        }
        if(move_type = validate_move(position, position + 2 * W))
        {
            enqueue(q, create_move_node(position, position + 2 * W, move_type));
        }
    }
}

void generate_valid_knight_moves(int position, struct queue *q)
{
    int *knight_moves = generate_knight_moves(position);
    if(knight_moves == NULL)
    {
        return;
    }

    for(int i = 0; i < 8; i++)
    {
        if(is_valid_pos(knight_moves[i]) && (color(Coin(position)) != color(Coin(knight_moves[i])))  &&
         !is_check_after_move(position, knight_moves[i], NORMAL))
        {
            enqueue(q, create_move_node(position, knight_moves[i], NORMAL));
        }
    }
    free(knight_moves);
}

void generate_valid_sliding_moves(int position, struct queue *q)
{
    int coin = Coin(position);
    int type = coin_type(coin);
    int start_direction, end_direction;
    
    if(type == BISHOP)
    {
        start_direction = 4;
        end_direction = 7;
    }
    else if(type == ROOK)
    {
        start_direction = 0;
        end_direction = 3;
    }
    else if(type == QUEEN)
    {
        start_direction = 0;
        end_direction = 7;
    }

    int max_steps, current_position;
    for(int i = start_direction; i <= end_direction; i++)
    {   
        current_position = position;
        max_steps = steps_to_edges[row(position)][column(position)][i];

        for(int steps = 1; steps <= max_steps; steps++)
        {
            current_position += direction_off_set[i];
            if(validate_move(position, current_position))
            {
                enqueue(q, create_move_node(position, current_position, NORMAL));
            }

            if(Coin(current_position))
            {
                break;
            }
        }
    }
}

struct queue* generate_valid_moves(int color)
{
    struct pos_node **positions = get_positions(color);
    struct pos_node *current_position;
    struct queue *list = init_queue();
    if(list == NULL)
    {
        return NULL;
    }

    int type;
    for(int i = 0; i < 6; i++)
    {
        current_position = positions[i];
        while(current_position != NULL)
        {
            type = coin_type(Coin(current_position->pos));
            if(type == PAWN)
            {
                generate_valid_pawn_moves(color, current_position->pos, list);
            }
            else if(type == KING)
            {
                generate_valid_king_moves(color, current_position->pos, list);
            }
            else if(type == KNIGHT)
            {
                generate_valid_knight_moves(current_position->pos, list);
            }
            else
            {
                generate_valid_sliding_moves(current_position->pos, list);
            }
            current_position = current_position->next;
        }
    }
    return list;
}

int op_color(int color)
{
    if(color == WHITE)
    {
        return BLACK;
    }
    return WHITE;
}

int moves_in_depth(int color, int depth)
{
    if(depth == 0)
    {
        return 1;
    }
    long int count = 0;
    struct queue *q;
    q = generate_valid_moves(color);
    if(q == NULL)
    {
        return 0;
    }
    struct move_node *mv = q->front;
    while(mv != NULL)
    {
        move(mv);
        mv = mv->next;
        count += moves_in_depth(op_color(color), depth - 1);
        undo();
    }
    free_queue(q);
    return count;
}

int queue_length(struct queue *q)
{
    struct move_node *temp = q->front;
    int count = 0;
    while(temp != NULL)
    {
        temp = temp->next;
        count++;
    }
    return count;
}

void push_check_path(int pos)
{
    check_path[++cpt] = pos;
}

int can_move_news(int coin)
{
    int type = coin_type(coin);
    return type != KNIGHT && type != BISHOP;
}

int is_en_passant(int from, int to)
{
    int last_move_not_made_by_pawn = move_log == NULL || coin_type(move_log->from_coin) != PAWN;
    if(last_move_not_made_by_pawn)
    {
        return 0;
    }
    
    int last_mv_steps = row(move_log->from) - row(move_log->to);
    int coin_color = color(Coin(from));
    if(coin_color == WHITE && last_mv_steps == -2 && to + S == move_log->to)
    {
        return EN_PASSANT;
    }
    else if(coin_color == BLACK && last_mv_steps == 2 && to + N == move_log->to)
    {
        return EN_PASSANT;
    }
    return 0;
}

int validate_pawn_move(int from, int to)
{
    int from_coin_color = color(Coin(from));
    int to_coin = Coin(to);
    int row_steps = row(from) - row(to);
    int column_steps = column(from) - column(to);
    int straight_move = column_steps == 0;
    
    if(from_coin_color == WHITE)
    {
        if(straight_move && to_coin == 0)
        {
            if(row_steps == 1)
            {
                return !is_check_after_move(from, to, NORMAL);
            }
            else if(row_steps == 2 && row(from) == 6 && is_news_path_clear(from, to))
            {
                return !is_check_after_move(from, to, NORMAL);
            }
        }
        else if(row_steps == 1 && (column_steps == 1 || column_steps == -1))
        {
            if(to_coin != 0)
            {
                return !is_check_after_move(from, to, NORMAL);
            }
            else if(row(from) == 3 && is_en_passant(from, to) && !is_check_after_move(from, to, EN_PASSANT))
            {
                return EN_PASSANT;
            }
        }
    }
    else
    {
        if(straight_move && to_coin == 0)
        {
            if(row_steps == -1)
            {
                return !is_check_after_move(from, to, NORMAL);
            }
            else if(row_steps == -2 && row(from) == 1 && is_news_path_clear(from, to))
            {
                return !is_check_after_move(from, to, NORMAL);
            }
        }
        else if(row_steps == -1 && (column_steps == 1 || column_steps == -1))
        {
            if(to_coin != 0)
            {
                return !is_check_after_move(from, to, NORMAL);
            }
            else if(row(from) == 4 && is_en_passant(from, to) && !is_check_after_move(from, to, EN_PASSANT))
            {
                return EN_PASSANT;
            }
        }
    }

    return 0;
}

int is_valid_pos(int pos)
{
    int r = row(pos);
    int c = column(pos);
    return r >= 0 && r < 8 && c < 8 && c >= 0;
}

int get_king_moves(int color)
{
    if(color == BLACK)
    {
        return black_king_moves;
    }
    return white_king_moves;
}

int validate_king_move(int from, int to)
{
    int row_steps = row(from) - row(to);
    int column_steps = column(from) - column(to);
    int news_move = row_steps == 0 || column_steps == 0;
    int cross_move = row_steps == column_steps || row_steps == -column_steps;

    if(!(news_move || cross_move))
    {
        return 0;
    }

    if(row_steps == 1 || row_steps == -1 || column_steps == 1 || column_steps == -1)
    {
        return !is_check_after_move(from, to, NORMAL);
    }
    else if(row_steps == 0 && (column_steps == 2 || column_steps == -2) && get_king_moves(color(Coin(from))) == 0)
    {
        return can_castle(color(Coin(from)), from, to);
    }
    return 0;
}

int steps_limit(int from, int to)
{
    int from_coin_color = color(Coin(from));
    int from_coin_type = coin_type(Coin(from));
    int row_steps = row(from) - row(to);
    int column_steps = column(from) - column(to);

    if(from_coin_type == KING)
    {
        return row_steps == 1 || row_steps == -1 || column_steps == 1 || column_steps == -1;
    }
    else if(from_coin_type == PAWN)
    {
        if(from_coin_color == WHITE)
        {
            return row_steps == 1 && column_steps && color(Coin(to)) == BLACK;
        }
        return row_steps == -1 && column_steps && color(Coin(to)) == WHITE;
    }
    return 1;
}

int is_path_clear(int from, int to, int direction)
{
    for(int current_position = from + direction; current_position < to; current_position += direction)
    {
        if(Coin(current_position))
        {
            return 0;
        }
    }
    return 1;
}

int is_news_path_clear(int from, int to)
{
    if(from > to)
    {
        swap(&from, &to);
    }

    if(column(from) == column(to))
    {
        return is_path_clear(from, to, S);
    }
    return is_path_clear(from, to, E);
}

int is_cross_path_clear(int from, int to)
{
    if(from > to)
    {
        swap(&from, &to);
    }

    if(column(from) < column(to))
    {
        return is_path_clear(from, to, SE);
    }
    return is_path_clear(from, to, SW);
}

int is_cross_move(int from, int to)
{
    int row_steps = row(from) - row(to);
    int column_steps = column(from) - column(to);

    return row_steps == column_steps || row_steps == -column_steps;
}

int can_move_cross(int coin)
{
   return coin_type(coin) != KNIGHT && coin_type(coin) != ROOK;
}

int* generate_knight_moves(int pos)
{
    int *knight_moves = (int*)malloc(sizeof(int) * 8);
    if(knight_moves == NULL)
    {
        return NULL;
    }

    knight_moves[0] = pos + S + S + E;
    knight_moves[1] = pos + N + N + E;
    knight_moves[2] = pos + N + N + W;
    knight_moves[3] = pos + S + S + W;
    knight_moves[4] = pos + S + E + E;
    knight_moves[5] = pos + N + E + E;
    knight_moves[6] = pos + N + W + W;
    knight_moves[7] = pos + S + W + W;
    return knight_moves;
}

int is_knight_move(int from, int to)
{
    int *moves = generate_knight_moves(from);
    if(moves == NULL)
    {
        printf("memory not allocated\n");
        return 0;
    }
    for(int i = 0; i < 8; i++)
    {
        if(moves[i] == to)
        {
            free(moves);
            return 1;
        }
    }
    free(moves);
    return 0;
}

int is_news_move(int from, int to)
{
    return row(from) == row(to) || column(from) == column(to);
}

int validate_move(int from, int to)
{
    if(!is_valid_pos(from) || !is_valid_pos(to))
    {
        return 0;
    }

    int from_coin = Coin(from);
    int from_coin_type = coin_type(from_coin);

    if(from_coin == 0)
    {
        return 0;
    }
    if(color(from_coin) == color(Coin(to)))
    {
        return 0;
    }

    if(from_coin_type == PAWN)
    {
        return validate_pawn_move(from, to);
    }
    else if(from_coin_type == KING)
    {
        return validate_king_move(from, to);
    }
    // else if(from_coin_type == KNIGHT)
    // {
    //     return is_knight_move(from, to) && !is_check_after_move(from, to, NORMAL);
    // }
    // else if(can_move_news(from_coin) && is_news_move(from, to))
    else
    {
        // return is_news_path_clear(from, to) && !is_check_after_move(from, to, NORMAL);
        return !is_check_after_move(from, to, NORMAL);

    }
    // else if(can_move_cross(from_coin) && is_cross_move(from, to))
    // {
    //     return is_cross_path_clear(from, to) && !is_check_after_move(from, to, NORMAL);
    // }
    // return 0;
}

int can_move(int direction, int coin)
{
	if(direction == N || direction == E || direction == W || direction == S)
	{
		return can_move_news(coin);
	}
	return can_move_cross(coin);
}

int is_check_path(int king_position, int steps, int direction, int track_path)
{
    int king_color = color(Coin(king_position)), current_position = king_position;
    cpt = -1;

    for(int i = 1; i <= steps; i++)
    {
        current_position += direction;

        if(track_path)
        {
            push_check_path(current_position);
        }

        if(Coin(current_position))
        {
            if(color(Coin(current_position)) != king_color)
            {
                return can_move(direction, Coin(current_position)) && steps_limit(current_position, king_position);
            }
            return 0;
        }
    }

    return 0;
}

int is_check(int king_color, int king_position, int track_path)
{
    int row_pos = row(king_position), col_pos = column(king_position);
    //north
    int steps = row_pos;
    if(is_check_path(king_position, steps, N, track_path))
    {
        return check_path[cpt];
    }
    //south
    steps = 7 - row_pos;
    if(is_check_path(king_position, steps, S, track_path))
    {
        return check_path[cpt];
    }
    //East
    steps = 7 - col_pos;
    if(is_check_path(king_position, steps, E, track_path))
    {
        return check_path[cpt];
    }
    //west
    steps = col_pos;
    if(is_check_path(king_position, steps, W, track_path))
    {
        return check_path[cpt];
    }
    //north west
    int min, max;
    if(row_pos < col_pos)
    {
        min = row_pos;
        max = col_pos;
    }
    else
    {
        min = col_pos;
        max = row_pos;
    }
    if(is_check_path(king_position, min, NW, track_path))
    {
        return check_path[cpt];
    }
    //south east mv
    if(is_check_path(king_position, 7 - max, SE, track_path))
    {
        return check_path[cpt];
    }
    //north east
    if((row_pos + col_pos) < 7)
    {
        steps = row_pos;
    }
    else
    {
        steps = 7 - col_pos;
    }
    if(is_check_path(king_position, steps, NE, track_path))
    {
        return check_path[cpt];
    }
    //south west
    if((row_pos + col_pos) < 7)
    {
        steps = col_pos;
    }
    else
    {
        steps = 7 - row_pos;
    }
    if(is_check_path(king_position, steps, SW, track_path))
    {
        return check_path[cpt];
    }
    //knight check
    cpt = -1;
    int op_color;
    if(king_color == WHITE)
    {
        op_color = BLACK;
    }
    else
    {
        op_color = WHITE;
    }
    int *moves = generate_knight_moves(king_position);
    if(moves == NULL)
    {
        printf("memory not allocated\n");
        return 0;
    }
    for(int i = 0; i < 8; i++)
    {
        if(is_valid_pos(moves[i]) && Coin(moves[i]) == op_color * 10 + KNIGHT)
        {
            push_check_path(moves[i]);
            free(moves);
            return check_path[cpt];
        }
    }
    free(moves);
    return -1;
}

int is_check_after_move(int from, int to, int move_type)
{
    int from_coin = Coin(from), to_coin = Coin(to), king_position;

    if(coin_type(from_coin) == KING)
    {
        king_position = to;
    }
    else if(color(from_coin) == WHITE)
    {
        king_position = white_king_pos;
    }
    else
    {
        king_position = black_king_pos;
    }

    board[row(to)][column(to)] = board[row(from)][column(from)];
    board[row(from)][column(from)] = 0;
    int en_pass_pos, en_coin;

    if(move_type == EN_PASSANT)
    {
        if(color(from_coin) == BLACK)
        {
            en_pass_pos = to + N;
            en_coin = Coin(en_pass_pos);
            board[row(to + N)][column(to + N)] = 0;
        }
        else
        {
            en_pass_pos = to + S;
            en_coin = Coin(en_pass_pos);
            board[row(to + S)][column(to + S)] = 0;
        }
    }

    int check = is_check(color(from_coin), king_position, 0);

    board[row(from)][column(from)] = from_coin;
    board[row(to)][column(to)] = to_coin;

    if(move_type == EN_PASSANT)
    {
        board[row(en_pass_pos)][column(en_pass_pos)] = en_coin;
    }

    return check != -1;
}

void remove_piece(int to)
{
    sub_material(Coin(to));
    sub_coin_count(coin_type(Coin(to)), to);
    delete_position(Coin(to), to);
}

void add_piece(int coin, int position)
{
    add_position(coin, position, -1);
    add_material(coin);
    add_coin_count(coin_type(coin), position);
}

void en_passant(int from, int to)
{
    int captured_pawn_pos;

    if(color(Coin(from)) == BLACK)
    {
        captured_pawn_pos = to + N;
    }
    else
    {
        captured_pawn_pos = to + S;
    }

    remove_piece(captured_pawn_pos);
    push_captured(color(Coin(captured_pawn_pos)), Coin(captured_pawn_pos));
    board[row(captured_pawn_pos)][column(captured_pawn_pos)] = 0;
}

void un_en_passant(struct log_node *temp)
{
    int captured_pawn_pos, coin_color;

    if(color(temp->from_coin) == BLACK)
    {
        coin_color = WHITE;
        captured_pawn_pos = temp->to + N;
    }
    else
    {   coin_color = BLACK;
        captured_pawn_pos = temp->to + S;
    }

    board[row(captured_pawn_pos)][column(captured_pawn_pos)] = coin_color * 10 + PAWN;
    add_piece(Coin(captured_pawn_pos), captured_pawn_pos);
    pop_captured(coin_color);
}

int rook_hash(int x)
{
    return x % 4;
}

void init_rook_info()
{
    rooks[rook_hash(0)].pos = 0;
    rooks[rook_hash(7)].pos = 7;
    rooks[rook_hash(70)].pos = 70;
    rooks[rook_hash(77)].pos = 77;
    rooks[0].captured = rooks[1].captured = rooks[2].captured = rooks[3].captured = 0;
    rooks[0].moves = rooks[1].moves = rooks[2].moves = rooks[3].moves = 0;
}

void un_capture_rook()
{
    rooks[captured_rooks[crt--]].captured = 0;
}

void add_rook_info(int current_position, int previous_position, int captured, int undone)
{
    for(int i = 0; i < 4; i++)
    {
        if(rooks[i].pos == previous_position && rooks[i].captured == 0)
        {
            rooks[i].pos = current_position;
            rooks[i].captured = captured;
            if(!captured)
            {
                if(undone)
                {
                    rooks[i].moves--;
                }
                else
                {
                    rooks[i].moves++;
                }
            }
            else
            {
                captured_rooks[++crt] = i;
            }
            break;
        }
    }
}

int can_castle(int color, int from, int to)
{
    int steps = column(from) - column(to);
    if(color == BLACK && black_king_moves == 0)
    {
        if(steps == -2 && rooks[rook_hash(7)].captured == 0 && rooks[rook_hash(7)].moves == 0 && is_news_path_clear(from, to))
        {
            if(is_check(color, black_king_pos, 0) == -1 && !is_check_after_move(from, from + E, NORMAL) && !is_check_after_move(from, from + 2 * E, NORMAL))
            {
                return SHORT_CASTLE;
            }
        }
        else if(steps == 2 && rooks[rook_hash(0)].captured == 0 && rooks[rook_hash(0)].moves == 0 && is_news_path_clear(from, to + 2 * W))
        {
            if(is_check(color, black_king_pos, 0) == -1 && !is_check_after_move(from, from + W, NORMAL) && !is_check_after_move(from, from + 2 * W, NORMAL))
            {
                return LONG_CASTLE;
            }
        }
    }
    else if(color == WHITE && white_king_moves == 0)
    {
        if(steps == -2 && rooks[rook_hash(77)].captured == 0 && rooks[rook_hash(77)].moves == 0 && is_news_path_clear(from, to))
        {
            if(is_check(color, white_king_pos, 0) == -1 && !is_check_after_move(from, from + E, NORMAL) && !is_check_after_move(from, from + 2 * E, NORMAL))
            {
                return SHORT_CASTLE;
            }
        }
        else if(steps == 2 && rooks[rook_hash(70)].captured == 0 && rooks[rook_hash(70)].moves == 0 && is_news_path_clear(from, to + 2 * W))
        {
            if(is_check(color, white_king_pos, 0) == -1 && !is_check_after_move(from, from + W, NORMAL) && !is_check_after_move(from, from + 2 * W, NORMAL))
            {
                return LONG_CASTLE;
            }
        }
    }
    return 0;
}

void update_position_and_rook_info(int coin, int from, int to, int undone)
{
    add_position(coin, to, from);
    add_rook_info(to, from, 0, undone);
    board[row(from)][column(from)] = 0;
    board[row(to)][column(to)] = color(coin) * 10 + ROOK;
}

void castle(int color, int castle_type)
{
    if(color == BLACK)
    {
        if(castle_type == SHORT_CASTLE)
        {
            update_position_and_rook_info(Coin(7), 7, 5, 0);
        }
        else
        {
            update_position_and_rook_info(Coin(0), 0, 3, 0);
        }
    }
    else
    {
        if(castle_type == SHORT_CASTLE)
        {
            update_position_and_rook_info(Coin(77), 77, 75, 0);
        }
        else
        {
            update_position_and_rook_info(Coin(70), 70, 73, 0);
        }
    }
}

void un_castle(struct log_node *temp)
{
    if(color(temp->from_coin) == BLACK)
    {
        if(temp->move_type == SHORT_CASTLE)
        {
            update_position_and_rook_info(Coin(5), 5, 7, 1);
        }
        else
        {
            update_position_and_rook_info(Coin(3), 3, 0, 1);
        }
    }
    else
    {
        if(temp->move_type == SHORT_CASTLE)
        {
            update_position_and_rook_info(Coin(75), 75, 77, 1);
        }
        else
        {
            update_position_and_rook_info(Coin(73), 73, 70, 1);
        }
    }
}

int move(struct move_node *validated_move)
{
    int from = validated_move->from, to = validated_move->to;
    int move_type = validated_move->move_type;
    //int move_type = validate_move(from, to);
    if(!move_type)
    {
        return 0;
    }

    int from_coin = Coin(from);
    int to_coin = Coin(to);
    int from_coin_type = coin_type(from_coin);

    push_log(&move_log, create_log_node(from, from_coin, to, to_coin, move_type));
    add_position(from_coin, to, from);
    
    if(to_coin != 0)
    {
        remove_piece(to);
        push_captured(color(to_coin), to_coin);
    }
    else if(move_type == EN_PASSANT)
    {
        en_passant(from, to);
    }
    else if(move_type == SHORT_CASTLE || move_type == LONG_CASTLE)
    {
        castle(color(from_coin), move_type);
    }

    if(to_coin != 0 || from_coin_type == PAWN)
    {
        reset_move_count();
    }
    else
    {
        increment_move_count();
    }

    if(coin_type(to_coin) == ROOK)
    {
        add_rook_info(to, to, 1, 0);
    }

    board[row(to)][column(to)] = from_coin;
    board[row(from)][column(from)] = 0;

    if(from_coin_type == PAWN && can_promote_pawn(to))
    {
        promote_pawn(to);
    }
    else if(from_coin_type == KING)
    {
        if(color(from_coin) == WHITE)
        {
            white_king_pos = to;
            white_king_moves++;
        }
        else
        {
            black_king_pos = to;
            black_king_moves++;
        }
    }
    else if(from_coin_type == ROOK)
    {
        add_rook_info(to, from, 0, 0);
    }
    return 1;
}

int undo()
{
    struct log_node *temp = pop_log();
    if(!temp)
    {
        return 0;
    }

    if(coin_type(temp->from_coin) == PAWN && can_promote_pawn(temp->to))
    {
        add_piece(temp->from_coin, temp->from);
        remove_piece(temp->to);
    }
    else
    {
        add_position(temp->from_coin, temp->from, temp->to);
    }

    if(coin_type(temp->from_coin) == ROOK)
    {
        add_rook_info(temp->from, temp->to, 0, 1);
    }
    else if(coin_type(temp->from_coin) == KING)
    {
        if(color(temp->from_coin) == WHITE)
        {
            white_king_pos = temp->from;
            white_king_moves--;
        }
        else
        {
            black_king_pos = temp->from;
            black_king_moves--;
        }
    }

    if(temp->to_coin != 0)
    {
        add_piece(temp->to_coin, temp->to);
        pop_captured(color(temp->to_coin));
    }
    else if(temp->move_type == EN_PASSANT)
    {
        un_en_passant(temp);
    }
    else if(temp->move_type == SHORT_CASTLE || temp->move_type == LONG_CASTLE)
    {
        un_castle(temp);
    }

    if(coin_type(temp->to_coin) == ROOK)
    {
        un_capture_rook();
    }

    pop_move_count();
    board[row(temp->from)][column(temp->from)] = temp->from_coin;
    board[row(temp->to)][column(temp->to)] = temp->to_coin;   
    free(temp);
    return 1;
}

int can_promote_pawn(int pos)
{
    return row(pos) == 0 || row(pos) == 7;
}

void promote_pawn(int pos)
{
    int coin_type;
    while(1)
    {
        printf("4.queen, 5.knight, 7.rook, 8.bishop ");
        scanf("%d", &coin_type);
        if(coin_type == QUEEN || coin_type == KNIGHT || coin_type == ROOK || coin_type == BISHOP)
        {
            remove_piece(pos);
            board[row(pos)][column(pos)] = color(Coin(pos)) * 10 + coin_type;
            add_piece(Coin(pos), pos);
            break;
        }
    }
}

int cover_check(int pos, int color)
{
    struct pos_node **positions, *current;
    positions = get_positions(color);

    for(int i = 0; i < 6; i++)
    {
        current = positions[i];
        while(current != NULL)
        {
            if(coin_type(Coin(current->pos)) != KING && validate_move(current->pos, pos))
            {
                return Coin(current->pos);
            }
            current = current->next;
        }
    }
    return 0;
}

int en_passant_cover(int position, int color)
{
    if(color == BLACK && is_valid_pos(position + S) && !Coin(position + S))
    {
        if(is_valid_pos(position + W) && Coin(position + W) == BLACK * 10 + PAWN &&
            validate_pawn_move(position + W, position + S) == EN_PASSANT)
        {
            return 1;
        }
        if(is_valid_pos(position + E) && Coin(position + E) == BLACK * 10 + PAWN && 
            validate_pawn_move(position + E, position + S) == EN_PASSANT)
        {
            return 1;
        }
    }
    else if(color == WHITE && is_valid_pos(position + N) && !Coin(position + N))
    {
        if(is_valid_pos(position + W) && Coin(position + W) == WHITE * 10 + PAWN &&
            validate_pawn_move(position + W, position + N) == EN_PASSANT)
        {
            return 1;
        }
        if(is_valid_pos(position + E) && Coin(position + E) == WHITE * 10 + PAWN &&
            validate_pawn_move(position + E, position + N) == EN_PASSANT)
        {
            return 1;
        }
    }
    return 0;
}

int is_check_covered(int color)
{
    int cover_coin;
    if(coin_type(Coin(check_path[cpt])) == PAWN && en_passant_cover(check_path[cpt], color))
    {
        return PAWN;
    }
    for(int i = cpt; i >= 0; i--)
    {
        cover_coin = cover_check(check_path[i], color);
        if(cover_coin)
        {
            return cover_coin;
        }
    }
    return 0;
}

int one_news_move(int pos)
{
    return validate_move(pos, pos + N) || validate_move(pos, pos + S) || validate_move(pos, pos + W) || validate_move(pos, pos + E);
}

int one_cross_move(int pos)
{
    return validate_move(pos, pos + NE) || validate_move(pos, pos + NW) || validate_move(pos, pos + SE) || validate_move(pos, pos + SW);
}

int one_pawn_move(int pos)
{
    if(color(Coin(pos)) == BLACK)
    {
        return validate_move(pos, pos + S) || validate_move(pos, pos + SE) || validate_move(pos, pos + SW);
    }
    else
    {
        return validate_move(pos, pos + N) || validate_move(pos, pos + NE) || validate_move(pos, pos + NW);
    }
}

int one_knight_move(int clr, int pos)
{
    int *moves = generate_knight_moves(pos);
    if(moves == NULL)
    {
        printf("memory not allocated\n");
        return 0;
    }
    for(int i = 0; i < 8; i++)
    {
        if(is_valid_pos(moves[i]) && color(Coin(moves[i])) != clr && !is_check_after_move(pos, moves[i], NORMAL))
        {
            free(moves);
            return 1;
        }
    }
    free(moves);
    return 0;
}

int one_king_move(int pos)
{
    return one_news_move(pos) || one_cross_move(pos);
}

int one_move(int type, int pos)
{
    if(type == PAWN)
    {
        return one_pawn_move(pos);
    }
    else if(type == ROOK)
    {
       return one_news_move(pos);
    }
    else if(type == BISHOP)
    {
        return one_cross_move(pos);
    }
    else if(type == KNIGHT)
    {
        return one_knight_move(color(Coin(pos)), pos);
    }
    else if(type == QUEEN)
    {
        return one_news_move(pos) || one_cross_move(pos);
    }
    return 0;
}

int have_one_move(int color)
{
    struct pos_node **positions, *coins;
    positions = get_positions(color);

    for(int i = 0; i < 6; i++)
    {
        coins = positions[i];
        while(coins != NULL)
        {
            if(one_move(coin_type(Coin(coins->pos)), coins->pos))
            {
                return 1;
            }
            coins = coins->next;
        }
    }
    return 0;
}

int is_insufficient_material()
{
    if(black_material + white_material < 10)
    {
        return 1;
    }
    else if(!other_coins && (!white_square_bishops || !black_square_bishops))
    {
        return 1;
    }
    return 0;
}

int is_game_over(int color, int king_position)
{
    int king_can_move = one_king_move(king_position);
    
    if(!king_can_move && is_check(color, king_position, 1) != -1)
    {
        if(!is_check_covered(color))
        {
            display_name_board();
            if(color == BLACK)
            {
                printf("White won by checkmate\n");
            }
            else
            {
                printf("Black won by checkmate\n");
            }
            return 1;
        }
    }
    else if(!king_can_move && !have_one_move(color))
    {
        display_name_board();
        printf("Drawn by Stale mate\n");
        return 1;
    }
    else if(head && head->count == 100)
    {
        printf("Game drawn by the 50 move rule\n");
        return 1;
    }
    else if(is_insufficient_material())
    {
        display_name_board();
        printf("Drawn by insufficient material\n");
        return 1;
    }
    return 0;
}

void write_move_log(FILE *file, struct log_node *temp)
{
    if(temp != NULL)
    {
        write_move_log(file, temp->next);
        fprintf(file, "%d %d %d %d %d ", temp->from, temp->from_coin, temp->to, temp->to_coin, temp->move_type);
    }
}

void write_move_count(FILE *file, struct move_count *temp)
{
    if(temp != NULL)
    {
        write_move_count(file, temp->next);
        fprintf(file, "%d ", temp->count);
    }
}

void read_move_count(FILE *file)
{
    int count = 0;
    struct move_count *new_node;
    while(fscanf(file, "%d", &count) != EOF)
    {
        if(count == -1)
        {
            break;
        }
        if(head == NULL)
        {
            new_node = (struct move_count*)malloc(sizeof(struct move_count));
            new_node->count = count;
            head = new_node;
            head->next = NULL;
        }
        else
        {
            new_node = (struct move_count*)malloc(sizeof(struct move_count));
            new_node->count = count;
            new_node->next = head;
            head = new_node;
        }
    }
}

void read_board_positions(FILE *file)
{
    int pos, coin;
    while(fscanf(file, "%d %d", &pos, &coin) != EOF)
    {
        if(pos == -1)
        {
            break;
        }
        board[row(pos)][column(pos)] = coin;
    }
}

void read_captured_coins(FILE *file)
{
    int coin;
    while(fscanf(file, "%d ", &coin) != EOF)
    {
        if(coin == -1)
        {
            break;
        }
        push_captured(color(coin), coin);
    }
}

void read_move_log(FILE *file)
{
    int from, from_coin, to, to_coin, mv_type;
    while(fscanf(file, "%d %d %d %d %d", &from, &from_coin, &to, &to_coin, &mv_type) != EOF)
    {
        if(from == -1)
        {
            break;
        }
        push_log(&move_log, create_log_node(from, from_coin, to, to_coin, mv_type));
    }
}

void read_king_positions(FILE *file)
{
    fscanf(file, "%d %d", &white_king_pos, &black_king_pos);
}

void read_material(FILE *file)
{
    fscanf(file, "%d %d", &white_material, &black_material);
}

void read_rooks_info(FILE *file)
{
    for(int i = 0; i < 4; i++)
    {
        fscanf(file, "%d %d %d", &rooks[i].pos, &rooks[i].moves, &rooks[i].captured);
    }
}

void read_captured_rooks(FILE *file)
{
    int captured_index;
    while(fscanf(file, "%d ", &captured_index) != EOF)
    {
        if(captured_index == -1)
        {
            break;
        }
        captured_rooks[++crt] = captured_index;
    }
}

void read_king_moves(FILE *file)
{
    fscanf(file, "%d %d\n", &white_king_moves, &black_king_moves);
}

void read_coin_count(FILE *file)
{
    fscanf(file, "%d %d %d\n", &white_square_bishops, &black_square_bishops, &other_coins);
}

void read_board()
{
    FILE *file;

    file = fopen(file_name, "r");
    if(file == NULL)
    {
        printf("error opening the file\n");
        return;
    }

    read_board_positions(file);
    read_captured_coins(file);
    read_move_log(file);
    read_king_positions(file);
    read_material(file);
    read_rooks_info(file);
    read_captured_rooks(file);
    read_king_moves(file);
    read_coin_count(file);
    read_move_count(file);

    if(fclose(file) != 0)
    {
        printf("error closing the file\n");
    }
}

void write_board_positions(FILE *file)
{
    for(int i = 0; i < 8; i++)
    {
        for(int j = 0; j < 8; j++)
        {   
            if(board[i][j] != 0)
            {
                fprintf(file, "%d %d ", i * 10 + j, board[i][j]);
            }
        }
    }
    fprintf(file, "%d %d\n", -1, -1);
}

void write_captured_coins(FILE *file)
{
    for(int i = 0; i <= wct; i++)
    {
        fprintf(file, "%d ", white_captured[i]);
    }
    for(int i = 0; i <= bct; i++)
    {
        fprintf(file, "%d ", black_captured[i]);
    }
    fprintf(file, "%d\n", -1);
}

void write_king_positions(FILE *file)
{
    fprintf(file, "%d %d\n", white_king_pos, black_king_pos);
}

void write_material(FILE *file)
{
    fprintf(file, "%d %d\n", white_material, black_material);
}

void write_rooks_info(FILE *file)
{
    for(int i = 0; i < 4; i++)
    {
        fprintf(file, "%d %d %d ", rooks[i].pos, rooks[i].moves, rooks[i].captured);
    }
    fprintf(file, "\n");
}

void write_captured_rooks(FILE *file)
{
    for(int i = 0; i <= crt; i++)
    {
        fprintf(file, "%d ", captured_rooks[i]);
    }
    fprintf(file, "%d\n", -1);
}

void write_king_moves(FILE *file)
{
    fprintf(file, "%d %d\n", white_king_moves, black_king_moves);
}

void write_coin_count(FILE *file)
{
    fprintf(file, "%d %d %d\n", white_square_bishops, black_square_bishops, other_coins);
}

void write_board()
{
    FILE *file;

    file = fopen(file_name, "w");
    if(file == NULL)
    {
        printf("error opening the file\n");
        return;
    }

    write_board_positions(file);
    write_captured_coins(file);
    write_move_log(file, move_log);
    fprintf(file, "%d %d %d %d %d\n", -1, -1, -1, -1, -1);
    write_king_positions(file);
    write_material(file);
    write_rooks_info(file);
    write_captured_rooks(file);
    write_king_moves(file);
    write_coin_count(file);
    write_move_count(file, head);
    fprintf(file, "%d\n", -1);

    if(fclose(file) != 0)
    {
        printf("error closing the file\n");
    }
}

void start_game()
{
    int choice;
    while(1)
    {
        // printf("\n1.New game 2.continue 3.settings ");
        // scanf("%d", &choice);
        choice = 1;
        if(choice == 1)
        {
            // printf("enter any id(number) for this game ");
            // scanf("%d", &file_id);
            // sprintf(file_name, "%d.txt", file_id);
            chess_board();
            init_rook_info();
            init_hash_table();
            generate_steps_to_edges();
            break;
        }
        else if(choice == 2)
        {
            printf("enter id of previous game ");
            scanf("%d", &file_id);
            sprintf(file_name, "%d.txt", file_id);
            read_board();
            init_hash_table();
            if(move_log != NULL && color(move_log->from_coin) == WHITE)
            {
                white_move = 0;
                if(is_game_over(BLACK, black_king_pos))
                {
                    destruct();
                    exit(0);
                }
            }
            else
            {
                white_move == 1;
                if(is_game_over(WHITE, white_king_pos))
                {
                    destruct();
                    exit(0);
                }
            }
            break;
        }
        else if(choice == 3)
        {
            char as;
            printf("do you want to enable autosave? y/n");
            scanf(" %c", &as);
            if(as == 'y' || as == 'Y')
            {
                autosave = 1;
            }
            else if(as == 'n' || as == 'N')
            {
                printf("-2, -2 to save file\n");
                autosave = 0;
            }
            continue;
        }
        else
        {
            printf("invalid choice\n");
        }
    }
}

void free_positions(struct pos_node **positions)
{
    struct pos_node *previous, *current;
    for(int i = 0; i < 8; i++)
    {
        current = positions[i];
        while(current != NULL)
        {
            previous = current;
            current = current->next;
            free(previous);
        }
        positions[i] = NULL;
    }
}

void free_log_nodes(struct log_node *head)
{
    struct log_node *temp;
    while(head != NULL)
    {
        temp = head;
        head = head->next;
        free(temp);
    }
}

void free_move_count(struct move_count *head)
{
    struct move_count *previous;
    while(head != NULL)
    {
        previous = head;
        head = head->next;
        free(previous);
    }
}

void destruct()
{
    free_log_nodes(move_log);
    free_move_count(head);
    free_positions(white_positions);
    free_positions(black_positions);

    move_log = NULL, head = NULL;
}

void construct()
{
    queen();
    king();
    knight();
    pawn();
    rook();
    bishop();
}

int main()
{
    start_game();
    // struct queue *q = generate_valid_moves(BLACK);
    // printf("moves generated %d\n", queue_length(q));
    // free_queue(q);
    // printf("depth %d, moves generated %d\n", 1, moves_in_depth(WHITE, 1));
    // printf("depth %d, moves generated %d\n", 2, moves_in_depth(WHITE, 2));
    // printf("depth %d, moves generated %d\n", 3, moves_in_depth(WHITE, 3));
    // printf("depth %d, moves generated %d\n", 4, moves_in_depth(WHITE, 4));
    // printf("depth %d, moves generated %d\n", 5, moves_in_depth(WHITE, 5));
    int depth = 5;
    printf("depth %d, moves generated %d\n", depth, moves_in_depth(WHITE, depth));
    // printf("depth %d, moves generated %d\n", 7, moves_in_depth(WHITE, 7));

    destruct();
    return 0;
    display_name_board();
    //construct();
    //display_board();
    int from, to;
    while(1)
    {
        if(autosave == 1)
        {
            write_board();
        }
        if(white_move)
        {
            printf("whites move ");
            scanf("%d %d", &from, &to);
            if((from == 3 && to == 3) && undo())
            {
                white_move = 0;
                display_name_board();
                continue;
            }
            else if(from == -2 && to == -2)
            {
                write_board();
                printf("saved\n");
                continue;
            }
            else if(!is_valid_pos(from) || !is_valid_pos(to))
            {
                continue;
            }
            else if(color(Coin(from)) != WHITE)
            {
                continue;
            }
            // else if(!move(from, to))
            // {
            //     printf("wrong move\n");
            //     continue;
            // }
            else
            {
                if(is_game_over(BLACK, black_king_pos))
                {
                    if(autosave == 1)
                    {
                        write_board();
                    }
                    break;
                }
            }
            white_move = 0;
        }
        else
        {
            printf("black move ");
            scanf("%d %d", &from, &to);
            if((from == 3 && to == 3) && undo())
            {
                white_move = 1;
                display_name_board();
                continue;
            }
            else if(from == -2 && to == -2)
            {
                write_board();
                printf("saved\n");
                continue;
            }
            else if(!is_valid_pos(from) || !is_valid_pos(to))
            {
                continue;
            }
            else if(color(Coin(from)) != BLACK)
            {
                continue;
            }
            // else if(!move(from, to))
            // {
            //     printf("wrong move\n");
            //     continue;
            // }
            else
            {
                if(is_game_over(WHITE, white_king_pos))
                {
                    if(autosave == 1)
                    {
                        write_board();
                    }
                    break;
                }
            }
            white_move = 1;
        }
        display_name_board();
        //display_positions();
        //display_board();
    }
    destruct();
    return 0;
}

void display_position(struct pos_node **positions)
{
    struct pos_node *current;
    for(int i = 0; i < 6; i++)
    {
        current = positions[i];
        printf("%d --", i);
        while(current != NULL)
        {
            printf("<%d,%d> ", current->pos, Coin(current->pos));
            current = current->next;
        }
        printf("\n");
    }
}

void display_positions()
{
    printf("black positions\n");
    display_position(black_positions);
    printf("white positions\n");
    display_position(white_positions);
}

void print_empty_row()
{
    for(int i = 0; i <= 12; i++)
    {
        printf(" ");
    }
}

void print_white_space()
{
    for(int i = 0; i <= 12; i++)
    {
        printf(":");
    }
}

void print_line()
{
    for(int i = 0; i <= 7; i++)
    {
        printf("______________");
    }
    printf("\n");
}

void display_row(int board[][7], int row, int color)
{

    for(int j = 0; j < 7; j++)
    {
        if(color == WHITE)
        {
            if(board[row][j] == 0)
            {
                printf(" ");
            }
            else
            {
                printf("0");
            }  
        }
        else
        {
            if(board[row][j] == 0)
            {
                printf(" ");
            }
            else
            {
                printf("*");
            }  
        }
    }
}

void coin_shape(int coin, int row, int color)
{
    if(coin_type(coin) == PAWN)
    {
       display_row(pawn_shape, row, color);
    }
    else if(coin_type(coin) == ROOK)
    {
        display_row(rook_shape, row, color);  
    }
    else if(coin_type(coin) == BISHOP)
    {
        display_row(bishop_shape, row, color);  
    }
    else if(coin_type(coin) == KNIGHT)
    {
        display_row(knight_shape, row, color);  
    }
    else if(coin_type(coin) == KING)
    {
        display_row(king_shape, row, color);  
    }
    else if(coin_type(coin) == QUEEN)
    {
        display_row(queen_shape, row, color);  
    }
}

void display_board()
{
    display_captured(black_captured, bct);
    printf("\t  ");
    for(int i = 0; i < 8; i++)
    {
        printf("     %d        ", i);
    }
    printf("\n");
    for(int i = 0; i <= 7; i++)
    {
        printf("\t");
        print_line();
        for(int k = 0; k <= 6; k++)
        {
            if(k == 3)
            {
                printf("%d ", i);
            }

            printf("\t|");        
            for(int j = 0; j <= 7; j++)
            {
                if(board[i][j] != 0)
                {
                    printf("   ");
                    coin_shape(board[i][j], k, board[i][j] / 10);
                    printf("   |");
                }
                else if((j + i) % 2 == 0)
                {
                    print_white_space();
                    printf("|");

                }
                else
                {
                    print_empty_row();
                    printf("|");
                }
            }
            printf("\n");
        }
    }
    printf("\t");
    print_line();
    printf("\n");
    display_captured(white_captured, wct);
}

void num_to_char(int num)
{
    if(num == WHITE)
    {
        printf("w");
    }
    if(num == BLACK)
    {
        printf("b");
    }
    if(num == KING)
    {
        printf("king");
    }
    if(num == QUEEN)
    {
        printf("queen");
    }
    if(num == KNIGHT)
    {
        printf("knight");
    }
    if(num == PAWN)
    {
        printf("pawn");
    }
    if(num == ROOK)
    {
        printf("rook");
    }
    if(num == BISHOP)
    {
        printf("bishop");
    }
}

void display_captured(int *captured, int top)
{
    printf("\t");
    for(int i = 0; i <= top; i++)
    {
        num_to_char(color(captured[i]));
        num_to_char(coin_type(captured[i]));
        printf(" ");
    }
    printf("\n");
}

void chess_board()
{
    board[0][0]=27,board[0][7]=27,board[0][1]=25,board[0][6]=25,board[0][2]=28,board[0][5]=28,board[0][3]=24,board[0][4]=23;
    board[7][0]=17,board[7][7]=17,board[7][1]=15,board[7][6]=15,board[7][2]=18,board[7][5]=18,board[7][3]=14,board[7][4]=13;
    for(int i = 0; i <= 7; i++)
    {
        for(int j = 0; j <= 7; j++)
        {
            if(i == 1)
            {
                board[i][j] = 26;
            }
            else if(i == 6)
            {
                board[i][j] = 16;
            }
        }
    }

}

void pawn()
{
pawn_shape[0][0]=pawn_shape[0][1]=pawn_shape[0][2]=pawn_shape[0][3]=pawn_shape[0][4]=pawn_shape[0][5]=pawn_shape[0][6]=0;
pawn_shape[1][0]=pawn_shape[1][1]=0,pawn_shape[1][2]=1,pawn_shape[1][3]=0,pawn_shape[1][4]=1,pawn_shape[1][5]=pawn_shape[1][6]=0;
pawn_shape[2][0]=0,pawn_shape[2][1]=1,pawn_shape[2][2]=0,pawn_shape[2][3]=1,pawn_shape[2][4]=0,pawn_shape[2][5]=1,pawn_shape[2][6]=0;
pawn_shape[3][0]=0,pawn_shape[3][1]=0,pawn_shape[3][2]=1,pawn_shape[3][3]=0,pawn_shape[3][4]=1,pawn_shape[3][5]=0,pawn_shape[3][6]=0;
pawn_shape[4][0]=0,pawn_shape[4][1]=0,pawn_shape[4][2]=1,pawn_shape[4][3]=1,pawn_shape[4][4]=1,pawn_shape[4][5]=0,pawn_shape[4][6]=0;
pawn_shape[5][0]=0,pawn_shape[5][1]=1,pawn_shape[5][2]=1,pawn_shape[5][3]=1,pawn_shape[5][4]=1,pawn_shape[5][5]=1,pawn_shape[5][6]=0;
pawn_shape[6][0]=1,pawn_shape[6][1]=1,pawn_shape[6][2]=1,pawn_shape[6][3]=1,pawn_shape[6][4]=1,pawn_shape[6][5]=1,pawn_shape[6][6]=1;
}

void bishop()
{
bishop_shape[0][0]=0,bishop_shape[0][1]=0,bishop_shape[0][2]=0,bishop_shape[0][3]=1,bishop_shape[0][4]=bishop_shape[0][5]=bishop_shape[0][6]=0;
bishop_shape[1][0]=0,bishop_shape[1][1]=1,bishop_shape[1][2]=1,bishop_shape[1][3]=0,bishop_shape[1][4]=1,bishop_shape[1][5]=1,bishop_shape[1][6]=0;
bishop_shape[2][0]=0,bishop_shape[2][1]=0,bishop_shape[2][2]=1,bishop_shape[2][3]=1,bishop_shape[2][4]=1,bishop_shape[2][5]=0,bishop_shape[2][6]=0;
bishop_shape[3][0]=0,bishop_shape[3][1]=0,bishop_shape[3][2]=1,bishop_shape[3][3]=1,bishop_shape[3][4]=1,bishop_shape[3][5]=0,bishop_shape[3][6]=0;
bishop_shape[4][0]=0,bishop_shape[4][1]=0,bishop_shape[4][2]=1,bishop_shape[4][3]=1,bishop_shape[4][4]=1,bishop_shape[4][5]=0,bishop_shape[4][6]=0;
bishop_shape[5][0]=0,bishop_shape[5][1]=1,bishop_shape[5][2]=1,bishop_shape[5][3]=1,bishop_shape[5][4]=1,bishop_shape[5][5]=1,bishop_shape[5][6]=0;
bishop_shape[6][0]=1,bishop_shape[6][1]=1,bishop_shape[6][2]=1,bishop_shape[6][3]=1,bishop_shape[6][4]=1,bishop_shape[6][5]=1,bishop_shape[6][6]=1;
}

void knight()
{
knight_shape[0][0]=0,knight_shape[0][1]=0,knight_shape[0][2]=1,knight_shape[0][3]=1,knight_shape[0][4]=knight_shape[0][5]=1,knight_shape[0][6]=0;
knight_shape[1][0]=1,knight_shape[1][1]=1,knight_shape[1][2]=1,knight_shape[1][3]=1,knight_shape[1][4]=0,knight_shape[1][5]=1,knight_shape[1][6]=1;
knight_shape[2][0]=1,knight_shape[2][1]=1,knight_shape[2][2]=1,knight_shape[2][3]=1,knight_shape[2][4]=1,knight_shape[2][5]=1,knight_shape[2][6]=1;
knight_shape[3][0]=0,knight_shape[3][1]=0,knight_shape[3][2]=0,knight_shape[3][3]=0,knight_shape[3][4]=1,knight_shape[3][5]=1,knight_shape[3][6]=1;
knight_shape[4][0]=0,knight_shape[4][1]=0,knight_shape[4][2]=0,knight_shape[4][3]=1,knight_shape[4][4]=1,knight_shape[4][5]=1,knight_shape[4][6]=0;
knight_shape[5][0]=0,knight_shape[5][1]=0,knight_shape[5][2]=1,knight_shape[5][3]=1,knight_shape[5][4]=1,knight_shape[5][5]=1,knight_shape[5][6]=0;
knight_shape[6][0]=1,knight_shape[6][1]=1,knight_shape[6][2]=1,knight_shape[6][3]=1,knight_shape[6][4]=1,knight_shape[6][5]=1,knight_shape[6][6]=1; 
}

void queen()
{
queen_shape[0][0]=1,queen_shape[0][1]=0,queen_shape[0][2]=0,queen_shape[0][3]=1,queen_shape[0][4]=queen_shape[0][5]=0,queen_shape[0][6]=1;
queen_shape[1][0]=0,queen_shape[1][1]=1,queen_shape[1][2]=0,queen_shape[1][3]=1,queen_shape[1][4]=0,queen_shape[1][5]=1,queen_shape[1][6]=0;
queen_shape[2][0]=1,queen_shape[2][1]=1,queen_shape[2][2]=1,queen_shape[2][3]=1,queen_shape[2][4]=1,queen_shape[2][5]=1,queen_shape[2][6]=1;
queen_shape[3][0]=0,queen_shape[3][1]=0,queen_shape[3][2]=1,queen_shape[3][3]=1,queen_shape[3][4]=1,queen_shape[3][5]=0,queen_shape[3][6]=0;
queen_shape[4][0]=0,queen_shape[4][1]=0,queen_shape[4][2]=1,queen_shape[4][3]=1,queen_shape[4][4]=1,queen_shape[4][5]=0,queen_shape[4][6]=0;
queen_shape[5][0]=0,queen_shape[5][1]=1,queen_shape[5][2]=1,queen_shape[5][3]=1,queen_shape[5][4]=1,queen_shape[5][5]=1,queen_shape[5][6]=0;
queen_shape[6][0]=1,queen_shape[6][1]=1,queen_shape[6][2]=1,queen_shape[6][3]=1,queen_shape[6][4]=1,queen_shape[6][5]=1,queen_shape[6][6]=1;
}

void king()
{
king_shape[0][0]=king_shape[0][1]=0,king_shape[0][2]=0,king_shape[0][3]=1,king_shape[0][4]=king_shape[0][5]=king_shape[0][6]=0;
king_shape[1][0]=0,king_shape[1][1]=1,king_shape[1][2]=1,king_shape[1][3]=1,king_shape[1][4]=1,king_shape[1][5]=1,king_shape[1][6]=0;
king_shape[2][0]=0,king_shape[2][1]=0,king_shape[2][2]=0,king_shape[2][3]=1,king_shape[2][4]=0,king_shape[2][5]=0,king_shape[2][6]=0;
king_shape[3][0]=1,king_shape[3][1]=1,king_shape[3][2]=1,king_shape[3][3]=1,king_shape[3][4]=1,king_shape[3][5]=1,king_shape[3][6]=1;
king_shape[4][0]=0,king_shape[4][1]=0,king_shape[4][2]=1,king_shape[4][3]=1,king_shape[4][4]=1,king_shape[4][5]=0,king_shape[4][6]=0;
king_shape[5][0]=0,king_shape[5][1]=1,king_shape[5][2]=1,king_shape[5][3]=1,king_shape[5][4]=1,king_shape[5][5]=1,king_shape[5][6]=0;
king_shape[6][0]=1,king_shape[6][1]=1,king_shape[6][2]=1,king_shape[6][3]=1,king_shape[6][4]=1,king_shape[6][5]=1,king_shape[6][6]=1;
}

void rook()
{
    for(int i = 0; i < 7; i++)
    {
        for(int j = 0; j < 7; j++)
        {
            if(i == 0 || i == 6)
            {
                rook_shape[i][j] = 1;
            }
            else
            {
                if(j == 0 || j == 6)
                {
                    rook_shape[i][j] = 0;
                }
                else
                {
                    rook_shape[i][j] = 1;
                }
            }
        }
        printf("\n");
    }
}

void display_name_board()
{
    display_captured(black_captured, bct);
    printf("\t");
    for(int i = 0; i < 8; i++)
    {
        printf("%d\t", i);
    }
    printf("\n\n");
    for(int i = 0; i < 8; i++)
    {
        printf("%d\t", i);
        for(int j = 0; j < 8; j++)
        {
            num_to_char(color(board[i][j]));
            num_to_char(coin_type(board[i][j]));
            printf("\t");
        }
        printf("\n\n");
    }
    display_captured(white_captured, wct);
}