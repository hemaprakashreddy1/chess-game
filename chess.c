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

struct log_node *move_log;
struct pos_node *black_positions[6],*white_positions[6];
int board[8][8];
int pawn_shape[7][7];
int rook_shape[7][7];
int bishop_shape[7][7];
int knight_shape[7][7];
int queen_shape[7][7];
int king_shape[7][7];
int black_captured[16],white_captured[16],check_path[8],cpt=-1,wct=-1,bct=-1;
int black_king_pos=04,white_king_pos=74,autosave=0,file_id=0,white_move=1;
const int WHITE=1,BLACK=2,PAWN=6,ROOK=7,BISHOP=8,KNIGHT=5,KING=3,QUEEN=4;
int knight_moves[8];
int white_material=143,black_material=143;
const int N=-10,S=10,E=1,W=-1,NW=-11,NE=-9,SW=9,SE=11;
const int NORMAL=1,EN_PASSANT=4;

void rook();
void queen();
void bishop();
void print_empty_row();
void display_name_board();
void display_captured();
void pawn();
void print_white_space();
void knight();
void king();
void print_line();
void display_row();
void coin_shape();
void display_board();
void num_to_char();
void chess_board();
void display_positions();
int news_path();
int is_check_after_move();
void read_or_write_board(char mode);
int can_promote_pawn();
void promote_pawn();

int row(int pos)
{
    return pos/10;
}

int column(int pos)
{
    return pos%10;
}

int Coin(int pos)
{
    return board[row(pos)][column(pos)];
}

int color(int coin)
{
    return coin/10;
}

int coin_type(int coin)
{
    return coin%10;
}

int pos_hash(int coin)
{
    return coin%6;
}

void swap(int *x,int *y)
{
    int t=*x;
    *x=*y;
    *y=t;
}

void add_position(int coin,int cur_pos,int prev_pos)
{
    int hash=pos_hash(coin);
    struct pos_node *new,*temp,**positions;
    if(color(coin)==BLACK)
    {
        positions=black_positions;
    }
    else
    {
        positions=white_positions;
    }
    if(positions[hash]==NULL)
    {
        new=(struct pos_node*)malloc(sizeof(struct pos_node));
        new->pos=cur_pos;
        new->next=NULL;
        positions[hash]=new;
    }
    else
    {
        temp=positions[hash];
        while(temp->pos!=prev_pos && temp->next!=NULL)
        {
            temp=temp->next;
        }
        if(temp->pos==prev_pos)
        {
            temp->pos=cur_pos;
        }
        else if(temp->next==NULL)
        {
            new=(struct pos_node*)malloc(sizeof(struct pos_node));
            new->pos=cur_pos;
            new->next=NULL;
            temp->next=new;
        }
    }
}

void delete_position(int coin,int pos)
{
    struct pos_node *temp,**positions;
    int hash=pos_hash(coin);
    if(color(coin)==BLACK)
    {
        positions=black_positions;
    }
    else
    {
        positions=white_positions;
    }   
    temp=positions[hash];
    if(temp!=NULL && positions[hash]->pos==pos)
    {
        positions[hash]=positions[hash]->next;
        free(temp);
    }
    else
    {
        struct pos_node *prev;
        while(temp!=NULL)
        {
            if(temp->pos==pos)
            {
                prev->next=temp->next;
                free(temp);
                break;
            }
            prev=temp;
            temp=temp->next;
        }
    }
}

void init_hash_table()
{
    for(int i=0;i<=7;i++)
    {
        for(int j=0;j<=7;j++)
        {
            if(board[i][j]!=0)
            {
                add_position(board[i][j],i*10+j,-1);
            }
        }
    }
}

void push_log(int from,int from_coin,int to,int to_coin,int move_type)
{
    struct log_node *new=(struct log_node*)malloc(sizeof(struct log_node));
    new->from=from;
    new->to=to;
    new->from_coin=from_coin;
    new->to_coin=to_coin;
    new->move_type=move_type;
    if(move_log==NULL)
    {
        move_log=new;
        move_log->next=NULL;
    }
    else
    {
        new->next=move_log;
        move_log=new;
    }
}

struct log_node* pop_log()
{
    if(move_log==NULL)
    {
        return NULL;
    }
    else
    {
        struct log_node *temp=move_log;
        move_log=move_log->next;
        return temp;
    }
}

void push_captured(int color,int coin)
{
    if(color==WHITE)
    {
        white_captured[++wct]=coin;
    }
    else
    {
        black_captured[++bct]=coin;
    }
}

void pop_captured(int color)
{
    if(color==WHITE)
    {
        wct--;
    }
    else
    {
        bct--;
    }
}

int value(int pos, int coin_type)
{
    if(coin_type==QUEEN || coin_type==ROOK || coin_type==PAWN)
    {
        return 10;
    }
    else if(coin_type==KNIGHT)
    {
        return 9;
    }
    else if(coin_type==BISHOP)
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
    if(color(coin)==BLACK)
    {
        black_material+=value(pos,coin_type(coin));
    }
    else
    {
        white_material+=value(pos,coin_type(coin));
    }
}

void sub_material(int coin, int pos)
{
    if(color(coin)==BLACK)
    {
        black_material-=value(pos,coin_type(coin));
    }
    else
    {
        white_material-=value(pos,coin_type(coin));
    }
}

void push_check_path(int pos)
{
    check_path[++cpt]=pos;
}

int can_move_news(int coin)
{
    return coin_type(coin)!=KNIGHT && coin_type(coin)!=BISHOP;
}

int is_en_passant(int start,int dest)
{
    if(move_log==NULL)
    {
        return 0;
    }
    else if(coin_type(move_log->from_coin)!=PAWN)
    {
        return 0;
    }
    int last_mv_steps = row(move_log->from) - row(move_log->to);
    if(color(Coin(start))==WHITE && last_mv_steps == -2 && dest+S==move_log->to)
    {
        return EN_PASSANT;
    }
    else if(color(Coin(start))==BLACK && last_mv_steps == 2 && dest+N==move_log->to)
    {
        return EN_PASSANT;
    }
    return 0;
}

int pawn_move(int start,int dest)
{
    int clr=color(Coin(start)),steps=row(start)-row(dest);
    if(clr==WHITE && Coin(dest)==0 && column(start)==column(dest) && steps>0)
    {
        if(steps==1)
        {
            return 1;
        }
        else if(steps==2 && row(start)==6 && news_path(start,dest))
        {
            return 1;
        }
        return 0;
    }
    else if(clr==WHITE && steps==1 && column(start)!=column(dest))
    {
        if(color(Coin(dest))==BLACK)
        {
            return 1;
        }
        else if(row(start)==3)
        {
            return is_en_passant(start,dest);
        }
    }

    if(clr==BLACK && Coin(dest)==0 && column(start)==column(dest) && steps<0)
    {
        if(steps==-1)
        {
            return 1;
        }
        else if(steps==-2 && row(start)==1 && news_path(start,dest))
        {
            return 1;
        }
        return 0;
    }
    else if(clr==BLACK && steps==-1 && column(start)!=column(dest))
    {
        if(color(Coin(dest))==WHITE)
        {
            return 1;
        }
        else if(row(start)==4)
        {
            return is_en_passant(start,dest);
        }
    }
    return 0;
}

int is_valid_pos(int pos)
{
    return row(pos)>=0 && row(pos)<8 && column(pos)<8 && column(pos)>=0;
}

int steps_limit(int start,int dest)
{
    if(coin_type(Coin(start))==KING)
    {
        int row_steps=row(start)-row(dest);
        int column_steps=column(start)-column(dest);
        return  row_steps==1 || row_steps==-1 || column_steps==1 || column_steps==-1;
    }
    else if(coin_type(Coin(start))==PAWN)
    {
        return pawn_move(start,dest);
    }
    else
    {
        return 1;
    }
}

int news_path(int start,int dest)
{
    if(start > dest)
    {
        swap(&start,&dest);
    }
    if(column(start)==column(dest))
    {
        int j=column(start);
        for(int i=row(start)+1;i<row(dest);i++)
        {
            if(board[i][j]!=0)
            {
                return 0;
            }
        }
    }
    else if(row(start)==row(dest))
    {
        int j=row(start);
        for(int i=column(start)+1;i<column(dest);i++)
        {
            if(board[j][i]!=0)
            {
                return 0;
            }
        }
    }
    return 1;
}

int cross_path(int start,int dest)
{
    if(start < dest)
    {
        swap(&start,&dest);
    }
    int steps=row(start)-row(dest);
    if(column(start) < column(dest))
    {
        int pos=start;
        for(int i=1;i<steps;i++)
        {
            pos=pos+NE;
            if(Coin(pos)!=0)
            {
                return 0;
            }
        }
    }
    else if(column(start) > column(dest))
    {
        int pos=start;
        for(int i=1;i<steps;i++)
        {
            pos=pos+NW;
            if(Coin(pos)!=0)
            {
                return 0;
            }
        }
    }
    return 1;
}

int is_cross_move(int start,int dest)
{
    if(start < dest)
    {
        swap(&start,&dest);
    }
    int steps=row(start)-row(dest);
    return start+NW*steps==dest || start+NE*steps==dest;
}

int can_move_cross(int coin)
{
   return coin_type(coin)!=KNIGHT && coin_type(coin)!=ROOK;
}

int is_knight(int coin)
{
    return coin_type(coin)==KNIGHT;
}

int* generate_knight_moves(int pos)
{
    knight_moves[0]=pos+S+S+E;
    knight_moves[1]=pos+N+N+E;
    knight_moves[2]=pos+N+N+W;
    knight_moves[3]=pos+S+S+W;
    knight_moves[4]=pos+S+E+E;
    knight_moves[5]=pos+N+E+E;
    knight_moves[6]=pos+N+W+W;
    knight_moves[7]=pos+S+W+W;
    return knight_moves;
}

int is_knight_move(int start,int dest)
{
    int *moves=generate_knight_moves(start);
    for(int i=0;i<8;i++)
    {
        if(is_valid_pos(moves[i]) && moves[i]==dest)
        {
            return 1;
        }
    }
    return 0;
}

int is_news_move(int start,int dest)
{
    return row(start)==row(dest) || column(start)==column(dest);
}

int validate_move(int start,int dest)
{
    if(!is_valid_pos(start) || !is_valid_pos(dest))
    {
        return 0;
    }
    else if(color(Coin(start))==color(Coin(dest)))
    {
        return 0;
    }
    else if(is_knight(Coin(start)))
    {
        return is_knight_move(start,dest) && !is_check_after_move(start,dest,NORMAL);
    }
    else if(can_move_news(Coin(start)) && is_news_move(start,dest))
    {
        if(coin_type(Coin(start))==KING || coin_type(Coin(start))==PAWN)
        {
            return steps_limit(start,dest) && !is_check_after_move(start,dest,NORMAL);
        }
        else
        {
            return news_path(start,dest) && !is_check_after_move(start,dest,NORMAL);
        }
    }
    else if(can_move_cross(Coin(start)) && is_cross_move(start,dest))
    {
        if(coin_type(Coin(start))==KING)
        {
            return steps_limit(start,dest) && !is_check_after_move(start,dest,NORMAL);
        }
        else if(coin_type(Coin(start))==PAWN)
        {
            int move_type=pawn_move(start,dest);
            if(move_type!=0 && !is_check_after_move(start,dest,move_type))
            {
                return move_type;
            }
            return 0;
        }
        else
        {
            return cross_path(start,dest) && !is_check_after_move(start,dest,NORMAL);
        }
    }
    return 0;
}

int is_check(int king_color,int square)
{
    cpt=-1;
    int x=row(square),y=column(square);
    for(int i=x-1;i>=0;i--)
    {
        push_check_path(i*10+y);
        if(color(board[i][y])==king_color)
        {
           break;
        }
        else if(board[i][y]!=0 && can_move_news(board[i][y]) && steps_limit(i*10+y,square))
        {
            return i*10+y;
        }
        else if(board[i][y]!=0)
        {
            break;
        }
    }
    cpt=-1;
    for(int i=square/10+1;i<=7;i++)
    {
        push_check_path(i*10+y);
        if(color(board[i][y])==king_color)
        {
            break;
        }
        else if(board[i][y]!=0 && can_move_news(board[i][y]) && steps_limit(i*10+y,square))
        {
            return i*10+y;
        }
        else if(board[i][y]!=0)
        {
            break;
        }
    }
    cpt=-1;
    for(int i=square%10+1;i<=7;i++)
    {
        push_check_path(x*10+i);
        if(color(board[x][i])==king_color)
        {
            break;
        }
        else if(board[x][i]!=0 && can_move_news(board[x][i]) && steps_limit(x*10+i,square))
        {
            return x*10+i;
        }
        else if(board[x][i]!=0)
        {
            break;
        }
    }
    cpt=-1;
    for(int i=square%10-1;i>=0;i--)
    {
        push_check_path(x*10+i);
        if(color(board[x][i])==king_color)
        {
            break;
        }
        else if(board[x][i]!=0 && can_move_news(board[x][i]) && steps_limit(x*10+i,square))
        {
            return x*10+i;
        }
        else if(board[x][i]!=0)
        {
            break;
        }
    }
    //north west
    int min,max;
    if(x<y)
    {
        min=x;
        max=y;
    }
    else
    {
        min=y;
        max=x;
    }
    int pos=square;
    cpt=-1;
    for(int i=0;i<min;i++)
    {
        pos=pos+NW;
        push_check_path(pos);
        if(color(Coin(pos))==king_color)
        {
            break;
        }
        else if(Coin(pos)!=0 && can_move_cross(Coin(pos)) && steps_limit(pos,square))
        {
            return pos;
        }
        else if(Coin(pos)!=0)
        {
            break;
        }
    }
    //south east mv
    cpt=-1;
    pos=square;
    for(int i=0;i<(7-max);i++)
    {
        pos=pos+SE;
        push_check_path(pos);
        if(color(Coin(pos))==king_color)
        {
            break;
        }
        else if(Coin(pos)!=0 && can_move_cross(Coin(pos)) && steps_limit(pos,square))
        {
            return pos;
        }
        else if(Coin(pos)!=0)
        {
            break;
        }
    }
    //north east
    cpt=-1;
    int steps;
    if((x+y)<7)
    {
        steps=x;
    }
    else
    {
        steps=7-y;
    }
    pos=square;
    for(int i=0;i<steps;i++)
    {
        pos=pos+NE;
        push_check_path(pos);
        if(color(Coin(pos))==king_color)
        {
            break;
        }
        else if(Coin(pos)!=0 && can_move_cross(Coin(pos)) && steps_limit(pos,square))
        {
            return pos;
        }
        else if(Coin(pos)!=0)
        {
            break;
        }
    }
    //south west
    cpt=-1;
    if((x+y)<7)
    {
        steps=y;
    }
    else
    {
        steps=7-x;
    }
    pos=square;
    for(int i=0;i<steps;i++)
    {
        pos=pos+SW;
        push_check_path(pos);
        if(color(Coin(pos))==king_color)
        {
            break;
        }
        else if(Coin(pos)!=0 && can_move_cross(Coin(pos)) && steps_limit(pos,square))
        {
            return pos;
        }
        else if(Coin(pos)!=0)
        {
            break;
        }
    }
    //knight check
    cpt=-1;
    int op_color;
    if(king_color==WHITE)
    {
        op_color=BLACK;
    }
    else
    {
        op_color=WHITE;
    }
    int *moves=generate_knight_moves(square);
    for(int i=0;i<8;i++)
    {
        if(is_valid_pos(moves[i]) && Coin(moves[i])==op_color*10+KNIGHT)
        {
            push_check_path(moves[i]);
            return check_path[cpt];
        }
    }
    return -1;
}

int is_check_after_move(int start,int dest,int move_type)
{
    int start_coin=Coin(start),dest_coin=Coin(dest),check,king_pos;
    if(coin_type(start_coin)==KING)
    {
        king_pos=dest;
    }
    else if(color(start_coin)==WHITE)
    {
        king_pos=white_king_pos;
    }
    else
    {
        king_pos=black_king_pos;
    }
    board[row(dest)][column(dest)]=board[row(start)][column(start)];
    board[row(start)][column(start)]=0;
    int en_pass_pos,en_coin;
    if(move_type == EN_PASSANT)
    {
        if(color(start_coin) == BLACK)
        {
            en_pass_pos=dest+N;
            en_coin=Coin(en_pass_pos);
            board[row(dest+N)][column(dest+N)]=0;
        }
        else
        {
            en_pass_pos=dest+S;
            en_coin=Coin(en_pass_pos);
            board[row(dest+S)][column(dest+S)]=0;
        }
    }
    check=is_check(color(start_coin),king_pos);
    board[row(start)][column(start)]=start_coin;
    board[row(dest)][column(dest)]=dest_coin;
    if(move_type == EN_PASSANT)
    {
        board[row(en_pass_pos)][column(en_pass_pos)]=en_coin;
    }
    return check!=-1;
}

void en_passant(int start,int dest)
{
    if(color(Coin(start))==BLACK)
    {
        sub_material(Coin(dest+N), -1);
        push_captured(color(Coin(dest+N)),Coin(dest+N));
        delete_position(Coin(dest+N),dest+N);
        board[row(dest+N)][column(dest+N)]=0;
    }
    else
    {
        sub_material(Coin(dest+S), -1);
        push_captured(color(Coin(dest+S)),Coin(dest+S));
        delete_position(Coin(dest+S),dest+S);
        board[row(dest+S)][column(dest+S)]=0;
    }
}

int move(int start,int dest)
{
    int move_type = validate_move(start,dest);
    if(move_type)
    {
        push_log(start,Coin(start),dest,Coin(dest),move_type);
        add_position(Coin(start),dest,start);
        if(Coin(dest)!=0)
        {
            sub_material(Coin(dest), dest);
            push_captured(color(Coin(dest)),Coin(dest));
            delete_position(Coin(dest),dest);
        }
        else if(move_type == EN_PASSANT)
        {
            en_passant(start,dest);
        }
        board[row(dest)][column(dest)]=Coin(start);
        board[row(start)][column(start)]=0;
        if(coin_type(Coin(dest))==PAWN && can_promote_pawn(color(Coin(dest)),dest))
        {
            promote_pawn(dest);
        }
        return 1;
    }
    return 0;
}

int undo()
{
    struct log_node *temp=pop_log();
    if(temp!=NULL)
    {
        if(coin_type(temp->from_coin)==PAWN && (row(temp->to)==0 || row(temp->to)==7))
        {
            add_position(temp->from_coin,temp->from,-1);
            delete_position(Coin(temp->to),temp->to);
            add_material(temp->from_coin, temp->from);
            sub_material(Coin(temp->to), temp->to);
        }
        else
        {
            add_position(temp->from_coin,temp->from,temp->to);
        }
        if(temp->to_coin!=0)
        {
            pop_captured(color(temp->to_coin));
            add_position(temp->to_coin,temp->to,-1);
            add_material(temp->to_coin, temp->to);
        }
        else if(temp->move_type==EN_PASSANT)
        {
            if(color(temp->from_coin)==WHITE)
            {
                pop_captured(BLACK);
                add_material(BLACK*10+PAWN, -1);
                add_position(BLACK*10+PAWN,temp->to+S,-1);
                board[row(temp->to+S)][column(temp->to+S)]=BLACK*10+PAWN; 
            }
            else
            {
                pop_captured(WHITE);
                add_material(WHITE*10+PAWN, -1);
                add_position(WHITE*10+PAWN,temp->to+N,-1);
                board[row(temp->to+N)][column(temp->to+N)]=WHITE*10+PAWN; 
            }
        }
        if(temp->from_coin==WHITE*10+KING)
        {
            white_king_pos=temp->from;
        }  
        else if(temp->from_coin==BLACK*10+KING)
        {
            black_king_pos=temp->from;
        }
        board[row(temp->from)][column(temp->from)]=temp->from_coin;
        board[row(temp->to)][column(temp->to)]=temp->to_coin;   
        free(temp);
        display_name_board();
        //display_board();
        return 1;
    }
    else
    {
        return 0;
    }
}

int can_promote_pawn(int color,int pos)
{
    return (row(pos)==0 && color==WHITE) || (row(pos)==7 && color==BLACK);
}

void promote_pawn(int pos)
{
    int coin;
    while(1)
    {
        printf("4.queen,5.knight,7.rook,8.bishop ");
        scanf("%d",&coin);
        if(coin==QUEEN || coin==KNIGHT || coin==ROOK || coin==BISHOP)
        {
            delete_position(Coin(pos),pos);
            sub_material(Coin(pos), pos);
            board[row(pos)][column(pos)]=color(Coin(pos))*10+coin;
            add_position(Coin(pos),pos,-1);
            add_material(Coin(pos), pos);
            break;
        }
    }
}

int cover_check(int pos,int color)
{
    struct pos_node **hashtable,*temp;
    if(color == BLACK)
    {
        hashtable=black_positions;
    }
    else
    {
        hashtable=white_positions;
    }
    for(int i=0;i<6;i++)
    {
        temp=hashtable[i];
        while(temp!=NULL)
        {
            if(coin_type(Coin(temp->pos))!=KING && validate_move(temp->pos,pos))
            {
                return Coin(temp->pos);
            }
            temp=temp->next;
        }
    }
    return 0;
}

int is_check_covered(int color)
{
    int cover_coin;
    for(int i=cpt;i>=0;i--)
    {
        cover_coin=cover_check(check_path[i],color);
        if(cover_coin)
        {
            return cover_coin;
        }
    }
    return 0;
}

int one_news_move(int pos)
{
    return validate_move(pos,pos+N) || validate_move(pos,pos+S) || validate_move(pos,pos+W) || validate_move(pos,pos+E);
}

int one_cross_move(int pos)
{
    return validate_move(pos,pos+NE) || validate_move(pos,pos+NW) || validate_move(pos,pos+SE) || validate_move(pos,pos+SW);
}

int one_pawn_move(int pos)
{
    if(color(Coin(pos))==BLACK)
    {
        return validate_move(pos,pos+S) || validate_move(pos,pos+SE) || validate_move(pos,pos+SW);
    }
    else
    {
        return validate_move(pos,pos+N) || validate_move(pos,pos+NE) || validate_move(pos,pos+NW);
    }
}

int one_knight_move(int clr,int pos)
{
    int *moves=generate_knight_moves(pos);
    for(int i=0;i<8;i++)
    {
        if(is_valid_pos(moves[i]) && color(Coin(moves[i]))!=clr)
        {
            return !is_check_after_move(pos,moves[i],NORMAL);
        }
    }
    return 0;
}

int one_king_move(int pos)
{
    return one_news_move(pos) || one_cross_move(pos);
}

int one_move(int type,int pos)
{
    if(type==PAWN)
    {
        return one_pawn_move(pos);
    }
    else if(type==ROOK)
    {
       return one_news_move(pos);
    }
    else if(type==BISHOP)
    {
        return one_cross_move(pos);
    }
    else if(type==KNIGHT)
    {
        return one_knight_move(color(Coin(pos)),pos);
    }
    else if(type==QUEEN)
    {
        return one_news_move(pos) || one_cross_move(pos);
    }
    return 0;
}

int have_one_move(int color)
{
    struct pos_node **positions,*coins;
    if(color==BLACK)
    {
        positions=black_positions;
    }
    else
    {
        positions=white_positions;
    }
    for(int i=0;i<6;i++)
    {
        coins=positions[i];
        while(coins!=NULL)
        {
            if(one_move(coin_type(Coin(coins->pos)),coins->pos))
            {
                return 1;
            }
            coins=coins->next;
        }
    }
    return 0;
}

int is_game_over(int color,int king_position)
{
    int king_can_move=one_king_move(king_position);
    if(black_material+white_material<10 || (black_material+white_material <=16 && black_material==white_material))
    {
        display_name_board();
        printf("Drawn by insufficient material\n");
        return 1;
    }
    else if(!king_can_move && is_check(color,king_position)!=-1 && !is_check_covered(color))
    {
        display_name_board();
        if(color==BLACK)
        {
            printf("White won by checkmate\n");
        }
        else
        {
            printf("Black won by checkmate\n");
        }
        return 1;
    }
    else if(!king_can_move && !have_one_move(color))
    {
        display_name_board();
        printf("Drawn by Stale mate\n");
        return 1;
    }
    return 0;
}

//storing in a file
void write_move_log(FILE *file,struct log_node*temp)
{
    if(temp!=NULL)
    {
        write_move_log(file,temp->next);
    }
    if(temp!=NULL)
    {
        fprintf(file,"%d %d %d %d %d ",temp->from,temp->from_coin,temp->to,temp->to_coin,temp->move_type);
    }
}
void read_or_write_board(char mode)
{
    FILE *file;
    char file_name[100];
    sprintf(file_name,"%d.txt",file_id);
    if(mode=='w')
    {
        file = fopen(file_name,"w");
        if(file==NULL)
        {
            printf("error opening the file\n");
            return;
        }
        for(int i=0;i<8;i++)
        {
            for(int j=0;j<8;j++)
            {   
                if(board[i][j]!=0)
                {
                    fprintf(file,"%d %d ",i*10+j,board[i][j]);
                }
            }
        }
        fprintf(file,"%d %d\n",-1,-1);
        //storing captured coins
        //white_positions
        for(int i=0;i<=wct;i++)
        {
            fprintf(file,"%d ",white_captured[i]);
        }
        //black_positions
        for(int i=0;i<=bct;i++)
        {
            fprintf(file,"%d ",black_captured[i]);
        }
        fprintf(file,"%d\n",-1);

        write_move_log(file,move_log);
        fprintf(file,"%d %d %d %d %d\n",-1,-1,-1,-1,-1);
        fprintf(file,"%d %d\n",white_king_pos,black_king_pos);
        fprintf(file,"%d %d\n",white_material,black_material);
        if(fclose(file)!=0)
        {
            printf("error closing the file\n");
        }
    }
    else if(mode == 'r')
    {
        int pos,coin;
        //file =fopen("chesslog.txt","r");
        file =fopen(file_name,"r");
        if(file==NULL)
        {
            printf("error opening the file\n");
            return;
        }
        while(fscanf(file,"%d %d",&pos,&coin)!=EOF)
        {
            if(pos==-1)
            {
                break;
            }
            board[row(pos)][column(pos)]=coin;
        }
        //reading captured coins
        while(fscanf(file,"%d ",&coin)!=EOF)
        {
            if(coin==-1)
            {
                break;
            }
            push_captured(color(coin),coin);
        }
        //reading_move_log
        int from,from_coin,to,to_coin,mv_type;
        while(fscanf(file,"%d %d %d %d %d",&from,&from_coin,&to,&to_coin,&mv_type)!=EOF)
        {
            if(from == -1)
            {
                break;
            }
            push_log(from,from_coin,to,to_coin,mv_type);
        }
        fscanf(file,"%d %d",&white_king_pos,&black_king_pos);
        fscanf(file,"%d %d",&white_material,&black_material);
        if(fclose(file)!=0)
        {
            printf("error closing the file\n");
        }
    }
}

void start_game()
{
    int choice;
    while(1)
    {
        printf("\n1.New game 2.continue 3.settings ");
        scanf("%d",&choice);
        if(choice==1)
        {
            printf("enter any id(number) for this game ");
            scanf("%d",&file_id);
            chess_board();
            break;
        }
        else if(choice==2)
        {
            printf("enter id of previous game");
            scanf("%d",&file_id);
            read_or_write_board('r');
            if(move_log!=NULL && color(move_log->from_coin)==WHITE)
            {
                white_move=0;
                if(is_game_over(BLACK,black_king_pos))
                {
                    exit(0);
                }
            }
            else
            {
                white_move==1;
                if(is_game_over(WHITE,white_king_pos))
                {
                    exit(0);
                }
            }
            break;
        }
        else if(choice==3)
        {
            char as;
            printf("do you want to enable autosave? y/n");
            scanf(" %c",&as);
            if(as=='y' || as=='Y')
            {
                autosave=1;
            }
            else if(as=='n' || as=='N')
            {
                printf("-2,-2 to save file\n");
                autosave=0;
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
    struct pos_node *prev,*temp;
    for(int i=0;i<=7;i++)
    {
        temp=positions[i];
        while(temp!=NULL)
        {
            prev=temp;
            temp=temp->next;
            free(prev);
        }
    }
}

void destruct()
{
    struct log_node *prev,*temp=move_log;
    while(temp!=NULL)
    {
        prev=temp;
        temp=temp->next;
        free(prev);
    }
    free_positions(white_positions);
    free_positions(black_positions);
}

void construct()
{
    init_hash_table();
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
    construct();
    display_name_board();
    //display_board();
    int from,to;
    while(1)
    {
        if(autosave==1)
        {
            read_or_write_board('w');
        }
        if(white_move)
        {
            printf("whites move ");
            scanf("%d %d",&from,&to);
            if((from==3 && to==3) && undo())
            {
                white_move=0;
                continue;
            }
            else if(from==-2 && to ==-2)
            {
                read_or_write_board('w');
                printf("saved\n");
                continue;
            }
            else if(!is_valid_pos(from) || !is_valid_pos(to))
            {
                continue;
            }
            else if(color(Coin(from))!=WHITE)
            {
                continue;
            }
            else if(!move(from,to))
            {
                printf("wrong move\n");
                continue;
            }
            else
            {
                if(Coin(to)==WHITE*10+KING)
                {
                    white_king_pos=to;
                }
                if(is_game_over(BLACK,black_king_pos))
                {
                    if(autosave==1)
                    {
                        read_or_write_board('w');
                    }
                    break;
                }
            }
            white_move=0;
        }
        else
        {
            printf("black move ");
            scanf("%d %d",&from,&to);
            if((from==3 && to==3) && undo())
            {
                white_move=1;
                continue;
            }
            else if(from==-2 && to ==-2)
            {
                read_or_write_board('w');
                printf("saved\n");
                continue;
            }
            else if(!is_valid_pos(from) || !is_valid_pos(to))
            {
                continue;
            }
            else if(color(Coin(from))!=BLACK)
            {
                continue;
            }
            else if(!move(from,to))
            {
                printf("wrong move\n");
                continue;
            }
            else
            {
                if(Coin(to)==BLACK*10+KING)
                {
                    black_king_pos=to;
                }
                if(is_game_over(WHITE,white_king_pos))
                {
                    if(autosave==1)
                    {
                        read_or_write_board('w');
                    }
                    break;
                }
            }
            white_move=1;
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
    struct pos_node*temp;
    for(int i=0;i<6;i++)
    {
        temp=positions[i];
        printf("%d --",i);
        while(temp!=NULL)
        {
            printf("<%d,%d> ",temp->pos,Coin(temp->pos));
            temp=temp->next;
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
    for(int i=0;i<=12;i++)
    {
        printf(" ");
    }
}

void print_white_space()
{
    for(int i=0;i<=12;i++)
    {
        printf(":");
    }
}

void print_line()
{
    for(int i=0;i<=7;i++)
    {
        printf("______________");
    }
    printf("\n");
}

void display_row(int board[][7],int row,int color)
{

    for(int j=0;j<7;j++)
    {
        if(color==WHITE)
        {
            if(board[row][j]==0)
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
            if(board[row][j]==0)
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

void coin_shape(int coin,int row,int color)
{
    if(coin_type(coin)==PAWN)
    {
       display_row(pawn_shape,row,color);
    }
    else if(coin_type(coin)==ROOK)
    {
        display_row(rook_shape,row,color);  
    }
    else if(coin_type(coin)==BISHOP)
    {
        display_row(bishop_shape,row,color);  
    }
    else if(coin_type(coin)==KNIGHT)
    {
        display_row(knight_shape,row,color);  
    }
    else if(coin_type(coin)==KING)
    {
        display_row(king_shape,row,color);  
    }
    else if(coin_type(coin)==QUEEN)
    {
        display_row(queen_shape,row,color);  
    }
}

void display_board()
{
    display_captured(black_captured,bct);
    printf("\t  ");
    for(int i=0;i<8;i++)
    {
        printf("     %d        ",i);
    }
    printf("\n");
    for(int i=0;i<=7;i++)
    {
        printf("\t");
        print_line();
        for(int k=0;k<=6;k++)
        {
            if(k==3)
            {
                printf("%d ",i);
            }

            printf("\t|");        
            for(int j=0;j<=7;j++)
            {
                if(board[i][j]!=0)
                {
                    printf("   ");
                    coin_shape(board[i][j],k,board[i][j]/10);
                    printf("   |");
                }
                else if((j+i)%2 == 0)
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
    display_captured(white_captured,wct);
}

void num_to_char(int num)
{
    if(num==WHITE)
    {
        printf("w");
    }
    if(num==BLACK)
    {
        printf("b");
    }
    if(num==KING)
    {
        printf("king");
    }
    if(num==QUEEN)
    {
        printf("queen");
    }
    if(num==KNIGHT)
    {
        printf("knight");
    }
    if(num==PAWN)
    {
        printf("pawn");
    }
    if(num==ROOK)
    {
        printf("rook");
    }
    if(num==BISHOP)
    {
        printf("bishop");
    }
}

void display_captured(int *captured,int top)
{
    printf("\t");
    for(int i=0;i<=top;i++)
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
    for(int i=0;i<=7;i++)
    {
        for(int j=0;j<=7;j++)
        {
            if(i==1)
            {
                board[i][j]=26;
            }
            else if(i==6)
            {
                board[i][j]=16;
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
    for(int i=0;i<7;i++)
    {
        for(int j=0;j<7;j++)
        {
            if(i==0 || i==6)
            {
                rook_shape[i][j]=1;
            }
            else
            {
                if(j==0 || j==6)
                {
                    rook_shape[i][j]=0;
                }
                else
                {
                    rook_shape[i][j]=1;
                }
            }
        }
        printf("\n");
    }
}

void display_name_board()
{
    display_captured(black_captured,bct);
    printf("\t");
    for(int i=0;i<8;i++)
    {
        printf("%d\t",i);
    }
    printf("\n\n");
    for(int i=0;i<8;i++)
    {
        printf("%d\t",i);
        for(int j=0;j<8;j++)
        {
            num_to_char(color(board[i][j]));
            num_to_char(coin_type(board[i][j]));
            printf("\t");
        }
        printf("\n\n");
    }
    display_captured(white_captured,wct);
}