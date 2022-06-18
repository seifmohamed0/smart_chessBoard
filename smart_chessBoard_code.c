// author : seif elden mohamed
#include "arduino.h"
#include <Adafruit_NeoPixel.h>

#define CHIPS_N   4
#define DATA_N   CHIPS_N * 8
#define PULSE   5
#define MAX_EATS 18
#define MAX_MOVES  36
#define _32_BIT unsigned long
//////////////////////////////////////// leds part begin /////////////////////////////////////////////////////////////////

#define led_strip_pin 15
#define led_strip_size 64
Adafruit_NeoPixel strip = Adafruit_NeoPixel(led_strip_size, led_strip_pin, NEO_GRB + NEO_KHZ800);

//////////////////////////////////// reed switches part begin /////////////////////////////////////////////////////////////////

#define shift_registers_delay   1 // Optional delay between shift register reads.

//Chips 1-4
int LOAD_PIN = 25;  // Connects to Parallel load pin the 165
int CLOCK_ENABLE_BIN = 27;  // Connects to Clock Enable pin the 165
int CLOCK_PIN = 26; // Connects to the Clock pin the 165
int _DATA_PIN_0TO31 = 17; // Connects to the Q7 pin the 165
int _DATA_PIN_32TO63 = 18;

_32_BIT _bit_values_from_0to31 = 0;
_32_BIT _bit_values_from_32to63 = 0;
_32_BIT switches_values_0to31;
_32_BIT old_ReedValues_0to31;
_32_BIT switches_values_32to63;
_32_BIT old_ReedValues_32to63;
_32_BIT input_values0to31;
_32_BIT input_values32to63;
_32_BIT tmp_val_0to31;
_32_BIT tmp_val_32to63;
_32_BIT moves_state_0to31;
_32_BIT moves_state_32to63;
const _32_BIT switches_0to31_StartingPos = 65535; // first 16 bit = 1  00000000000000001111111111111111
const _32_BIT switches_32to63_StartingPos = 4294901760; // second 16 bit = 111111111111111110000000000000000

/////////////////////////////////// reed switches part end ////////////////////////////////////////////////////////////////////////


int board[64] , tmp_board[64];
bool white_turn;
int eats[MAX_EATS], eats_ptr;
int moves[MAX_MOVES] ,  moves_ptr;
int bit_sign;
int last_move;
int knight_xd[] = {2 , -2 , 1 , -1};
int knight_yd[] = {2 , -2 , 1 , -1};
int king_xd[] = {0 , 0 , 1 , -1 , 1 , 1 , -1 , -1};
int king_yd[] = {1 , -1 , 0 , 0 , 1 , -1 , 1 , -1};
// some colours code

uint32_t colour_off = strip.Color(0, 0, 0);
uint32_t colour_yellow = strip.Color(255 , 255 , 0);
uint32_t colour_purple = strip.Color(127, 0, 255); //purple
uint32_t colour_green = strip.Color(0, 255, 0); //green
uint32_t colour_red = strip.Color(255, 0, 0); //red
uint32_t colour_orange = strip.Color(255, 165, 0); //orange
uint32_t colour_blue = strip.Color(0, 0, 255); //blue
uint32_t colour_out = strip.Color(219, 182, 92);
uint32_t colour_in = strip.Color(245, 63, 7);
uint32_t colour_warmwhite = strip.Color(239, 235, 216); //warm white
uint32_t colour_white = strip.Color(255, 255, 255); //warm white
uint32_t colour_selected = strip.Color(0, 0, 0);
uint32_t colour_eat = strip.Color(255, 165, 0);
uint32_t colour_move = strip.Color(0, 255, 0);
uint32_t colour_eat2 = strip.Color(46, 156, 108);
///////////////////////////////////// leds part end ////////////////////////////////////////////////////////////////////////////

enum {
  WKI = 1 , WQU , WRO , WBI , WKN , WPA , BKI , BQU , BRO , BBI , BKN , BPA
};


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  strip.begin();
  strip.setBrightness(40);
  strip.show();
  initPins();
  init_game();
  init_board_places();
}

void loop() {

  // printing bitboard on serial monitor


  // leds show state for every piece to make it easy to figure out wrong positions

  leds_show_state();

  // this step check if only one piece moved or not and if only one piece moved it returns cell_number else it returns -1

  int cell = get_changed_cells();

  // first we check if there is only one piece moved
  // otherwise the game will be paused till every piece go back to its valid position

  if (cell != -1) {

    // here we check if the moved piece is white or not

    int cell_is_white = is_white(cell);

    // here there is only two valid states
    // first valid move if the moved piece is white and it's white_player turn
    // second valid move if the moved piece is black and it's black_player turn

    if (white_turn && cell_is_white) {

      // when it's white player turn and moved cell is white
      // we use generate possible moves function to generate all possible moves
      // we store this moves in to arrays first one for ordinary moves and second one for eats_moves

      generate_possible_moves(cell);

      // the next step we show all possible moves in leds to let the players know valid moves

      show_possible_moves(); // led system function calling

      // here we get the play that player chose

      get_the_move(cell);

      leds_off();
      check_promotion(last_move);
      // fix led system after each play show us the state of all the board by leds

      leds_off();

      // here we flip the player's turn

      white_turn ^= 1;

    } else if (!white_turn && (!cell_is_white)) {
      generate_possible_moves(cell);
      show_possible_moves(); // led system function calling
      get_the_move(cell);
      leds_off();
      check_promotion(last_move);
      leds_off();
      white_turn ^= 1;
    }
  }
  delay(300);
}
bool is_black_pawn_file_rank(int file , int rank) {
  // given file and rank 
  // check if positions is valid or not 
  // if positions is valid & this cell is black pawn return true
  if (file < 0 || file > 7 || rank < 0 || rank > 7 )
    return false;
  if (tmp_board[get_cell_file_rank(file , rank)] == BPA)
    return true;
  return false;
}
bool is_black_knight_file_rank(int file , int rank) {
 // given file and rank 
  // check if positions is valid or not 
  // if positions is valid & this cell is black knight return true
  if (file < 0 || file > 7 || rank < 0 || rank > 7 )
    return false;
  if (tmp_board[get_cell_file_rank(file , rank)] == BKN)
    return true;
  return false;
}
bool is_white_pawn_file_rank(int file , int rank) {
  if (file < 0 || file > 7 || rank < 0 || rank > 7 )
    return false;
  if (tmp_board[get_cell_file_rank(file , rank)] == WPA)
    return true;
  return false;
}
bool is_white_knight_file_rank(int file , int rank) {
  if (file < 0 || file > 7 || rank < 0 || rank > 7 )
    return false;
  if (tmp_board[get_cell_file_rank(file , rank)] == WKN)
    return true;
  return false;
}
bool is_white_in_tmp_board(int cell) {
  return tmp_board[cell] >= 1 && tmp_board[cell] <= 6;
}
bool is_black_in_tmp_board(int cell) {
  if (cell < 0 || cell > 63)return false;
  return tmp_board[cell] >= 7 && tmp_board[cell] <= 12;
}

bool is_white_king_under_attack_vir() {
  // after we make a virtual board in tmp_board array
  // we check if the white king is under attack in this virtual board
  int white_king_pos = -1;
  for (int i = 0; i < 64 ; i++) {
    if (tmp_board[i] == WKI) {
      white_king_pos = i;
      break;
    }
  }
  int file = white_king_pos / 8 , rank = white_king_pos % 8;
  // check pawn
  if (is_black_pawn_file_rank(file - 1 , rank - 1)) {
    return true;
  }
  if (is_black_pawn_file_rank(file - 1 , rank + 1)) {
    return true;
  }
  // check knight
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4 ; j++) {
      if (abs(knight_xd[i]) != abs(knight_yd[j])) {
        if (is_black_knight_file_rank(file + knight_xd[i] , rank + knight_yd[j])) {
          return true;
        }
      }
    }
  }
  // check rook || queen
  // up
  for (int i = 1; i <= 7 ; i++) {
    if (file - i < 0)break;
    int new_cell = get_cell_file_rank(file - i , rank);
    if (is_white_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == BRO || tmp_board[new_cell] == BQU) {
      return true;
    }
  }
  // down
  for (int i = 1; i <= 7 ; i++) {
    if (file + i > 7)break;
    int new_cell = get_cell_file_rank(file + i , rank);
    if (is_white_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == BRO || tmp_board[new_cell] == BQU) {
      return true;
    }
  }
  // right
  for (int i = 1; i <= 7 ; i++) {
    if (rank + i > 7)break;
    int new_cell = get_cell_file_rank(file , rank + i);
    if (is_white_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == BRO || tmp_board[new_cell] == BQU) {
      return true;
    }
  }
  // left
  for (int i = 1; i <= 7 ; i++) {
    if (rank - i < 0)break;
    int new_cell = get_cell_file_rank(file , rank - i);
    if (is_white_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == BRO || tmp_board[new_cell] == BQU) {
      return true;
    }
  }
  // check bishop || queen
  //up left file - rank -
  for (int i = 1 ; i <= 7 ; i++) {
    if (file - i < 0 || rank - i < 0)
      break;
    int new_cell = get_cell_file_rank(file - i , rank - i);
    if (is_white_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == BBI || tmp_board[new_cell] == BQU) {
      return true;
    }
  }
  //up right file - rank +
  for (int i = 1 ; i <= 7 ; i++) {
    if (file - i < 0 || rank + i > 7)
      break;
    int new_cell = get_cell_file_rank(file - i , rank + i);
    if (is_white_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == BBI || tmp_board[new_cell] == BQU) {
      return true;
    }
  }
  //down left file + rank -
  for (int i = 1 ; i <= 7 ; i++) {
    if (file + i > 7 || rank - i < 0)
      break;
    int new_cell = get_cell_file_rank(file + i , rank - i);
    if (is_white_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == BBI || tmp_board[new_cell] == BQU) {
      return true;
    }
  }
  //down right file + rank +
  for (int i = 1 ; i <= 7 ; i++) {
    if (file + i > 7 || rank + i > 7)
      break;
    int new_cell = get_cell_file_rank(file + i , rank + i);
    if (is_white_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == BBI || tmp_board[new_cell] == BQU) {
      return true;
    }
  }
  return false;
}

bool is_black_king_under_attack_vir() {
  int black_king_pos = -1;
  for (int i = 0; i < 64 ; i++) {
    if (tmp_board[i] == BKI) {
      black_king_pos = i;
      break;
    }
  }
  int file = black_king_pos / 8 , rank = black_king_pos % 8;
  // check pawn
  if (is_white_pawn_file_rank(file + 1 , rank - 1)) {
    return true;
  }
  if (is_white_pawn_file_rank(file + 1 , rank + 1)) {
    return true;
  }
  // check knight
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4 ; j++) {
      if (abs(knight_xd[i]) != abs(knight_yd[j])) {
        if (is_white_knight_file_rank(file + knight_xd[i] , rank + knight_yd[j])) {
          return true;
        }
      }
    }
  }
  // check rook || queen
  // up
  for (int i = 1; i <= 7 ; i++) {
    if (file - i < 0)break;
    int new_cell = get_cell_file_rank(file - i , rank);
    if (is_black_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == WRO || tmp_board[new_cell] == WQU) {
      return true;
    }
  }
  // down
  for (int i = 1; i <= 7 ; i++) {
    if (file + i > 7)break;
    int new_cell = get_cell_file_rank(file + i , rank);
    if (is_black_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == WRO || tmp_board[new_cell] == WQU) {
      return true;
    }
  }
  // right
  for (int i = 1; i <= 7 ; i++) {
    if (rank + i > 7)break;
    int new_cell = get_cell_file_rank(file , rank + i);
    if (is_black_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == WRO || tmp_board[new_cell] == WQU) {
      return true;
    }
  }
  // left
  for (int i = 1; i <= 7 ; i++) {
    if (rank - i < 0)break;
    int new_cell = get_cell_file_rank(file , rank - i);
    if (is_black_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == WRO || tmp_board[new_cell] == WQU) {
      return true;
    }
  }
  // check bishop || queen
  //up left file - rank -
  for (int i = 1 ; i <= 7 ; i++) {
    if (file - i < 0 || rank - i < 0)
      break;
    int new_cell = get_cell_file_rank(file - i , rank - i);
    if (is_black_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == WBI || tmp_board[new_cell] == WQU) {
      return true;
    }
  }
  //up right file - rank +
  for (int i = 1 ; i <= 7 ; i++) {
    if (file - i < 0 || rank + i > 7)
      break;
    int new_cell = get_cell_file_rank(file - i , rank + i);
    if (is_black_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == WBI || tmp_board[new_cell] == WQU) {
      return true;
    }
  }
  //down left file + rank -
  for (int i = 1 ; i <= 7 ; i++) {
    if (file + i > 7 || rank - i < 0)
      break;
    int new_cell = get_cell_file_rank(file + i , rank - i);
    if (is_black_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == WBI || tmp_board[new_cell] == WQU) {
      return true;
    }
  }
  //down right file + rank +
  for (int i = 1 ; i <= 7 ; i++) {
    if (file + i > 7 || rank + i > 7)
      break;
    int new_cell = get_cell_file_rank(file + i , rank + i);
    if (is_black_in_tmp_board(new_cell))
      break;
    if (tmp_board[new_cell] == WBI || tmp_board[new_cell] == WQU) {
      return true;
    }
  }
  return false;
}

void fix_tmp_board(int old_cell , int new_cell) {
  tmp_board[old_cell] = 0;
  tmp_board[new_cell] = board[old_cell];
}
void restore_tmp_board(int old_cell , int new_cell) {
  tmp_board[old_cell] = board[old_cell];
  tmp_board[new_cell] = board[new_cell];
}

void leds_show_state() {

  // leds show state if a function to display state for each cell
  // red_colour means there is a piece should be here and it's missing
  // green colour means there is a piece should be here and it's here

  // we get the new values of all 64 senosrs
  read_shift_regs();
  switches_values_0to31 = getValue0to31();
  switches_values_32to63 = getValue32to63();

  // here we check for every cell if it gonna be from the red state or green state
  bool check_places_flag = 0;
  for (int i = 0; i < 32 ; i++) {
    if (((old_ReedValues_0to31 >> i) & 1) != ((switches_values_0to31 >> i) & 1)) {
      check_places_flag = true;
      break;
    }
    if (((old_ReedValues_32to63 >> i) & 1) != ((switches_values_32to63 >> i) & 1)) {
      check_places_flag = true;
      break;
    }
  }
  if (!check_places_flag) {
    for (int i = 0; i < 32 ; i++) {
      if ((old_ReedValues_0to31 >> i) & 1) {
        Single_Led(colour_green , i);
      }
      if ((old_ReedValues_32to63 >> i) & 1) {
        Single_Led(colour_green , i + 32);
      }
    }
  } else {
    for (int i = 0; i < 32 ; i++) {
      if ((old_ReedValues_0to31 >> i) & 1) {
        if ((switches_values_0to31 >> i) & 1 ) {
          Single_Led(colour_yellow , i);
        } else {
          Single_Led(colour_red, i);
        }
      }
      if ((old_ReedValues_32to63 >> i) & 1) {
        if ((switches_values_32to63 >> i) & 1 ) {
          Single_Led(colour_yellow , i + 32);
        } else {
          Single_Led(colour_red, i + 32);
        }
      }
    }
  }
}
void leds_off() {
  // set all leds off
  for (int i = 0; i < 64 ; i++) {
    Single_Led(colour_off , i);
  }
}


bool cell_is_captured(int cell) {
  // here we check if there is a piece stands on this cell
  // first we read new values from each sensor

  read_shift_regs();
  switches_values_0to31 = getValue0to31();
  switches_values_32to63 = getValue32to63();

  // here we check if the bit_value of this cell is one or not

  if (cell < 32) {
    // in first half
    return (switches_values_0to31 >> cell) & 1;
  }
  // in second half
  cell -= 32;
  return (switches_values_32to63 >> cell) & 1;
}
void remove_old_cell_put_new_cell(int old_cell , int new_cell) {
  // when we move piece from cell to another
  // after this operation we need to make changes in the main board and in the old variables
  // first we remove the old place by taking the old value xor with the old bit so it will be flipped
  // second we add the new value by taking it or with the old value

  board[new_cell] = board[old_cell];
  if (old_cell != new_cell)
    board[old_cell] = 0;
  tmp_board[new_cell] = tmp_board[old_cell];
  if (old_cell != new_cell)
    tmp_board[old_cell] = 0;
  tmp_val_0to31 = old_ReedValues_0to31;
  tmp_val_32to63 = old_ReedValues_32to63;
  // here we remove old cell
  if (old_cell <= 31) {
    tmp_val_0to31 ^= (unsigned long) 1 << old_cell;
    moves_state_0to31 |= (unsigned long) 1 << old_cell;
  } else {
    tmp_val_32to63 ^= (unsigned long) 1 << (old_cell - 32);
    moves_state_32to63 |= (unsigned long) 1 << (old_cell - 32);
  }
  // here we add new_cell
  if (new_cell <= 31) {
    tmp_val_0to31 |= (unsigned long) 1 << new_cell;
  } else {
    tmp_val_32to63 |= (unsigned long) 1 << (new_cell - 32);
  }
  old_ReedValues_0to31 = tmp_val_0to31;
  old_ReedValues_32to63 = tmp_val_32to63;
}
bool cell_is_removed(int cell) {
  // check if the cell is on or off
  if (cell < 32) {
    return !((switches_values_0to31 >> cell) & 1) ;
  } else {
    return !((switches_values_32to63 >> (cell - 32)) & 1);
  }
}
void change_color(int cell) {
  // turn of all the leds expect this cell
  for (int i = 0; i < eats_ptr ; i++)
    Single_Led(colour_off , eats[i]);
  for (int i = 0; i < moves_ptr ; i++)
    Single_Led(colour_off , moves[i]);
  Single_Led(colour_eat2 , cell);
}
void track_eat_move(int old_cell , int new_cell) {
  // while this cell is empty we wait till there is a piece in this new_cell
  while (!cell_is_captured(new_cell)) {

    delay(300);
  }
  remove_old_cell_put_new_cell(old_cell , new_cell);
}

bool get_promotion(int cell , int state , int new_piece_value) {
  if (state == 1)
    print_queen();
  else if (state == 2)
    print_knight();
  else if (state == 3)
    print_rook();
  else if (state == 4)
    print_bishop();
  int interval = 2400;
  while (interval >= 300) {
    if (cell_is_captured(cell)) {
      board[cell] = new_piece_value;
      tmp_board[cell] = new_piece_value;
      return true;
    }
    delay(300);
    interval -= 300;
  }
  return false;
}
void get_new_white_piece(int cell) {
  // queen , knight , rook , bishop
  while (!cell_is_removed(cell)) {
    read_shift_regs();
    switches_values_0to31 = getValue0to31();
    switches_values_32to63 = getValue32to63();
    delay(300);
  }
  while (true) {
    if (get_promotion(cell , 1 , WQU))
      return;
    if (get_promotion(cell , 2 , WKN))
      return;
    if (get_promotion(cell , 3 , WRO)) {
      return;
    }
    if (get_promotion(cell , 4 , WBI)) {
      return;
    }
  }
}
void get_new_black_piece(int cell) {
  while (!cell_is_removed(cell)) {
    read_shift_regs();
    switches_values_0to31 = getValue0to31();
    switches_values_32to63 = getValue32to63();
    delay(300);
  }
  while (true) {
    if (get_promotion(cell , 1 , BQU))
      return;
    if (get_promotion(cell , 2 , BKN))
      return;
    if (get_promotion(cell , 3 , BRO)) {
      return;
    }
    if (get_promotion(cell , 4 , BBI)) {
      return;
    }
  }
}
void check_promotion(int cell) {
  int file = cell / 8 , rank = cell % 8;
  if (board[cell] == WPA && file == 0) {
    get_new_white_piece(cell);
  }
  if (board[cell] == BPA && file == 7) {
    get_new_black_piece(cell);
  }
}
void check_white_castling(int old_king_pos , int new_king_pos) {
  if (board[new_king_pos] != WKI) {
    return;
  }
  if ((new_king_pos == 58 && old_king_pos == 60) || (new_king_pos == 62 && old_king_pos == 60)) {
    if (new_king_pos == 58) {
      remove_old_cell_put_new_cell(56 , 59);
    } else {
      remove_old_cell_put_new_cell(63 , 61);
    }
  }
  return;
}
void check_black_castling(int old_king_pos , int new_king_pos) {
  if (board[new_king_pos] != BKI)
    return;
  if (old_king_pos == 4 && (new_king_pos == 2 || new_king_pos == 6)) {
    if (new_king_pos == 2) {
      remove_old_cell_put_new_cell(0 , 3);
    } else {
      remove_old_cell_put_new_cell(7 , 5);
    }
  }
}
void get_the_move(int cell) {
  moves[moves_ptr++] = cell;
  bool move_done = false;
  while (1) {
    read_shift_regs();
    switches_values_0to31 = getValue0to31();
    switches_values_32to63 = getValue32to63();
    for (int i = 0; i < moves_ptr ; i++) {
      if (cell_is_captured(moves[i])) {
        if (moves[i] == cell)
          white_turn ^= 1;
        remove_old_cell_put_new_cell(cell , moves[i]);
        check_white_castling(cell , moves[i]);
        check_black_castling(cell , moves[i]);
        last_move = moves[i];
        //check_promotion(moves[i])
        return;
      }
    }
    for (int i = 0; i < eats_ptr ; i++) {
      if (cell_is_removed(eats[i])) {
        change_color(eats[i]);
        track_eat_move(cell , eats[i]);
        last_move = eats[i];
        //check_promotion(eats[i]);
        return;
      }
    }
    delay(300);
  }
}

void show_possible_moves() {
  for (int i = 0; i < 64 ; i++)
    Single_Led(colour_off , i);
  for (int i = 0; i < eats_ptr ; i++)
    Single_Led(colour_eat , eats[i]);
  for (int i = 0; i < moves_ptr ; i++)
    Single_Led(colour_move , moves[i]);
}

void generate_possible_moves(int cell) {
  // this function used to recognize the piece on the moved cell and call its function
  eats_ptr = moves_ptr = 0; // set eats_pointer and moves_pointer equal to zero makes the two arrays empty
  if (board[cell] == WPA) {
    // white pawn
    calculate_white_pawn_moves(cell);
  } else if (board[cell] == BPA) {
    // black pawn
    calculate_black_pawn_moves(cell);
  } else if (board[cell] == WKN) {
    // white knight
    calculate_white_knight_moves(cell);
  } else if (board[cell] == BKN) {
    // black knight
    calculate_black_knight_moves(cell);
  } else if (board[cell] == WBI) {
    // white bishop
    bit_sign = 0;
    calculate_white_bishop_moves(cell);
  } else if (board[cell] == BBI) {
    // black bishop
    bit_sign = 0;
    calculate_black_bishop_moves(cell);
  } else if (board[cell] == WRO) {
    // white rook
    bit_sign = 0;
    calculate_white_rook_moves(cell);
  } else if (board[cell] == BRO) {
    // black rook
    bit_sign = 0;
    calculate_black_rook_moves(cell);
  } else if (board[cell] == WQU) {
    // white queen
    bit_sign = 0;
    calculate_white_rook_moves(cell);
    bit_sign = 0;
    calculate_white_bishop_moves(cell);
  } else if (board[cell] == BQU) {
    // black queen
    bit_sign = 0;
    calculate_black_rook_moves(cell);
    bit_sign = 0;
    calculate_black_bishop_moves(cell);
  } else if (board[cell] == WKI) {
    // white king
    calculate_white_king_moves(cell);
  } else if (board[cell] == BKI) {
    // black king
    calculate_black_king_moves(cell);
  }
}
void check_white_king_move(int file , int rank) {
  // check if this move (valid , not valid , is_eat , is_move)
  if (!x_y_on_board(file , rank)) // out of orders
    return ;
  int cell = get_cell_file_rank(file , rank);
  if (is_white(cell))return; // if this cell is white. white king can not move here
  if (is_black(cell)) { // if this cell is black so white king cat eat this piece
    eats[eats_ptr++] = cell;
    return;
  }
  // if the cell in the board and not black and not white so it's empty and this move is an ordinary move
  moves[moves_ptr++] = cell;
}
void check_black_king_move(int file , int rank) {
  // check if this move (valid , not valid , is_eat , is_move)
  if (!x_y_on_board(file , rank))   // out of orders
    return ;
  int cell = get_cell_file_rank(file , rank);
  if (is_black(cell))return;// if this cell is black. black king can not move here
  if (is_white(cell)) { // if this cell is white so black king cat eat this piece
    eats[eats_ptr++] = cell;
    return;
  }
  // if the cell in the board and not black and not white so it's empty and this move is an ordinary move
  moves[moves_ptr++] = cell;
}

bool is_moved(int cell) {
  if (cell <= 31) {
    return (moves_state_0to31 >> cell) & 1;
  } else {
    return (moves_state_32to63 >> (cell - 32)) & 1;
  }
}
void fix_tmp_board_for_castle(int old_king_pos , int new_king_pos , int old_rook_pos , int new_rook_pos) {
  tmp_board[new_king_pos] = tmp_board[old_king_pos];
  tmp_board[new_rook_pos] = tmp_board[old_rook_pos];
  tmp_board[old_king_pos] = tmp_board[old_rook_pos] = 0;
}
void check_white_left_castle(int cell) {
  if (is_moved(56))
    return;
  for (int i = 57 ; i < 60 ; i++) {
    if (!is_empty(i)) {
      return;
    }
  }
  fix_tmp_board_for_castle(60 , 58 , 56 , 59);
  if (is_white_king_under_attack_vir())
    return;
  fix_tmp_board_for_castle(58 , 60 , 59 , 56);
  moves[moves_ptr++] = 58;
}

void check_black_left_castle(int cell) {
  if (is_moved(0))
    return;
  for (int i = 1; i < 4 ; i++) {
    if (!is_empty(i)) {
      return;
    }
  }
  fix_tmp_board_for_castle(4 , 2 , 0 , 3);
  if (is_black_king_under_attack_vir())
    return;
  fix_tmp_board_for_castle(2 , 4 , 3 , 0);
  moves[moves_ptr++] = 2;
}

void check_white_right_castle(int cell) {
  if (is_moved(63))
    return;
  for (int i = 61 ; i < 63 ; i++) {
    if (!is_empty(i))
      return;
  }
  fix_tmp_board_for_castle(60 , 62 , 63 , 61);
  if (is_white_king_under_attack_vir())
    return;
  fix_tmp_board_for_castle(62 , 60 , 61 , 63);
  moves[moves_ptr++] = 62;
}
void check_black_right_castle(int cell) {
  if (is_moved(7))
    return ;
  for (int i = 5 ; i < 7 ; i++)
    if (!is_empty(i)) {
      return;
    }
  fix_tmp_board_for_castle(4 , 6 , 7 , 5);
  if (is_black_king_under_attack_vir())
    return;
  fix_tmp_board_for_castle(6 , 4 , 5 , 7);
  moves[moves_ptr++] = 6;
}
void check_white_king_castle(int cell) {
  if (cell != 60)return;
  if (is_moved(60))
    return;
  if (is_white_king_under_attack_vir())
    return;
  check_white_left_castle(cell);
  check_white_right_castle(cell);
}
void check_black_king_castle(int cell) {
  if (cell != 4)
    return;
  if (is_moved(4))
    return;
  if (is_black_king_under_attack_vir())
    return;
  check_black_left_castle(cell);
  check_black_right_castle(cell);
}
void calculate_white_king_moves(int cell) {
  // we iterate ove 8 possible white king moves and for every possible move we check its state
  int file = cell / 8 , rank = cell % 8;
  for (int i = 0; i < 8 ; i++) {
    int new_cell = get_cell_file_rank(file + king_xd[i] , rank + king_yd[i]);
    fix_tmp_board(cell , new_cell);
    if (!is_white_king_under_attack_vir())
      check_white_king_move(file + king_xd[i] , rank + king_yd[i]);
    restore_tmp_board(cell , new_cell);
  }
  check_white_king_castle(cell);
}
void calculate_black_king_moves(int cell) {
  // we iterate ove 8 possible black king moves and for every possible move we check its state
  int file = cell / 8 , rank = cell % 8;
  for (int i = 0; i < 8 ; i++) {
    int new_cell = get_cell_file_rank(file + king_xd[i] , rank + king_yd[i]);
    fix_tmp_board(cell , new_cell);
    if (!is_black_king_under_attack_vir())
      check_black_king_move(file + king_xd[i] , rank + king_yd[i]);
    restore_tmp_board(cell , new_cell);
  }
  check_black_king_castle(cell);
}
void check_white_rook_move(int file , int rank , int bit_value) {
  if (!x_y_on_board(file , rank)) {
    return;
  }
  int cell = get_cell_file_rank(file , rank);
  if (is_white(cell)) {
    bit_sign |= bit_value;
    return;
  }
  if (is_empty(cell)) {
    moves[moves_ptr++] = cell;
    return;
  }
  eats[eats_ptr++] = cell;
  bit_sign |= bit_value;
  return;
}
void check_black_rook_move(int file , int rank , int bit_value) {
  if (!x_y_on_board(file , rank)) {
    return;
  }
  int cell = get_cell_file_rank(file , rank);
  if (is_black(cell)) {
    bit_sign |= bit_value;
    return;
  }
  if (is_empty(cell)) {
    moves[moves_ptr++] = cell;
    return;
  }
  eats[eats_ptr++] = cell;
  bit_sign |= bit_value;
  return;
}
void calculate_white_rook_moves(int cell) {
  int file = cell / 8 , rank = cell % 8 , new_file, new_rank;
  for (int i = 1; i <= 7 && bit_sign != 15 ; i++) {
    if ((bit_sign & 1) == 0) {
      new_file = file + i, new_rank = rank  ;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 1;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_white_king_under_attack_vir()) {
          check_white_rook_move(new_file , new_rank , 1);
        } else {
          if (!is_empty(new_cell))
            bit_sign |= 1;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
    if ((bit_sign & 2) == 0) {
      new_file = file - i , new_rank = rank  ;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 2;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_white_king_under_attack_vir())
          check_white_rook_move(new_file , new_rank , 2);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 2;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
    if ((bit_sign & 4) == 0) {
      new_file = file  , new_rank = rank - i;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file, new_rank)) {
        bit_sign |= 4;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_white_king_under_attack_vir())
          check_white_rook_move(new_file , new_rank , 4);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 4;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
    if ((bit_sign & 8) == 0) {
      new_file = file , new_rank = rank + i;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 8;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_white_king_under_attack_vir())
          check_white_rook_move(new_file , new_rank , 8);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 8;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
  }
}
void calculate_black_rook_moves(int cell) {
  int file = cell / 8 , rank = cell % 8 , new_file, new_rank;
  for (int i = 1; i <= 7 && bit_sign != 15 ; i++) {
    if ((bit_sign & 1) == 0) {
      new_file = file + i, new_rank = rank  ;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 1;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_black_king_under_attack_vir())
          check_black_rook_move(new_file , new_rank , 1);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 1;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
    if ((bit_sign & 2) == 0) {
      new_file = file - i , new_rank = rank  ;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 2;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_black_king_under_attack_vir())
          check_black_rook_move(new_file , new_rank , 2);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 2;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
    if ((bit_sign & 4) == 0) {
      new_file = file  , new_rank = rank - i;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 4;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_black_king_under_attack_vir())
          check_black_rook_move(new_file , new_rank , 4);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 4;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
    if ((bit_sign & 8) == 0) {
      new_file = file , new_rank = rank + i;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 8;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_black_king_under_attack_vir())
          check_black_rook_move(new_file , new_rank , 8);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 8;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
  }
}
void check_white_bishop_move(int file , int rank , int bit_value) {
  if (!x_y_on_board(file , rank)) {
    bit_sign |= bit_value;
    return;
  }
  int cell = get_cell_file_rank(file , rank);
  if (is_white(cell)) {
    bit_sign |= bit_value;
    return ;
  }
  if (is_empty(cell)) {
    moves[moves_ptr++] = cell;
    return;
  }
  eats[eats_ptr++] = cell;
  bit_sign |= bit_value;
  return;
}
void check_black_bishop_move(int file , int rank , int bit_value) {
  if (!x_y_on_board(file , rank)) {
    bit_sign |= bit_value;
    return;
  }
  int cell = get_cell_file_rank(file , rank);
  if (is_black(cell)) {
    bit_sign |= bit_value;
    return ;
  }
  if (is_empty(cell)) {
    moves[moves_ptr++] = cell;
    return;
  }
  eats[eats_ptr++] = cell;
  bit_sign |= bit_value;
  return;
}
void calculate_white_bishop_moves(int cell) {
  int file = cell / 8 , rank = cell % 8 , new_file, new_rank;
  for (int i = 1; i <= 7 && bit_sign != 15 ; i++) {
    if ((bit_sign & 1) == 0) {
      new_file = file + i, new_rank = rank + i ;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 1;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_white_king_under_attack_vir())
          check_white_bishop_move(new_file , new_rank , 1);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 1;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
    if ((bit_sign & 2) == 0) {
      new_file = file - i , new_rank = rank + i ;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 2;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_white_king_under_attack_vir())
          check_white_bishop_move(new_file , new_rank , 2);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 2;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
    if ((bit_sign & 4) == 0) {
      new_file = file - i , new_rank = rank - i;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 4;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_white_king_under_attack_vir())
          check_white_bishop_move(new_file , new_rank , 4);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 4;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
    if ((bit_sign & 8) == 0) {
      new_file = file + i , new_rank = rank - i;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 8;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_white_king_under_attack_vir())
          check_white_bishop_move(new_file , new_rank , 8);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 8;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
  }
}
void calculate_black_bishop_moves(int cell) {
  int file = cell / 8 , rank = cell % 8 , new_file , new_rank;
  for (int i = 1; i <= 7 && bit_sign != 15 ; i++) {
    if ((bit_sign & 1) == 0) {
      new_file = file + i, new_rank = rank + i ;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 1;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_black_king_under_attack_vir())
          check_black_bishop_move(new_file , new_rank , 1);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 1;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
    if ((bit_sign & 2) == 0) {
      new_file = file - i , new_rank = rank + i ;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 2;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_black_king_under_attack_vir())
          check_black_bishop_move(new_file , new_rank , 2);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 2;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
    if ((bit_sign & 4) == 0) {
      new_file = file - i , new_rank = rank - i;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 4;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_black_king_under_attack_vir())
          check_black_bishop_move(new_file , new_rank , 4);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 4;
        }
        restore_tmp_board(cell , new_cell);
      }
    }
    if ((bit_sign & 8) == 0) {
      new_file = file + i , new_rank = rank - i;
      int new_cell = get_cell_file_rank(new_file, new_rank);
      if (!x_y_on_board(new_file , new_rank)) {
        bit_sign |= 8;
      } else {
        fix_tmp_board(cell , new_cell);
        if (!is_black_king_under_attack_vir())
          check_black_bishop_move(new_file , new_rank , 8);
        else {
          if (!is_empty(new_cell))
            bit_sign |= 8;
        }
      }
      restore_tmp_board(cell , new_cell);
    }
  }
}
void calculate_white_knight_moves(int cell) {
  int file = cell / 8, rank = cell % 8, new_rank = 0 , new_file = 0;
  for (int i = 0; i < 4 ; i++) {
    for (int j = 0; j < 4 ; j++) {
      if (abs(knight_xd[i]) == abs(knight_yd[j]))
        continue;
      new_rank = knight_xd[i] + rank;
      new_file = knight_yd[j] + file;
      if (x_y_on_board(new_rank , new_file)) {
        int new_cell = get_cell_file_rank(new_file , new_rank);
        fix_tmp_board(cell , new_cell);
        if (is_white_king_under_attack_vir()) {
          restore_tmp_board(cell , new_cell);
          continue;
        }
        restore_tmp_board(cell , new_cell);
        if (!is_white(new_cell)) {
          if (is_black(new_cell)) {
            eats[eats_ptr++] = new_cell;
          } else {
            moves[moves_ptr++] = new_cell;
          }
        }
      }
    }
  }
}
void calculate_black_knight_moves(int cell) {
  int file = cell / 8, rank = cell % 8, new_rank = 0 , new_file = 0;
  for (int i = 0; i < 4 ; i++) {
    for (int j = 0; j < 4 ; j++) {
      if (abs(knight_xd[i]) == abs(knight_yd[j]))
        continue;
      new_rank = knight_xd[i] + rank;
      new_file = knight_yd[j] + file;
      if (x_y_on_board(new_rank , new_file)) {
        int new_cell = get_cell_file_rank(new_file , new_rank);
        fix_tmp_board(cell , new_cell);
        if (is_black_king_under_attack_vir()) {
          restore_tmp_board(cell , new_cell);
          continue;
        }
        restore_tmp_board(cell , new_cell);
        if (!is_black(new_cell)) {
          if (is_white(new_cell)) {
            eats[eats_ptr++] = new_cell;
          } else {
            moves[moves_ptr++] = new_cell;
          }
        }
      }
    }
  }
}
void calculate_black_pawn_moves(int cell) {
  int file = cell / 8 , rank = cell % 8;
  if (rank >= 1) {
    int cell_down_left = get_cell_file_rank(file + 1 , rank - 1);
    fix_tmp_board(cell , cell_down_left);
    if (!is_black_king_under_attack_vir()) {
      if (is_white(cell_down_left)) {
        eats[eats_ptr++] = cell_down_left ;
      }
    }
    restore_tmp_board(cell , cell_down_left);
  }
  if (rank <= 6) {
    int cell_down_right = get_cell_file_rank(file + 1 , rank + 1);
    fix_tmp_board(cell , cell_down_right);
    if (!is_black_king_under_attack_vir()) {
      if (is_white(cell_down_right)) {
        eats[eats_ptr++] = cell_down_right;
      }
    }
    restore_tmp_board(cell , cell_down_right);

  }
  int cell_down = get_cell_file_rank(file + 1 , rank);
  fix_tmp_board(cell , cell_down);
  if (!is_black_king_under_attack_vir()) {
    if (is_empty(cell_down)) {
      moves[moves_ptr++] = cell_down;
    } else {
      restore_tmp_board(cell , cell_down);
      return;
    }
  }
  restore_tmp_board(cell , cell_down);
  if (file != 1)return;
  if (file + 2 <= 7) {
    cell_down = get_cell_file_rank(file + 2 , rank);
    fix_tmp_board(cell , cell_down);
    if (!is_black_king_under_attack_vir())
      if (is_empty(cell_down)) {
        moves[moves_ptr++] = cell_down;
      }
    restore_tmp_board(cell , cell_down);

  }
}
void calculate_white_pawn_moves(int cell) {
  int file = cell / 8, rank = cell % 8 ;
  if (rank >= 1) {
    int cell_up_left = get_cell_file_rank(file - 1 , rank - 1);
    fix_tmp_board(cell , cell_up_left);
    if (!is_white_king_under_attack_vir())
      if (is_black(cell_up_left)) {
        eats[eats_ptr++] = cell_up_left;
      }
    restore_tmp_board(cell , cell_up_left);

  }
  if (rank <= 6) {
    int cell_up_right = get_cell_file_rank(file - 1 , rank + 1);
    fix_tmp_board(cell , cell_up_right);
    if (!is_white_king_under_attack_vir())
      if (is_black(cell_up_right)) {
        eats[eats_ptr++] = cell_up_right;
      }
    restore_tmp_board(cell , cell_up_right);

  }
  int cell_up = get_cell_file_rank(file - 1 , rank);
  fix_tmp_board(cell , cell_up);
  if (!is_white_king_under_attack_vir()) {
    if (is_empty(cell_up)) {
      moves[moves_ptr++] = cell_up;
    } else {
      restore_tmp_board(cell , cell_up);
      return ;
    }
  }
  restore_tmp_board(cell , cell_up);
  if (file != 6)return;
  if (file - 2 >= 0) {
    cell_up = get_cell_file_rank(file - 2 , rank);
    fix_tmp_board(cell , cell_up);
    if (!is_white_king_under_attack_vir())
      if (is_empty(cell_up)) {
        moves[moves_ptr++] = cell_up;
      }
    restore_tmp_board(cell , cell_up);
  }
}
//////////////////////////////// game initialization /////////////////////////////////////////////////////////////

void init_board_places() {
  board[0] = BRO;
  board[1] = BKN;
  board[2] = BBI;
  board[3] = BQU;
  board[4] = BKI;
  board[5] = BBI;
  board[6] = BKN;
  board[7] = BRO;
  for (int i = 8 ; i <= 15 ; i++) {
    board[i] = BPA;
  }
  for (int i = 48 ; i <= 55 ; i++) {
    board[i] = WPA;
  }
  board[63] = WRO;
  board[62] = WKN;
  board[61] = WBI;
  board[60] = WKI;
  board[59] = WQU;
  board[58] = WBI;
  board[57] = WKN;
  board[56] = WRO;
  for (int i = 0; i < 64 ; i++)
    tmp_board[i] = board[i];
  moves_state_0to31 = moves_state_32to63 = 0;
}

void init_game() {
  print_new_game_quote();
  leds_off();
  test_positions();
  print_welcome_quote();
  leds_off();
}
void test_positions() {
  switches_values_0to31 = 0;
  switches_values_32to63 = 0;
  white_turn = 1;
  while (switches_values_0to31 != switches_0to31_StartingPos || switches_values_32to63 !=  switches_32to63_StartingPos) {
    read_shift_regs();
    switches_values_0to31 = getValue0to31();
    switches_values_32to63 = getValue32to63();
    set_sensors_equal_reeds_opening();
    delay(100);
  }
  old_ReedValues_0to31 = switches_0to31_StartingPos;
  old_ReedValues_32to63  = switches_32to63_StartingPos;
}
void set_sensors_equal_reeds_opening() {
  for (int i = 0; i < 16 ; i++) {
    if ((switches_values_0to31 >> i) & 1) {
      Single_Led(colour_green , i);
    } else {
      Single_Led(colour_red , i);
    }
  }
  for (int i = 48 ; i < 64 ; i++) {
    if ((switches_values_32to63 >> (i - 32)) & 1) {
      Single_Led(colour_green , i);
    } else {
      Single_Led(colour_red , i);
    }
  }
}
void print_welcome_quote() {
  print_w();
  delay(1000);
  print_e();
  delay(1000);
  print_l();
  delay(1000);
  print_c();
  delay(1000);
  print_o();
  delay(1000);
  print_m();
  delay(1000);
  print_e();
  delay(1000);
}
void print_new_game_quote() {
  print_n();
  delay(1000);
  print_e();
  delay(1000);
  print_w();
  delay(2000);
  print_g();
  delay(1000);
  print_a();
  delay(1000);
  print_m();
  delay(1000);
  print_e();
  delay(2000);
}
/////////////////////////////// reed switches functions ///////////////////////////////////////////////////////////
void initPins()
{
  // Initialize our digital pins...
  pinMode(LOAD_PIN, OUTPUT);
  pinMode(CLOCK_ENABLE_BIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(_DATA_PIN_0TO31, INPUT);
  pinMode(_DATA_PIN_32TO63, INPUT);
  digitalWrite(CLOCK_PIN, LOW);
  digitalWrite(LOAD_PIN, HIGH);
}
void init_values(){
  _bit_values_from_0to31 = 0;
  _bit_values_from_32to63 = 0;

}
void init_load_pins(){
  digitalWrite(CLOCK_ENABLE_BIN, HIGH);
  digitalWrite(LOAD_PIN, LOW);
  delayMicroseconds(PULSE);
  digitalWrite(LOAD_PIN, HIGH);
  digitalWrite(CLOCK_ENABLE_BIN, LOW);
}
void read_shift_regs()
{
  init_values();
  init_load_pins();
  _32_BIT tmp_input_values0to31 = 0;
  _32_BIT tmp_input_values32to63 = 0; 
  for (int i = 0; i < DATA_N; i++)
  {
    _bit_values_from_0to31 = digitalRead(_DATA_PIN_0TO31);
    _bit_values_from_32to63 = digitalRead(_DATA_PIN_32TO63);

    tmp_input_values0to31 |= (_bit_values_from_0to31 << ((DATA_N - 1) - i));
    tmp_input_values32to63 |= (_bit_values_from_32to63 << ((DATA_N - 1) - i));

    digitalWrite(CLOCK_PIN, HIGH);
    delayMicroseconds(PULSE);
    digitalWrite(CLOCK_PIN, LOW);
  }

  input_values0to31 = tmp_input_values0to31;
  input_values32to63 = tmp_input_values32to63;
}

_32_BIT getValue0to31() {

  return input_values0to31;
}
_32_BIT getValue32to63() {

  return input_values32to63;
}

/////////////////////////////// here is some leds functions ////////////////////////////////////////////////////////
// this one for controlling one led
void Single_Led(uint32_t colour, int ledNumber)
{
  strip.setPixelColor(ledNumber, colour);
  strip.show();
}

void print_a() {
  for (int i = 0; i < 64 ; i++) {
    if ((i > 1 && i < 6) || (i == 17) || (i == 25) || (i >= 33 && i <= 38) || (i == 41
                                                                              ) || (i == 49) || (i == 22 ) || (i == 30) || (i == 46) || (i == 54) || i == 9 || i == 14 )
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);

  }
}
void print_b() {
  for (int i = 0; i < 64 ; i++) {
    if ((i >= 1 && i <= 5) || (i == 14) || i == 22 || (i >= 25 && i <= 30)
        || ( i >= 33 && i <= 37) || i == 46 || i == 54 || (i >= 57 && i <= 61)
        || i == 9 || i == 17 || i == 25 || i == 33 || i == 41 || i == 49)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_c() {
  for (int i = 0; i < 64 ; i++) {
    if ((i > 9 && i < 14) || (i == 17) || (i == 25) || (i == 33)
        || (i == 41) || (i > 49 && i < 54) || i == 22 || i == 46)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);

  }
}

void print_d() {
  for (int i = 0; i < 64 ; i++) {
    if ((i >= 1 && i <= 4) || (i == 13) || (i == 22) || (i == 30)
        || (i == 38) || (i == 46) || (i == 53) || (i >= 57 && i <= 60)
        || (i == 9) || (i == 17) || (i == 25) || (i == 33)
        || (i == 41) || (i == 49))
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_e() {
  for (int i = 0; i < 64 ; i++) {
    if ((i >= 1 && i <= 6) || (i == 9) || (i == 17) || (i == 33)
        || (i == 41)  || (i >= 25 && i <= 30) || (i >= 49 && i <= 54))
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_f() {
  for (int i = 0; i < 64 ; i++) {
    if ((i >= 1 && i <= 6) || (i == 9) || (i == 17) || (i == 33)
        || (i == 41) || (i == 49) || (i == 57)  || (i >= 25 && i <= 30) )
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_g() {
  for (int i = 0; i < 64 ; i++) {
    if ((i >= 37 && i <= 39) || (i > 9 && i < 14) || (i == 17) || (i == 25)
        || (i == 33) || (i == 41) || (i > 49 && i < 54) || i == 22 || i == 46)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);

  }
}
void print_h() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 57 || i == 62 || (i ==  1) || ( i == 6) || (i == 17) || (i == 25)
        || (i >= 33 && i <= 38) || (i == 41) || (i == 49) || (i == 22 ) || (i == 30)
        || (i == 46) || (i == 54) || i == 9 || i == 14 )
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);

  }
}
void print_i() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 11 || i == 12 || i == 27 || i == 28 || i == 3 || i == 4 || i == 35
        || i == 36 || i == 43 || i == 44 || i == 51 || i == 52 || i == 59 || i == 60)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_j() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 19 || i == 20 || i == 27 || i == 28 || i == 3 || i == 4 || i == 35
        || i == 36 || i == 43 || i == 44 || i == 51 || i == 41 || i == 49 || i == 50
        || i == 52 || i == 58 || i == 59 || i == 40)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_k() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 1 || i == 9 || i == 17 || i == 25 || i == 33 || i == 41
        || i == 49 || i == 57 || i == 18 || i == 11 || i == 4 || i == 42 || i == 51 || i == 60)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_l() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 1 || i == 9 || i == 17 || i == 18 || i == 25 || i == 33 || i == 41
        || i == 49 || i == 57 || i == 2 || i == 10 || i == 17 || i == 26 || i == 34 || i == 42 || i == 50 || i == 58
        || i == 51 || i == 59 || i == 52 || i == 60 || i == 61 || i == 53 || i == 54 || i == 62)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_m() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 16 || i == 24 || i == 32 || i == 40 || i == 48 || i == 9 || i == 10
        || i == 19 || i == 20 || i == 27 || i == 28 || i == 35 || i == 36 || i == 43 || i == 44
        || i == 51 || i == 52 || i == 13 || i == 14 || i == 23 || i == 31 || i == 39 || i == 47 || i == 55)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_n() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 8 || i == 9 || i == 16 || i == 17 || i == 24 || i == 25 || i == 32
        || i == 33 || i == 40 || i == 41 || i == 48 || i == 49
        || i == 18 || i == 27 || i == 36 || i == 45 || i == 54 || i == 55 || i == 46
        || i == 47 || i == 38 || i == 39 || i == 30 || i == 31 || i == 22 || i == 23 || i == 14 || i == 15)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_o() {
  for (int i = 0; i < 64 ; i++) {
    if ((i > 9 && i < 14) || (i == 17) || (i == 25) || (i == 33) || (i == 41)
        || (i > 49 && i < 54) || i == 22 || i == 46 || i == 38 || i == 30)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);

  }
}
void print_p() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 57 || i == 58 || i == 49 || i == 50 || i == 41 || i == 42 || i == 33
        || i == 34 || i == 25 || i == 26 || i == 17 || i == 18 || i == 9 || i == 10 || i == 2
        || i == 3 || i == 4 || i == 11 || i == 12 || i == 13 || i == 21 || i == 28 || i == 27 || i == 26)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);

  }
}
void print_q() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 45 || i == 54 || i == 63 || (i > 9 && i < 14) || (i == 17) || (i == 25)
        || (i == 33) || (i == 41) || (i > 49 && i < 54) || i == 22 || i == 46 || i == 38 || i == 30)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);

  }
}
void print_r() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 57 || i == 58 || i == 49 || i == 50 || i == 41 || i == 42 || i == 33
        || i == 34 || i == 25 || i == 26 || i == 17 || i == 18 || i == 9 || i == 10 || i == 2
        || i == 3 || i == 4 || i == 11 || i == 12 || i == 13 || i == 21 || i == 28
        || i == 27 || i == 26 || i == 35 || i == 36 || i == 44 || i == 45 || i == 52 || i == 53 || i == 60 || i == 61 )
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);

  }
}
void print_s() {
  for (int i = 0; i < 64 ; i++) {
    if ((i >= 9 && i < 14) || (i == 17) || (i == 25) || (i > 33 && i < 38)
        || (i == 46) || (i == 54) || (i >= 57 && i < 62) || (i == 22) || (i == 49))
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}

void print_t() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 11 || i == 12 || i == 27 || i == 28 || i == 3 || i == 4 || i == 35
        || i == 36 || i == 43 || i == 44 || i == 51 || i == 52 || i == 59 || i == 60 || i == 19 || i == 20
        || i == 1 || i == 2 || i == 9 || i == 10  || i == 5 || i == 6 || i == 13 || i == 14)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_u() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 8 || i == 9 || i == 16 || i == 17 || i == 24 || i == 25 || i == 32
        || i == 33 || i == 41 || i == 40 || i == 49 || i == 50 || i == 58 || i == 51 || i == 59
        || i == 52 || i == 60 || i == 53 || i == 61 || i == 54 || i == 55 || i == 46
        || i == 47 || i == 38 || i == 39 || i == 30 || i == 31 || i == 22 || i == 23 || i == 14 || i == 15)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_v() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 51 || i == 52 || i == 42 || i == 33 || i == 24 || i == 43 || i == 34 || i == 25
        || i == 16 || i == 44 || i == 37 || i == 30 || i == 23 || i == 31 || i == 38 || i == 45)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_w() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 16 || i == 24 || i == 32 || i == 40  || i == 49 || i == 50 || i == 19
        || i == 20 || i == 27 || i == 28 || i == 35 || i == 36 || i == 43 || i == 44
        || i == 53 || i == 54 || i == 23 || i == 31 || i == 39 || i == 47)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_x() {
  for (int i = 0; i < 64 ; i++) {
    if (i % 9 == 0 || i % 7 == 0)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_y() {
  for (int i = 0; i < 64 ; i++) {
    if (((i % 9 == 0 || i % 7 == 0) && i <= 39) || i == 59 || i == 60 || i == 51
        || i == 52 || i == 43 || i == 44 || i == 8 || i == 17 || i == 26 || i == 15 || i == 22 || i == 29)
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_z() {
  for (int i = 0; i < 64 ; i++) {
    if ((i % 7 == 0) || ((i - 1) % 7 == 0 && i > 1) || i <= 15 || (i >= 48 && i <= 63))
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
/////////////////////// addational functions ////////////////////////////////////////

bool is_black(int index) {
  return board[index] >= 7 && board[index] <= 12;
}

bool is_white(int index) {
  return board[index] >= 1 && board[index] <= 6;
}
bool is_empty(int cell_number) {
  return board[cell_number] == 0;
}
bool x_y_on_board(int rank , int file) {
  return file >= 0 && file <= 7 && rank >= 0 && rank <= 7;
}


int get_changed_cells() {
  //old_ReedValues_0to31 old_ReedValues_32to63
  // read sensors
  // one piece old_reed_values = 1 .... new read = 0
  // else return -1
  read_shift_regs();
  switches_values_0to31 = getValue0to31();
  switches_values_32to63 = getValue32to63();
  int cnt = 0 , ret = -1;
  for (int i = 0; i < 32 ; i++) {
    if (((old_ReedValues_0to31 >> i) & 1) - ((switches_values_0to31 >> i) & 1) ==  1) {
      ++cnt;
      ret = i ;
    }
    if (((old_ReedValues_32to63 >> i) & 1) - ((switches_values_32to63 >> i) & 1) == 1) {
      ++cnt;
      ret = i + 32;
    }
  }
  if (cnt == 1)return ret;
  else return -1;
}
int get_cell_file_rank(int file , int rank) {
  return file * 8 + rank;
}
void print_rook() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 9 || i == 11 || i == 12 || i == 14 || (i >= 17 && i <= 22) || (i >= 25 && i <= 30) || (i >= 33 && i <= 38) || ( i >= 42 && i <= 45) || (i >= 49 && i <= 54))
      Single_Led(colour_in , i);
    else Single_Led(colour_out , i);
  }
}
void print_queen() {
  for (int i = 0; i < 64 ; i++) {
    if ((i >= 57 && i <= 62) || (i >= 50 && i <= 53) || (i >= 43 && i <= 44) || i == 35 || i == 36 || (i >= 25 && i <= 30) || (i >= 19 && i <= 20) || i == 10 || i == 3 || i == 4 || i == 13 || i == 17 || i == 22
        || i == 23 || i == 16 || i == 24 || i == 31) {
      Single_Led(colour_in , i);
    } else {
      Single_Led(colour_out , i);
    }
  }
  Single_Led(colour_off , 11);
  Single_Led(colour_off , 12);
}
void print_knight() {
  for (int i = 0; i < 64 ; i++) {
    if (i == 3 || i == 4 || i == 11 || i == 12 || i == 10 || i == 13 || i == 14 || (i >= 17 && i < 22 ) || (i >= 24 && i <= 30) || i == 33 || i == 34 || i == 36 || i == 37 ||
        i == 44 || i == 45 || i == 46 || i == 54 || i == 53 || i == 51 || i == 59 || i == 58  || i == 53 || i == 54 || i == 50 || i == 53 || i == 60 || i == 52) {
      Single_Led(colour_in , i);
    } else {
      Single_Led(colour_out , i);
    }
  }
  Single_Led(colour_yellow , 19);
}
void print_bishop() {
  for (int i = 0 ; i < 64 ; i++) {
    if ((i >= 2 && i <= 5) || (i >= 9 && i <= 13) || (i >= 16 && i <= 20) || (i >= 24 && i <= 27) || (i >= 30 && i <= 47) || (i >= 49 && i <= 54) || (i >= 56 && i <= 63)) {
      Single_Led(colour_in , i);
    } else if (i == 14 || i == 21 || i == 22 || i == 28 || i == 29) {
      Single_Led(colour_yellow , i);
    }
    else {
      Single_Led(colour_out , i);
    }
  }
}
