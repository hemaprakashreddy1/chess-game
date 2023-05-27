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
    int cur_pos;
    int moves;
    int captured;
};

//
struct moves
{
    char notation[7];
    struct moves *next;
};

struct moves *front, *rear;
int top = 0, promotion = -1, from_pos[9], to_pos, total_moves = 0;
//
struct move_count_stack
{
    int count;
    struct move_count_stack *next;
};

struct log_node *move_log;
struct pos_node *black_positions[6], *white_positions[6];
struct rook_info rooks[4];
struct move_count_stack *head;

int board[8][8];
int pawn_shape[7][7];
int rook_shape[7][7];
int bishop_shape[7][7];
int knight_shape[7][7];
int queen_shape[7][7];
int king_shape[7][7];

int black_captured[16], white_captured[16], check_path[8], captured_rooks[4], crt = -1, cpt = -1, wct = -1, bct = -1;

int black_king_pos = 04, white_king_pos = 74, autosave = 0, file_id = 0, white_move = 1;
char file_name[100];
const int WHITE = 1, BLACK = 2, PAWN = 6, ROOK = 7, BISHOP = 8, KNIGHT = 5, KING = 3, QUEEN = 4;
int knight_moves[8];
int white_material = 143, black_material = 143;
const int N = -10, S = 10, E = 1, W = -1, NW = -11, NE = -9, SW = 9, SE = 11;
const int NORMAL = 1, SHORT_CASTLE = 2, LONG_CASTLE = 3, EN_PASSANT = 4;
int black_king_moves = 0, white_king_moves = 0;

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
int news_path(int start, int dest);
int is_check_after_move(int start, int dest, int move_type);
int can_promote_pawn(int color, int pos);
void promote_pawn(int pos);
int can_castle(int clr, int start, int dest);
void construct();
void destruct();
int is_news_move(int start, int dest);

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

struct pos_node* same_type_positions(int coin, struct pos_node** positions)
{
    return positions[pos_hash(coin)];
}

void add_position(int coin,int cur_pos, int prev_pos)
{
    int hash = pos_hash(coin);
    struct pos_node *new, *temp, **positions;
    positions = get_positions(color(coin));

    if(positions[hash] == NULL)
    {
        new = (struct pos_node*)malloc(sizeof(struct pos_node));
        new->pos = cur_pos;
        new->next = NULL;
        positions[hash] = new;
    }
    else
    {
        temp = positions[hash];
        while(temp->pos != prev_pos && temp->next != NULL)
        {
            temp = temp->next;
        }
        if(temp->pos == prev_pos)
        {
            temp->pos = cur_pos;
        }
        else if(temp->next == NULL)
        {
            new = (struct pos_node*)malloc(sizeof(struct pos_node));
            new->pos = cur_pos;
            new->next = NULL;
            temp->next = new;
        }
    }
}

void delete_position(int coin, int pos)
{
    struct pos_node *temp, **positions;
    int hash = pos_hash(coin);
    positions = get_positions(color(coin));

    temp = positions[hash];
    if(temp != NULL && positions[hash]->pos == pos)
    {
        positions[hash] = positions[hash]->next;
        free(temp);
    }
    else
    {
        struct pos_node *prev;
        while(temp != NULL && temp->pos != pos)
        {
            prev = temp;
            temp = temp->next;
        }
        if(temp != NULL && temp->pos == pos)
        {
            prev->next = temp->next;
            free(temp);
        }
    }
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

void push_log(int from, int from_coin, int to, int to_coin, int move_type)
{
    struct log_node *new = (struct log_node*)malloc(sizeof(struct log_node));
    new->from = from;
    new->to = to;
    new->from_coin = from_coin;
    new->to_coin = to_coin;
    new->move_type = move_type;

    if(move_log == NULL)
    {
        move_log = new;
        move_log->next = NULL;
    }
    else
    {
        new->next = move_log;
        move_log = new;
    }
}

struct log_node* pop_log()
{
    if(move_log == NULL)
    {
        return NULL;
    }

    struct log_node *temp = move_log;
    move_log = move_log->next;
    return temp;
}

void push_captured(int color, int coin)
{
    if(color == WHITE)
    {
        white_captured[++wct] = coin;
    }
    else
    {
        black_captured[++bct] = coin;
    }
}

void pop_captured(int color)
{
    if(color == WHITE)
    {
        wct--;
    }
    else
    {
        bct--;
    }
}

void push_move_count(int reset)
{
    if(head == NULL)
    {
        struct move_count_stack *new = (struct move_count_stack*)malloc(sizeof(struct move_count_stack));
        new->count = !reset;
        new->next = NULL;
        head = new;
    }
    else if(reset)
    {
        struct move_count_stack *new = (struct move_count_stack*)malloc(sizeof(struct move_count_stack));
        new->count = 0;
        new->next = head;
        head = new;
    }
    else
    {
        head->count++;
    }
}

int pop_move_count()
{
    if(head == NULL)
    {
        return 0;
    }
    else if(head->count == 0)
    {
        struct move_count_stack *temp = head;
        head = head->next;
        free(temp);
        return 1; 
    }
    else
    {
        head->count--;
        return 1;
    }
}

int value(int pos, int coin_type)
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
        if((row(pos) + column(pos)) % 2 == 0)
        {
            return 6 + WHITE;
        }
        return 6 + BLACK;
    }
    return 0;
}

void add_material(int coin, int pos)
{
    if(color(coin) == BLACK)
    {
        black_material += value(pos, coin_type(coin));
    }
    else
    {
        white_material += value(pos, coin_type(coin));
    }
}

void sub_material(int coin, int pos)
{
    if(color(coin) == BLACK)
    {
        black_material -= value(pos, coin_type(coin));
    }
    else
    {
        white_material -= value(pos, coin_type(coin));
    }
}

void push_check_path(int pos)
{
    check_path[++cpt] = pos;
}

int can_move_news(int coin)
{
    return coin_type(coin) != KNIGHT && coin_type(coin) != BISHOP;
}

int is_en_passant(int start, int dest)
{
    if(move_log == NULL || coin_type(move_log->from_coin) != PAWN)
    {
        return 0;
    }
    
    int last_mv_steps = row(move_log->from) - row(move_log->to);
    if(color(Coin(start)) == WHITE && last_mv_steps == -2 && dest + S == move_log->to)
    {
        return EN_PASSANT;
    }
    else if(color(Coin(start)) == BLACK && last_mv_steps == 2 && dest + N == move_log->to)
    {
        return EN_PASSANT;
    }
    return 0;
}

int pawn_move(int start, int dest)
{
    int clr = color(Coin(start)), steps = row(start) - row(dest);
    if(clr == WHITE && Coin(dest) == 0 && column(start) == column(dest))
    {
        if(steps == 1)
        {
            return 1;
        }
        else if(steps == 2 && row(start) == 6 && news_path(start, dest))
        {
            return 1;
        }
    }
    else if(clr == WHITE && steps == 1 && column(start) != column(dest))
    {
        if(color(Coin(dest)) == BLACK)
        {
            return 1;
        }
        else if(row(start) == 3)
        {
            return is_en_passant(start, dest);
        }
    }
    else if(clr == BLACK && Coin(dest) == 0 && column(start) == column(dest))
    {
        if(steps == -1)
        {
            return 1;
        }
        else if(steps == -2 && row(start) == 1 && news_path(start, dest))
        {
            return 1;
        }
    }
    else if(clr == BLACK && steps == -1 && column(start) != column(dest))
    {
        if(color(Coin(dest)) == WHITE)
        {
            return 1;
        }
        else if(row(start) == 4)
        {
            return is_en_passant(start, dest);
        }
    }
    return 0;
}

int is_valid_pos(int pos)
{
    return row(pos) >= 0 && row(pos) < 8 && column(pos) < 8 && column(pos) >= 0;
}

int king_moves(int color)
{
    if(color == BLACK)
    {
        return black_king_moves;
    }
    return white_king_moves;
}

int steps_limit(int start, int dest)
{
    if(coin_type(Coin(start)) == KING)
    {
        int row_steps = row(start) - row(dest);
        int column_steps = column(start) - column(dest);
        if(row_steps == 1 || row_steps == -1 || column_steps == 1 || column_steps == -1)
        {
            return 1;
        }
        else if((column_steps == 2 || column_steps == -2) && king_moves(color(Coin(start))) == 0 && is_news_move(start, dest))
        {
            return can_castle(color(Coin(start)), start, dest);
        }
        return 0;
    }
    else if(coin_type(Coin(start)) == PAWN)
    {
        return pawn_move(start, dest);
    }
    return 1;
}

int is_empty_path(int pos, int steps, int direction)
{
    for(int i = 1; i < steps; i++)
    {
        pos = pos + direction;
        if(Coin(pos) != 0)
        {
            return 0;
        }
    }
    return 1;
}

int news_path(int start, int dest)
{
    if(start > dest)
    {
        swap(&start, &dest);
    }
    int steps;
    if(column(start) == column(dest))
    {
        steps = row(dest) - row(start);
        return is_empty_path(start, steps, S);
    }
    steps = column(dest) - column(start);
    return is_empty_path(start, steps, E);
}

int cross_path(int start, int dest)
{
    if(start < dest)
    {
        swap(&start, &dest);
    }
    int steps = row(start) - row(dest);

    if(column(start) < column(dest))
    {
        return is_empty_path(start, steps, NE);
    }
    return is_empty_path(start, steps, NW);
}

int is_cross_move(int start, int dest)
{
    if(start < dest)
    {
        swap(&start, &dest);
    }
    int steps = row(start) - row(dest);
    return start + NW * steps == dest || start + NE * steps == dest;
}

int can_move_cross(int coin)
{
   return coin_type(coin) != KNIGHT && coin_type(coin) != ROOK;
}

int is_knight(int coin)
{
    return coin_type(coin) == KNIGHT;
}

int* generate_knight_moves(int pos)
{
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

int is_knight_move(int start, int dest)
{
    int *moves = generate_knight_moves(start);
    for(int i = 0; i < 8; i++)
    {
        if(moves[i] == dest && is_valid_pos(moves[i]))
        {
            return 1;
        }
    }
    return 0;
}

int is_news_move(int start, int dest)
{
    return row(start) == row(dest) || column(start) == column(dest);
}

int validate_move(int start, int dest)
{
    if(!is_valid_pos(start) || !is_valid_pos(dest))
    {
        return 0;
    }
    else if(color(Coin(start)) == color(Coin(dest)))
    {
        return 0;
    }
    else if(is_knight(Coin(start)))
    {
        return is_knight_move(start, dest) && !is_check_after_move(start, dest, NORMAL);
    }
    else if(can_move_news(Coin(start)) && is_news_move(start, dest))
    {
        if(coin_type(Coin(start)) == KING)
        {
            int move_type = steps_limit(start, dest);
            if(move_type > NORMAL)
            {
                return move_type;
            }
            return move_type && !is_check_after_move(start, dest, NORMAL);
        }
        else if(coin_type(Coin(start)) == PAWN)
        {
            return pawn_move(start, dest) && !is_check_after_move(start, dest, NORMAL);
        }
        else
        {
            return news_path(start, dest) && !is_check_after_move(start, dest, NORMAL);
        }
    }
    else if(can_move_cross(Coin(start)) && is_cross_move(start, dest))
    {
        if(coin_type(Coin(start)) == KING)
        {
            return steps_limit(start, dest) && !is_check_after_move(start, dest, NORMAL);
        }
        else if(coin_type(Coin(start)) == PAWN)
        {
            int move_type = pawn_move(start, dest);
            if(move_type && !is_check_after_move(start, dest, move_type))
            {
                return move_type;
            }
            return 0;
        }
        else
        {
            return cross_path(start, dest) && !is_check_after_move(start, dest, NORMAL);
        }
    }
    return 0;
}

int can_move(int direction, int coin)
{
	if(direction == N || direction == E || direction == W || direction == S)
	{
		return can_move_news(coin);
	}
	return can_move_cross(coin);
}

int is_check_path(int square, int steps, int direction, int track_path)
{
    int king_color = color(Coin(square)), pos = square;
    cpt = -1;
    for(int i = 1; i <= steps; i++)
    {
        pos += direction;
        if(track_path)
        {
            push_check_path(pos);
        }
        if(Coin(pos) && color(Coin(pos)) != king_color)
        {
            return can_move(direction, Coin(pos)) && steps_limit(pos, square);
        }
        else if(Coin(pos))
        {
            return 0;
        }
    }
    return 0;
}

int is_check(int king_color, int square, int track_path)
{
    int x = row(square), y = column(square);
    //north
    int steps = row(square);
    if(is_check_path(square, steps, N, track_path))
    {
        return check_path[cpt];
    }
    //south
    steps = 7 - row(square);
    if(is_check_path(square, steps, S, track_path))
    {
        return check_path[cpt];
    }
    //East
    steps = 7 - column(square);
    if(is_check_path(square, steps, E, track_path))
    {
        return check_path[cpt];
    }
    //west
    steps = column(square);
    if(is_check_path(square, steps, W, track_path))
    {
        return check_path[cpt];
    }
    //north west
    int min, max;
    if(x < y)
    {
        min = x;
        max = y;
    }
    else
    {
        min = y;
        max = x;
    }
    if(is_check_path(square, min, NW, track_path))
    {
        return check_path[cpt];
    }
    //south east mv
    if(is_check_path(square, 7 - max, SE, track_path))
    {
        return check_path[cpt];
    }
    //north east
    if((x + y) < 7)
    {
        steps = x;
    }
    else
    {
        steps = 7 - y;
    }
    if(is_check_path(square, steps, NE, track_path))
    {
        return check_path[cpt];
    }
    //south west
    if((x + y) < 7)
    {
        steps = y;
    }
    else
    {
        steps = 7 - x;
    }
    if(is_check_path(square, steps, SW, track_path))
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
    int *moves = generate_knight_moves(square);
    for(int i = 0; i < 8; i++)
    {
        if(is_valid_pos(moves[i]) && Coin(moves[i]) == op_color * 10 + KNIGHT)
        {
            push_check_path(moves[i]);
            return check_path[cpt];
        }
    }
    return -1;
}

int is_check_after_move(int start, int dest, int move_type)
{
    int start_coin = Coin(start), dest_coin = Coin(dest), check, king_pos;
    if(coin_type(start_coin) == KING)
    {
        king_pos = dest;
    }
    else if(color(start_coin) == WHITE)
    {
        king_pos = white_king_pos;
    }
    else
    {
        king_pos = black_king_pos;
    }
    board[row(dest)][column(dest)] = board[row(start)][column(start)];
    board[row(start)][column(start)] = 0;
    int en_pass_pos, en_coin;
    if(move_type == EN_PASSANT)
    {
        if(color(start_coin) == BLACK)
        {
            en_pass_pos = dest + N;
            en_coin = Coin(en_pass_pos);
            board[row(dest + N)][column(dest + N)] = 0;
        }
        else
        {
            en_pass_pos = dest + S;
            en_coin = Coin(en_pass_pos);
            board[row(dest + S)][column(dest + S)] = 0;
        }
    }
    check = is_check(color(start_coin), king_pos, 0);
    board[row(start)][column(start)] = start_coin;
    board[row(dest)][column(dest)] = dest_coin;
    if(move_type == EN_PASSANT)
    {
        board[row(en_pass_pos)][column(en_pass_pos)] = en_coin;
    }
    return check != -1;
}

void en_passant(int start, int dest)
{
    if(color(Coin(start)) == BLACK)
    {
        sub_material(Coin(dest + N), -1);
        push_captured(color(Coin(dest + N)), Coin(dest + N));
        delete_position(Coin(dest + N), dest + N);
        board[row(dest + N)][column(dest + N)] = 0;
    }
    else
    {
        sub_material(Coin(dest + S), -1);
        push_captured(color(Coin(dest + S)), Coin(dest + S));
        delete_position(Coin(dest + S), dest + S);
        board[row(dest + S)][column(dest + S)] = 0;
    }
}

void un_en_passant(struct log_node *temp)
{
    if(color(temp->from_coin) == WHITE)
    {
        pop_captured(BLACK);
        add_material(BLACK * 10 + PAWN, -1);
        add_position(BLACK * 10 + PAWN, temp->to + S, -1);
        board[row(temp->to + S)][column(temp->to + S)] = BLACK * 10 + PAWN;
    }
    else
    {
        pop_captured(WHITE);
        add_material(WHITE * 10 + PAWN, -1);
        add_position(WHITE * 10 + PAWN, temp->to + N, -1);
        board[row(temp->to + N)][column(temp->to + N)] = WHITE * 10 + PAWN;
    }
}

int rook_hash(int x)
{
    return x % 4;
}

void init_rook_info()
{
    rooks[rook_hash(0)].cur_pos = 0;
    rooks[rook_hash(7)].cur_pos = 7;
    rooks[rook_hash(70)].cur_pos = 70;
    rooks[rook_hash(77)].cur_pos = 77;
    rooks[0].captured = rooks[1].captured = rooks[2].captured = rooks[3].captured = 0;
    rooks[0].moves = rooks[1].moves = rooks[2].moves = rooks[3].moves = 0;
}

void add_rook_info(int cur_pos, int prev_pos, int captured, int undone)
{
    if(captured == 2)
    {
        rooks[captured_rooks[crt--]].captured = 0;
    }
    else
    {
        for(int i = 0; i < 4; i++)
        {
            if(rooks[i].cur_pos == prev_pos && rooks[i].captured == 0)
            {
                rooks[i].cur_pos = cur_pos;
                rooks[i].captured = captured;
                if(captured == 0)
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
            }
        }
    }
}

int can_castle(int clr, int start, int dest)
{
    int steps = column(start) - column(dest);
    if(clr == BLACK && black_king_moves == 0)
    {
        if(steps == -2 && rooks[rook_hash(7)].captured == 0 && rooks[rook_hash(7)].moves == 0 && news_path(start, dest))
        {
            if(is_check(clr, black_king_pos, 0) == -1 && !is_check_after_move(start, start + E, NORMAL) && !is_check_after_move(start, start + 2 * E, NORMAL))
            {
                return SHORT_CASTLE;
            }
        }
        else if(steps == 2 && rooks[rook_hash(0)].captured == 0 && rooks[rook_hash(0)].moves == 0 && news_path(start, dest + 2 * W))
        {
            if(is_check(clr, black_king_pos, 0) == -1 && !is_check_after_move(start, start + W, NORMAL) && !is_check_after_move(start, start + 2 * W, NORMAL))
            {
                return LONG_CASTLE;
            }
        }
    }
    else if(clr == WHITE && white_king_moves == 0)
    {
        if(steps == -2 && rooks[rook_hash(77)].captured == 0 && rooks[rook_hash(77)].moves == 0 && news_path(start, dest))
        {
            if(is_check(clr, white_king_pos, 0) == -1 && !is_check_after_move(start, start + E, NORMAL) && !is_check_after_move(start, start + 2 * E, NORMAL))
            {
                return SHORT_CASTLE;
            }
        }
        else if(steps == 2 && rooks[rook_hash(70)].captured == 0 && rooks[rook_hash(70)].moves == 0 && news_path(start, dest + 2 * W))
        {
            if(is_check(clr, white_king_pos, 0) == -1 && !is_check_after_move(start, start + W, NORMAL) && !is_check_after_move(start, start + 2 * W, NORMAL))
            {
                return LONG_CASTLE;
            }
        }
    }
    return 0;
}

void castle(int clr, int castle_type)
{
    if(clr == BLACK)
    {
        if(castle_type == SHORT_CASTLE)
        {
            add_position(Coin(7), 5, 7);
            add_rook_info(5, 7, 0, 0);
            board[0][7] = 0;
            board[0][5] = BLACK * 10 + ROOK;
        }
        else
        {
            add_position(Coin(0), 3, 0);
            add_rook_info(3, 0, 0, 0);
            board[0][0] = 0;
            board[0][3] = BLACK * 10 + ROOK;
        }
    }
    else
    {
        if(castle_type == SHORT_CASTLE)
        {
            add_position(Coin(77), 75, 77);
            add_rook_info(75, 77, 0, 0);
            board[7][7] = 0;
            board[7][5] = WHITE * 10 + ROOK;
        }
        else
        {
            add_position(Coin(70), 73, 70);
            add_rook_info(73, 70, 0, 0);
            board[7][0] = 0;
            board[7][3] = WHITE * 10 + ROOK;
        }
    }
}

void un_castle(struct log_node *temp)
{
    if(temp->move_type == SHORT_CASTLE)
    {
        if(color(temp->from_coin) == BLACK)
        {
            add_position(BLACK * 10 + ROOK, 7, 5);
            add_rook_info(7, 5, 0, 1);
            board[0][5] = 0;
            board[0][7] = BLACK * 10 + ROOK;
        }
        else
        {
            add_position(WHITE * 10 + ROOK, 77, 75);
            add_rook_info(77, 75, 0, 1);
            board[7][5] = 0;
            board[7][7] = WHITE * 10 + ROOK;
        }
    }
    else if(temp->move_type == LONG_CASTLE)
    {
        if(color(temp->from_coin) == BLACK)
        {
            add_position(BLACK * 10 + ROOK, 0, 3);
            add_rook_info(0, 3, 0, 1);
            board[0][3] = 0;
            board[0][0] = BLACK * 10 + ROOK;
        }
        else
        {
            add_position(WHITE * 10 + ROOK, 70, 73);
            add_rook_info(70, 73, 0, 1);
            board[7][3] = 0;
            board[7][0] = WHITE * 10 + ROOK;
        }
    }
}

int move(int start, int dest)
{
    int move_type = validate_move(start, dest);
    if(!move_type)
    {
        return 0;
    }

    push_log(start, Coin(start), dest, Coin(dest), move_type);
    add_position(Coin(start), dest, start);
    if(Coin(dest) != 0)
    {
        sub_material(Coin(dest), dest);
        push_captured(color(Coin(dest)), Coin(dest));
        delete_position(Coin(dest), dest);
        if(coin_type(Coin(dest)) == ROOK)
        {
            add_rook_info(dest, dest, 1, 0);
        }
    }
    else if(move_type == EN_PASSANT)
    {
        en_passant(start, dest);
    }
    else if(move_type == SHORT_CASTLE || move_type == LONG_CASTLE)
    {
        castle(color(Coin(start)), move_type);
    }

    if(Coin(dest) != 0 || coin_type(Coin(start)) == PAWN)
    {
        push_move_count(1);
    }
    else
    {
        push_move_count(0);
    }

    board[row(dest)][column(dest)] = Coin(start);
    board[row(start)][column(start)] = 0;
    if(coin_type(Coin(dest)) == PAWN && can_promote_pawn(color(Coin(dest)), dest))
    {
        promote_pawn(dest);
    }
    else if(Coin(dest) == WHITE * 10 + KING)
    {
        white_king_pos = dest;
        white_king_moves++;
    }
    else if(Coin(dest) == BLACK * 10 + KING)
    {
        black_king_pos = dest;
        black_king_moves++;
    }
    else if(coin_type(Coin(dest)) == ROOK)
    {
        add_rook_info(dest, start, 0, 0);
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

    if(coin_type(temp->from_coin) == PAWN && (row(temp->to) == 0 || row(temp->to) == 7))
    {
        add_position(temp->from_coin, temp->from, -1);
        delete_position(Coin(temp->to), temp->to);
        add_material(temp->from_coin, temp->from);
        sub_material(Coin(temp->to), temp->to);
    }
    else
    {
        add_position(temp->from_coin, temp->from, temp->to);
        if(coin_type(temp->from_coin) == ROOK)
        {
            add_rook_info(temp->from, temp->to, 0, 1);
        }
    }

    if(temp->to_coin != 0)
    {
        pop_captured(color(temp->to_coin));
        add_position(temp->to_coin, temp->to, -1);
        add_material(temp->to_coin, temp->to);
        if(coin_type(temp->to_coin) == ROOK)
        {
            add_rook_info(temp->to, temp->to, 2, 1);
        }
    }
    else if(temp->move_type == EN_PASSANT)
    {
        un_en_passant(temp);
    }
    else if(temp->move_type == SHORT_CASTLE || temp->move_type == LONG_CASTLE)
    {
        un_castle(temp);
    }

    if(temp->from_coin == WHITE * 10 + KING)
    {
        white_king_pos = temp->from;
        white_king_moves--;
    }  
    else if(temp->from_coin == BLACK * 10 + KING)
    {
        black_king_pos = temp->from;
        black_king_moves--;
    }
    pop_move_count();
    board[row(temp->from)][column(temp->from)] = temp->from_coin;
    board[row(temp->to)][column(temp->to)] = temp->to_coin;   
    free(temp);
    display_name_board();
    //display_board();
    return 1;
}

int can_promote_pawn(int color, int pos)
{
    return (row(pos) == 0 && color == WHITE) || (row(pos) == 7 && color == BLACK);
}

void promote_pawn(int pos)
{
    int coin;
    while(1)
    {
        //printf("4.queen, 5.knight, 7.rook, 8.bishop ");
        //scanf("%d", &coin);
        coin = promotion;
        if(coin == QUEEN || coin == KNIGHT || coin == ROOK || coin == BISHOP)
        {
            delete_position(Coin(pos), pos);
            sub_material(Coin(pos), pos);
            board[row(pos)][column(pos)] = color(Coin(pos)) * 10 + coin;
            add_position(Coin(pos), pos, -1);
            add_material(Coin(pos), pos);
            break;
        }
    }
}

int cover_check(int pos, int color)
{
    struct pos_node **positions, *temp;
    positions = get_positions(color);

    for(int i = 0; i < 6; i++)
    {
        temp = positions[i];
        while(temp != NULL)
        {
            if(coin_type(Coin(temp->pos)) != KING && validate_move(temp->pos, pos))
            {
                return Coin(temp->pos);
            }
            temp = temp->next;
        }
    }
    return 0;
}

int en_passant_cover(int position, int color)
{
    if(color == BLACK && is_valid_pos(position + S) && !Coin(position + S))
    {
        if(is_valid_pos(position + W) && Coin(position + W) == BLACK * 10 + PAWN)
        {
            return pawn_move(position + W, position + S) == EN_PASSANT && !is_check_after_move(position + W, position + S, EN_PASSANT);
        }
        else if(is_valid_pos(position + E) && Coin(position + E) == BLACK * 10 + PAWN)
        {
            return pawn_move(position + E, position + S) == EN_PASSANT && !is_check_after_move(position + E, position + S, EN_PASSANT);
        }
    }
    else if(color == WHITE && is_valid_pos(position + N) && !Coin(position + N))
    {
        if(is_valid_pos(position + W) && Coin(position + W) == WHITE * 10 + PAWN)
        {
            return pawn_move(position + W, position + N) == EN_PASSANT && !is_check_after_move(position + W, position + N, EN_PASSANT);
        }
        else if(is_valid_pos(position + E) && Coin(position + E) == WHITE * 10 + PAWN)
        {
            return pawn_move(position + E, position + N) == EN_PASSANT && !is_check_after_move(position + E, position + N, EN_PASSANT);
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
    for(int i = 0; i < 8; i++)
    {
        if(is_valid_pos(moves[i]) && color(Coin(moves[i])) != clr)
        {
            return !is_check_after_move(pos, moves[i], NORMAL);
        }
    }
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

int is_game_over(int color, int king_position)
{
    int king_can_move = one_king_move(king_position);
    if(black_material + white_material < 10 || (black_material + white_material <= 16 && black_material == white_material))
    {
        // display_name_board();
        // printf("Drawn by insufficient material\n");
        return 1;
    }
    else if(!king_can_move && is_check(color, king_position, 1) != -1)
    {
        if(!is_check_covered(color))
        {
            //display_name_board();
            // if(color == BLACK)
            // {
            //     printf("White won by checkmate\n");
            // }
            // else
            // {
            //     printf("Black won by checkmate\n");
            // }
            return 1;
        }
    }
    else if(!king_can_move && !have_one_move(color))
    {
        // display_name_board();
        // printf("Drawn by Stale mate\n");
        return 1;
    }
    else if(head && head->count == 100)
    {
        //printf("Game drawn by the 50 move rule\n");
        return 1;
    }
    return 0;
}

//storing in a file
void write_move_log(FILE *file, struct log_node *temp)
{
    if(temp != NULL)
    {
        write_move_log(file, temp->next);
        fprintf(file, "%d %d %d %d %d ", temp->from, temp->from_coin, temp->to, temp->to_coin, temp->move_type);
    }
}

void write_move_count(FILE *file, struct move_count_stack *temp)
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
    struct move_count_stack *new;
    while(fscanf(file, "%d", &count) != EOF)
    {
        if(count == -1)
        {
            break;
        }
        if(head == NULL)
        {
            new = (struct move_count_stack*)malloc(sizeof(struct move_count_stack));
            new->count = count;
            head = new;
            head->next = NULL;
        }
        else
        {
            new = (struct move_count_stack*)malloc(sizeof(struct move_count_stack));
            new->count = count;
            new->next = head;
            head = new;
        }
    }
}

void read_board()
{
    FILE *file;
    int pos, coin;
    file = fopen(file_name, "r");
    if(file == NULL)
    {
        printf("error opening the file\n");
        return;
    }
    while(fscanf(file, "%d %d", &pos, &coin) != EOF)
    {
        if(pos == -1)
        {
            break;
        }
        board[row(pos)][column(pos)] = coin;
    }
    //reading captured coins
    while(fscanf(file, "%d ", &coin) != EOF)
    {
        if(coin == -1)
        {
            break;
        }
        push_captured(color(coin), coin);
    }
    //reading_move_log
    int from, from_coin, to, to_coin, mv_type;
    while(fscanf(file, "%d %d %d %d %d", &from, &from_coin, &to, &to_coin, &mv_type) != EOF)
    {
        if(from == -1)
        {
            break;
        }
        push_log(from, from_coin, to, to_coin, mv_type);
    }
    fscanf(file, "%d %d", &white_king_pos, &black_king_pos);
    fscanf(file, "%d %d", &white_material, &black_material);
    for(int i = 0; i < 4; i++)
    {
        fscanf(file, "%d %d %d", &rooks[i].cur_pos, &rooks[i].moves, &rooks[i].captured);
    }
    int captured_index;
    while(fscanf(file, "%d ", &captured_index) != EOF)
    {
        if(captured_index == -1)
        {
            break;
        }
        captured_rooks[++crt] = captured_index;
    }
    fscanf(file, "%d %d\n", &white_king_moves, &black_king_moves);
    read_move_count(file);
    if(fclose(file) != 0)
    {
        printf("error closing the file\n");
    }
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
    //storing captured coins
    //white_positions
    for(int i = 0; i <= wct; i++)
    {
        fprintf(file, "%d ", white_captured[i]);
    }
    //black_positions
    for(int i = 0; i <= bct; i++)
    {
        fprintf(file, "%d ", black_captured[i]);
    }
    fprintf(file, "%d\n", -1);

    write_move_log(file, move_log);
    fprintf(file, "%d %d %d %d %d\n", -1, -1, -1, -1, -1);
    fprintf(file, "%d %d\n", white_king_pos, black_king_pos);
    fprintf(file, "%d %d\n", white_material, black_material);
    for(int i = 0; i < 4; i++)
    {
        fprintf(file, "%d %d %d ", rooks[i].cur_pos, rooks[i].moves, rooks[i].captured);
    }
    fprintf(file, "\n");
    for(int i = 0; i <= crt; i++)
    {
        fprintf(file, "%d ", captured_rooks[i]);
    }
    fprintf(file, "%d\n", -1);
    fprintf(file, "%d %d\n", white_king_moves, black_king_moves);
    write_move_count(file, head);
    fprintf(file, "%d\n", -1);
    if(fclose(file) != 0)
    {
        printf("error closing the file\n");
    }
}

void start_game()
{
    int choice = 1;
    while(1)
    {
        //printf("\n1.New game 2.continue 3.settings ");
        //scanf("%d",&choice);
        if(choice == 1)
        {
            //printf("enter any id(number) for this game ");
            //scanf("%d", &file_id);
            file_id = 143341;
            sprintf(file_name, "%d.txt", file_id);
            chess_board();
            init_rook_info();
            init_hash_table();
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
    struct pos_node *prev, *temp;
    for(int i = 0; i <= 7; i++)
    {
        temp = positions[i];
        while(temp != NULL)
        {
            prev = temp;
            temp = temp->next;
            free(prev);
        }
        positions[i] = NULL;
    }
}

void destruct()
{
    struct log_node *prev, *temp = move_log;
    struct moves* f = front, *prev_move;
    while(f != NULL)
    {
        prev_move = f;
        f = f->next;
        free(prev_move);
    }
    while(temp != NULL)
    {
        prev = temp;
        temp = temp->next;
        free(prev);
    }
    struct move_count_stack *previous, *current = head;
    while(current != NULL)
    {
        previous = current;
        current = current->next;
        free(previous);
    }
    free_positions(white_positions);
    free_positions(black_positions);
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
//
void append(struct moves *new)
{
    if(front == NULL)
    {
        front = rear = new;
    }
    else
    {
        rear->next = new;
        rear = rear->next;
    }
    rear->next = NULL;
}

int is_end(char *arr)
{
    for(int i = 0; arr[i] != '\0'; i++)
    {
        if(arr[i] == '#')
        {
            return 1;
        }
    }
    return 0;
}

int read_moves(FILE *file)
{
    //FILE *file = fopen("moves.txt", "r");
    if(!file)
    {
        printf("error opening file\n");
        exit(1);
    }

    int color = 1;
    struct moves *new = (struct moves*)malloc(sizeof(struct moves));
    while(fscanf(file, "%s", new->notation) != EOF)
    {
        if(color == 2)
        {
            color = 1;
        }
        else
        {
            total_moves++;
            fscanf(file, "%s", new->notation);
            color = 2;
        }
        append(new);
        if(is_end(new->notation))
        {
            return 1;
        }
        new = (struct moves*)malloc(sizeof(struct moves));
    }
    free(new);
    return 0;
}

void print_moves()
{
    while(front != NULL)
    {
        printf("%s |", front->notation);
        front = front->next;
    }
    printf("\n");
}

int len(char *arr)
{
    int ln = 0;
    while(arr[ln] != '\0')
    {
        ln++;
    }
    return ln;
}

int char_to_coin(char c)
{
    if(c == 'K')
    {
        return KING;
    }
    else if(c == 'Q')
    {
        return QUEEN;
    }
    else if(c == 'N')
    {
        return KNIGHT;
    }
    else if(c == 'R')
    {
        return ROOK;
    }
    else if(c == 'B')
    {
        return BISHOP;
    }
}

int destination(char *arr)
{
    int dest = 0;
    while(1)
    {
        if(arr[top] >= '0' && arr[top] <= '9')
        {
            dest = 8 - (arr[top--] - '0');
            dest = dest * 10 + (arr[top--] - 'a');
            return dest;
        }
        else if(arr[top] == 'O')
        {
            while(top >= 0)
            {
                if(arr[top] == 'O')
                {
                    dest++;
                }
                top -= 2;
            }
            return -dest;
        }
        else if(arr[top] >= 'A' && arr[top] <= 'Z')
        {
            promotion = char_to_coin(arr[top]);
            top -= 2;
        }
        else
        {
            top--;
        }
    }
}

void convert(char *notation, int color)
{
    top = len(notation) - 1;
    int from_coin = -1, f_row = -1, f_col = -1;
    to_pos = destination(notation);

    if(to_pos == -SHORT_CASTLE)
    {
        if(color == BLACK)
        {
            from_pos[0] = 4;
            to_pos = 6;
        }
        else
        {
            from_pos[0] = 74;
            to_pos = 76;
        }
        from_pos[1] = -1;
    }
    else if(to_pos == -LONG_CASTLE)
    {
        if(color == BLACK)
        {
            from_pos[0] = 4;
            to_pos = 2;
        }
        else
        {
            from_pos[0] = 74;
            to_pos = 72;
        }
        from_pos[1] = -1;
    }
    else
    {
        if(notation[0] >= 'A' && notation[0] <= 'Z')
        {
            from_coin = char_to_coin(notation[0]);
        }
        else
        {
            from_coin = PAWN;
        }

        while(top >= 0)
        {
            if(notation[top] >= 'a' && notation[top] <= 'h')
            {
                f_col = notation[top] - 'a';
            }
            if(notation[top] >= '0' && notation[top] <= '9')
            {
                f_row = 8 - (notation[top] - '0');
            }
            top--;
        }

        struct pos_node *temp = same_type_positions(color * 10 + from_coin, get_positions(color));

        if(temp == NULL)
        {
            printf("%d coins are missing\n", from_coin);
            print_moves();
            exit(0);
        }

        if(f_row != -1 && f_col != -1)
        {
            from_pos[0] = f_row * 10 + f_col;
            from_pos[1] = -1;
        }
        else if(f_row != -1)
        {
            int i = 0;
            while (temp != NULL)
            {
                if(row(temp->pos) == f_row)
                {
                    from_pos[i] = temp->pos;
                    i++;
                }
                temp = temp->next;
            }
            from_pos[i] = -1;
        }
        else if(f_col != -1)
        {
            int i = 0;
            while (temp != NULL)
            {
                if(column(temp->pos) == f_col)
                {
                    from_pos[i] = temp->pos;
                    i++;
                }
                temp = temp->next;
            }
            from_pos[i] = -1;
        }
        else if(from_coin == PAWN)
        {
            if(color == BLACK)
            {
                from_pos[0] = to_pos + N;
                from_pos[1] = to_pos + 2 * N;
            }
            else
            {
                from_pos[0] = to_pos + S;
                from_pos[1] = to_pos + 2 * S;
            }
            from_pos[2] = -1;
        }
        else
        {
            int i = 0;
            while (temp != NULL)
            {
                from_pos[i] = temp->pos;
                temp = temp->next;
                i++;
            }
            from_pos[i] = -1;
        }
    }
}

//

void init_new_game()
{
    move_log = NULL, front = rear = NULL;
    crt = -1, cpt = -1, wct = -1, bct = -1;
    white_king_pos = 74, black_king_pos = 4, white_king_moves = 0, black_king_moves = 0;
    white_material = 143, black_material = 143, white_move = 1;
    total_moves = 0;
    head = NULL;

    for(int i = 0; i <= 7; i++)
    {
        for(int j = 0; j <= 7; j++)
        {
            board[i][j] = 0;
        }
    }
    chess_board();
    init_hash_table();
    init_rook_info();
}

int main()
{
    FILE *file = fopen("fifty_moves.txt", "r");
    if(!file)
    {
        printf("error opening the file\n");
        return 0;
    }
    start_game();
    //display_name_board();
    //construct();
    //display_board();
    read_moves(file);
    struct moves *temp = front;
    int from, to, fpt = 0;
    int move_no = 0, wrong_moves = 0, invalid_pos = 0, wrong_color = 0, undone = 0, saved = 0;
    int white_won = 0, black_won = 0, games = 0, valid_games = 0;
    while(temp != NULL)
    {
        if(white_move)
        {   fpt = 0;
            convert(temp->notation, WHITE);
            while(from_pos[fpt] != -1)
            {
                from = from_pos[fpt++];
                to = to_pos;
                if((from == 3 && to == 3) && undo())
                {
                    white_move=0;
                    undone++;
                    printf("undone\n");
                    //continue;
                }
                else if(from == -2 && to == -2)
                {
                    write_board();
                    saved++;
                    printf("saved\n");
                    //continue;
                }
                else if(!is_valid_pos(from) || !is_valid_pos(to))
                {
                    //printf("not a valid from - %d to - %d, notation - %s\n", from, to, temp->notation);
                    invalid_pos++;
                    //continue;
                }
                else if(color(Coin(from)) != WHITE)
                {
                    //printf("wrong color\n");
                    wrong_color++;
                    //continue;
                }
                else if(!move(from,to))
                {
                    //printf("wrong move %d to %d not %s\n", from, to, temp->notation);
                    wrong_moves++;
                    //continue;
                }
                else
                {
                    break;
                }
            }
            move_no++;
            //printf("move = %d\n",move_no);
            white_move=0;
            temp = temp->next;
            if(is_game_over(BLACK, black_king_pos))
            {
                write_board();
                white_won++;
                games++;
                destruct();
                //printf("white - %d\nblack - %d\n", white_won, black_won);
                if(total_moves == move_no)
                {
                    valid_games++;
                }
                else
                {
                    printf("invalid game - %d\n",games);
                }
                move_no = 0;
                init_new_game();
                if(read_moves(file))
                {
                    temp = front;
                    continue;
                }
                break;
            }
        }
        else
        {
            fpt = 0;
            convert(temp->notation, BLACK);
            while(from_pos[fpt] != -1)
            {
                from = from_pos[fpt++];
                to = to_pos;
                if((from == 3 && to == 3) && undo())
                {
                    printf("undone\n");
                    undone++;
                    white_move = 1;
                    continue;
                }
                else if(from == -2 && to == -2)
                {
                    write_board();
                    printf("saved\n");
                    saved++;
                    continue;
                }
                else if(!is_valid_pos(from) || !is_valid_pos(to))
                {
                    //printf("not a valid from - %d to - %d, notation - %s\n", from, to, temp->notation);
                    invalid_pos++;
                    continue;
                }
                else if(color(Coin(from)) != BLACK)
                {
                    //printf("wrong color\n");
                    wrong_color++;
                    continue;
                }
                else if(!move(from, to))
                {
                    //printf("wrong move\n");
                    wrong_moves++;
                    continue;
                }
                else
                {
                    break;
                }
            }
            white_move = 1;
            temp = temp->next;
            if(is_game_over(WHITE, white_king_pos))
            {
                write_board();
                black_won++;
                games++;
                destruct();
                //printf("white - %d\nblack - %d\n", white_won, black_won);
                if(total_moves == move_no)
                {
                    valid_games++;
                }
                else
                {
                    printf("invalid game - %d\n",games);
                }
                move_no = 0;
                init_new_game();
                if(read_moves(file))
                {
                    temp = front;
                    continue;
                }
                break;
            }
        }
        // printf("move %d to %d\n", from, to);
        // display_name_board();
        //printf("move from %d, to %d\n", from, to);
        /*printf("black positions\n");
        display_positions(black_positions);
        printf("white positions\n");
        display_positions(white_positions);*/
        //display_board();
    }
    printf("white - %d\nblack - %d\ngames - %d\n", white_won, black_won, games);
    if(fclose(file))
    {
        printf("error closing file\n");
    }
    //print_moves();
    //printf("moves - %d\n", move_no);
    //printf("invalid_pos - %d, wrong_color - %d, saved - %d, wrong_moves - %d, undone - %d\n", invalid_pos, wrong_color, saved, wrong_moves, undone);
    destruct();
    return 0;
}

void display_positions(struct pos_node **positions)
{
    struct pos_node*temp;
    for(int i = 0; i < 6; i++)
    {
        temp = positions[i];
        printf("%d --", i);
        while(temp != NULL)
        {
            printf("<%d,%d> ", temp->pos, Coin(temp->pos));
            temp = temp->next;
        }
        printf("\n");
    }
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