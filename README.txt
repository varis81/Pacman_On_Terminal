The game consists of three files, pacman.c, set?ticket.c and pac.h . Two playgrounds|
are given for pacman to run!Floyd Warshall Algorithm is used for Ghost movement.    |
Check attached lecture to understand how it is working!				    | 	
-------------------------------------------------------------------------------------


-pacman.c

1)main
2)void set_up();     //set up function to prepear screen & initialize positions//
3)void Move();       //function that calls pac_move and ghost_move according to period//
4)int pac_move();
5)int ghost_move(int);  //move i-ghost//
6)void next_level(int); //change level,without bonus if it's user's command//
7)void reset(int);      //reset for next level,or when pacman is killed//
8)void Floyd_Warshall(int **W,int dots,int **d);
9)void find(int i,int j,int col,int *pos ,int A[]);

Letter mapping:
C:  Pacman
G:  Ghosts
P:  Life Pills
W:  Walls
c:  Ghost House

Controls:
The usual, W,A,S,D	