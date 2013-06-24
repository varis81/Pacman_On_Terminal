#define MAX_LINES           60
#define MAX_COL             60
#define BLANK               ' ' 
#define PAC_SYMBOL          'C'
#define GHOST_SYMBOL        'G'
//#define PIL_SYMBOL          '@'
#define PIL_SYMBOL          'P'
#define MAX_PIL_NUMBER       4   // megistos ariQmos xapiwn pou upostirizei i pista
#define MAX_GHOST_NUMBER     4  //megistos ariQmos fantasmatwn pou upostirizei i pista
#define DOOR_SYMBOL         'w'
#define FOOD_SYMBOL         '.'
//#define FOOD_SYMBOL         '-'
#define POWER_PILL_DURATION  5
#define LIFES                3

struct Ghost {
  int x ,x_initial,y_initial, y , y_dir, x_dir,inside,inside_count;
  char temp;
};

struct Packman{
  int x, x_initial, y ,y_initial, y_dir ,lifes, x_dir,dot_count,points,power;
};

struct Pil{
  int x , y;
};

