#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <curses.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include "pac.h"

static volatile sig_atomic_t Doneflag=0;
static void setdoneflag(int signo){endwin();initscr();clear();cbreak();Doneflag=1;}

char D[MAX_LINES][MAX_COL], CONST[MAX_LINES][MAX_COL];
int lines=0,w_x,w_y,col=-1,fd,dots=0,G_count=0,P_count=0,**W,*A,**d,ghosts_period,level=1,pac_period,period_decrease,beggin=0,others=0;
struct Ghost ghosts[MAX_GHOST_NUMBER];
struct Packman packman;
struct Pil pil[MAX_PIL_NUMBER];

void set_up();     //set up function to prepear screen & initialize positions//
void Move();       //function that calls pac_move and ghost_move according to period//
int pac_move();
int ghost_move(int);  //move i-ghost//
void next_level(int); //change level,without bonus if it's user's command//
void reset(int);      //reset for next level,or when pacman is killed//
void Floyd_Warshall(int **W,int dots,int **d);
void find(int i,int j,int col,int *pos ,int A[]);

int main(int argc, char *argv[])
{
  struct sigaction act1,act2;
  int flag=0,i=0,j,bytesread;
  char ch;

  act1.sa_handler=Move;     //handler for Move,whick moves Ghosts and pacman//
  act1.sa_flags=0;
  if( (sigemptyset(&act1.sa_mask ) == -1) || (sigaction(SIGALRM,&act1,NULL) == -1 ) ){
    perror("FAIL TO INSTALL SIGALARM SIGNAL HANDLER");
    return 1;}

  act2.sa_handler=setdoneflag; //handler for ctrl-C,ctrl-Z//
  act2.sa_flags=0;
  if( (sigemptyset(&act2.sa_mask)==-1) || (sigaction(SIGINT,&act2,NULL)==-1) || (sigaction(SIGTSTP,&act2,NULL)==-1) ){
     perror("FAIL TO INSTALL SIGINT 'N' SIGTSTP SIGNAL HANDLER");
     return 1;}

  if(argc!=4){
    printf("FOUR ARGUMENTS ARE NEEDED\n");
    return 1;}
  else{
    if( (pac_period=atoi(argv[2]))<1){
       printf("DEN UPORSTIRIZETAI TOSO MIKRI PERIODOS\n");
       exit(0);} 
    ghosts_period=atoi(argv[2])+10;  //ghosts period initially higher//
    period_decrease=atoi(argv[3]);} //for ghosts' period//

  if((fd=open(argv[1],O_RDONLY))==-1 ){
    perror("Failed to open input file");
    return 1;}

  while( (read(fd,&ch,sizeof(char))>0) && (ch!=EOF) ){//find no  of collums and lines
    if(flag!=1)col++;
    if(ch=='\n') {lines++;flag=1;}
  }
  set_up();
  while(bytesread=read(0,&ch,sizeof(char))){   
      if( (bytesread==-1)&& (errno==EINTR)){
         if      ( Doneflag  || !packman.lifes ){endwin();clear();cbreak();return 1; }
         else if ( ch == 's'                   ){
                 packman.x_dir=1 ;
                 beggin=1;}
         else if ( ch == 'j'&& beggin ){ packman.x_dir=-1; packman.y_dir=0;    }
         else if ( ch == 'l'&& beggin ){ packman.x_dir=+1; packman.y_dir=0;    }
         else if ( ch == 'i'&& beggin ){ packman.y_dir=-1; packman.x_dir=0;    }
         else if ( ch == 'k'&& beggin ){ packman.y_dir=+1; packman.x_dir=0;    }
         else if ( ch == 'N'&& beggin ){ next_level(0);                        }
         else if ( ch == 'Q'          ){ cbreak(); endwin(); clear(); return 1;}     
      }
   }
}


void set_up()
{
  char ch;
  int i,j,k=0,pos,pos1;
  initscr();
  cbreak();
  lseek(fd,0L,0);
  for(i=0;i<(lines);i++)
    for(j=0;j<(col+1);j++){
      read(fd,&ch,sizeof(char));
      CONST[i][j]=D[i][j]=ch;   //a constant matrix and one for the changes//
      mvaddch(i,j,D[i][j]);
      if ( ch == PAC_SYMBOL ){         //find positions so as to work for every ground//
         packman.x=packman.x_initial=j;
         packman.y=packman.y_initial=i;dots++;others++;}
      else if ( ch == PIL_SYMBOL ) {
         pil[P_count].x=j;
         pil[P_count++].y=i;dots++;others++;}
      else if(ch == GHOST_SYMBOL){
         ghosts[G_count].x_initial=ghosts[G_count].x=j;
         ghosts[G_count].y_initial=ghosts[G_count++].y=i;}
      else if ( ch == FOOD_SYMBOL    )
          dots++;
      else if ( ch == DOOR_SYMBOL ){
          w_x=j;w_y=i;}
    }
 close(fd); 
 if( !G_count)//if there are no ghosts place four inside home//
    for(i=0;i<MAX_GHOST_NUMBER;i++){
        mvaddch(w_y+1,w_x+1+i, CONST[w_y+1][w_x+1+i]=D[w_y+1][w_x+1+i]= GHOST_SYMBOL);
        ghosts[G_count].x=ghosts[i].x_initial=w_x+1+i;
        ghosts[G_count++].y=ghosts[i].y_initial=w_y+1;
    }
  W=malloc(dots*sizeof(int*));
  for(i=0;i<dots;i++)
     W[i]=malloc(dots*sizeof(int));
  A=malloc(dots*sizeof(int*));
  for(i=0;i<lines;i++)
    for(j=0;j<col;j++)
      if(D[i][j] == FOOD_SYMBOL || D[i][j]==PAC_SYMBOL || D[i][j]==PIL_SYMBOL){
         A[k]=i*col+j;
         k++;}
  for(i=0;i<dots;i++)
    for(j=0;j<dots;j++)
      if(i==j) W[i][j]=0;
      else W[i][j]=7000;
  for(i=0;i<lines;i++)
    for(j=0;j<col;j++)
     if( D[i][j] == FOOD_SYMBOL && i==lines-1){
        find(i,j,col,&pos1,A);
        find(0,j,col,&pos,A);
        W[pos][pos1]=1; W[pos1][pos]=1;}
      else if( D[i][j] == FOOD_SYMBOL && j==col-1){
         find(i,j,col,&pos1,A);  
         find(i,0,col,&pos,A);   
         W[pos][pos1]=1; W[pos1][pos]=1;}
      else  if( D[i][j] == FOOD_SYMBOL ||D[i][j]==PAC_SYMBOL || D[i][j]==PIL_SYMBOL) {
            if( D[i+1][j]==FOOD_SYMBOL ||D[i+1][j]==PAC_SYMBOL|| D[i+1][j]==PIL_SYMBOL){
               find(i+1,j,col,&pos,A);
               find(i,j,col,&pos1,A);
               W[pos][pos1]=1; W[pos1][pos]=1;}
            if( D[i-1][j]==FOOD_SYMBOL ||D[i-1][j]==PAC_SYMBOL ||D[i-1][j]==PIL_SYMBOL){
               find(i-1,j,col,&pos,A);
               find(i,j,col,&pos1,A);
               W[pos][pos1]=1; W[pos1][pos]=1;}
            if( D[i][j-1]==FOOD_SYMBOL ||D[i][j-1]==PAC_SYMBOL ||D[i][j-1]==PIL_SYMBOL ){
               find(i,j-1,col,&pos,A);
               find(i,j,col,&pos1,A);
               W[pos][pos1]=1; W[pos1][pos]=1;}
            if( D[i][j+1]==FOOD_SYMBOL ||D[i][j+1]==PAC_SYMBOL  || D[i][j+1]==PIL_SYMBOL){
               find(i,j+1,col,&pos,A);
               find(i,j,col,&pos1,A);
               W[pos][pos1]=1; W[pos1][pos]=1;}
         }
   d=malloc(dots*sizeof(int*));
   for(i=0;i<dots;i++)
      d[i]=malloc(dots*sizeof(int));
   for(i=0;i<dots;i++)
     for(j=0;j<dots;j++)
       d[i][j]=0;
   Floyd_Warshall(W,dots,d);
   if(has_colors() == FALSE){      //initializing for colours//
     endwin();
     printf("Your terminal does not support color\n");
     exit(1);}
   start_color();
  refresh();
  for(i=0;i<G_count;i++){
      ghosts[i].y_dir=ghosts[i].x_dir=ghosts[i].inside_count=0;
      ghosts[i].inside=1;
      ghosts[i].temp=BLANK;}
  packman.y_dir =packman.x_dir = packman.power=packman.dot_count=packman.points=0;       //initializing//
  packman.lifes=LIFES;
  noecho();
  crmode();
  set_ticker();
}

void Move()
{
 if( beggin){
    int i,Points[]={300,600,900,3200};
    static long int temp=0,ticks2=0,ticks=0,j=0;
    ticks=++ticks%10000;       //to avoid overflow//
    if( !(ticks%25))                          //each second    //
       for(i=0;i<G_count; i++){              //for one ghost  //
          if(ghosts[i].inside){             //whick is inside//
             ghosts[i].inside_count+=1;    //extract it,if it has stayed in 3 to 15 seconds//
             if( (ghosts[i].inside_count/(rand()%12+3)>=1) && D[w_y-1][w_x]!=GHOST_SYMBOL){
                standout();
                mvaddch(  w_y-1, w_x, GHOST_SYMBOL );
                standend();
                ghosts[i].temp=D[w_y-1][w_x];
                D[w_y-1][w_x]=GHOST_SYMBOL;
                mvaddch(   ghosts[i].y_initial,ghosts[i].x_initial, BLANK );
                D[ghosts[i].y_initial][ghosts[i].x_initial]=BLANK;
                ghosts[i].x=w_x;
                ghosts[i].y=w_y-1;
                ghosts[i].x_dir=ghosts[i].y_dir=ghosts[i].inside=ghosts[i].inside_count=0;
                break;}
          }
       }
    if( (ticks%pac_period)==0 && (i=pac_move())!=100 )  //move pacman according to period//
       {sleep(5);                                      //if pacman meet ghost           //
        if( packman.power){                           //<--state of power!!!           //
           standout();
           mvaddch(packman.y,packman.x,PAC_SYMBOL);
           standend();
           if(D[ghosts[i].y][ghosts[i].x]==FOOD_SYMBOL)packman.dot_count++;//an to fantasma einai panw se teleia,fae kai tin telia//
           mvaddch(ghosts[i].y=ghosts[i].y_initial,ghosts[i].x=ghosts[i].x_initial,GHOST_SYMBOL); 
           packman.points+=Points[++j%4];
           ghosts[i].inside=1;}
        else{                          //not state of power               //
           packman.lifes--;
           reset(2);}
       }
    if( (ticks%ghosts_period)==0)                   //according to period                //
       for(i=0;i<G_count;i++)                      //move all ghosts that are outside   //
         if( !ghosts[i].inside)
            if( !ghost_move(i) ){                 //if ghost meet pacman              //
               sleep(5);                         
               if( packman.power){              //<--state of power!!!              //
                   standout();
                   mvaddch(packman.y,packman.x,PAC_SYMBOL);
                   standend();
                   mvaddch(ghosts[i].y=ghosts[i].y_initial,ghosts[i].x=ghosts[i].x_initial,GHOST_SYMBOL);                 
                   packman.points+=Points[++j%4];
                   ghosts[i].inside=1;}
               else{                          //not state of power               //
                   packman.lifes--;
                   reset(2);}
            }
   char* str1=(char *)malloc(sizeof(char)); //timer//
   if( !(ticks%25)&& (packman.power)){     //if pacman is in power state//
      temp=++temp%(POWER_PILL_DURATION+1);//POWER_PILL duration DEFINE      //
      if( !temp) 
         packman.power=j=0;
      sprintf(str1,"%d",temp);
      mvaddstr( lines+2  ,0 , "sec:" );
      addstr(str1);
      free(str1);}
   if     ( level==1) mvaddstr(lines+3,0,"YOU ARE BEGINNER!");
   else if( level==2) mvaddstr(lines+3,0,"YOU SHOULD TRY HARDER!");
   else if( level==3) mvaddstr(lines+3,0,"YOU MUST BE AFRAID OF ME!");
   else               mvaddstr(lines+3,0,"I WILL KILL YOU SOON....!");
   move(LINES-1,COLS-1);
   refresh();
 }
}

int pac_move()
{
 int  y_cur= packman.y, x_cur=packman.x , moved=1,i; //current possition//

 packman.y +=packman.y_dir ;              //next position//
 packman.x +=packman.x_dir;
 if ( packman.dot_count == (dots-others) && P_count==0){  //moving to next level??//
     moved=0; 
     next_level(1);}
 if ( D[packman.y][packman.x]==PIL_SYMBOL){       //power pills//
    packman.power=1;
    packman.points+=60; P_count--;
    for(i=0;i<G_count;i++){                //change ghosts' direction immediately//
       ghosts[i].x_dir=-1*(ghosts[i].x_dir);///edw kati pio kalo Qelw
       ghosts[i].y_dir=-1*(ghosts[i].y_dir);}
 }
 if ( D[packman.y][packman.x]=='W' || D[packman.y][packman.x]==DOOR_SYMBOL){ //stop if yoou have to//
     moved=0;
     packman.y -= packman.y_dir ;
     packman.x -= packman.x_dir ;}
 
 if ( packman.x == col    )  packman.x=0;          //appear to the other //
 else if(packman.x==-1    )  packman.x=col-1;     // side if it's needed//
 else if(packman.y==lines )  packman.y=0;
 else if(packman.y==-1     )  packman.y=lines-1;

 if ( moved ) {                              //move packman and      //
    if ( D[ packman.y][ packman.x]==FOOD_SYMBOL){     //points for each dot eaten//
         packman.dot_count++;
         packman.points+=10;}
    mvaddch( y_cur, x_cur, BLANK );        //refresh to see changes//
    D[y_cur][x_cur]=BLANK;
    standout();
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    attron(COLOR_PAIR(2));
    mvaddch( packman.y, packman.x, PAC_SYMBOL );
    standend();
    attroff(COLOR_PAIR(2));
    D[packman.y][ packman.x]=PAC_SYMBOL;
    char* str1=(char *)malloc(sizeof(char));
    char* str2=(char *)malloc(sizeof(char));
    char* str3=(char *)malloc(sizeof(char));
    char* str4=(char *)malloc(sizeof(char));
    char* str5=(char *)malloc(sizeof(char));
    char* str6=(char *)malloc(sizeof(char));
    sprintf(str1,"%d",packman.points);
    sprintf(str2,"%d",packman.lifes);
    sprintf(str3,"%d",dots);///Qa fugei///
    sprintf(str4,"%d",packman.dot_count);
    sprintf(str5,"%d",level);
    sprintf(str6,"%d-%d",pac_period,ghosts_period);
    mvaddstr( lines  ,0 , "Points:" ); addstr(str1);
    mvaddstr( lines  ,33 , "Lifes:" );  addstr(str2);
    mvaddstr( lines+1  ,33 , "Level:" );   addstr(str5);
    mvaddstr( lines+1 ,0 , "food eaten:" );  addstr(str4);
    mvaddstr( lines+1 ,17 , "Period:" ); addstr(str6);
    free(str1); free(str2); free(str3); free(str4); free(str5); free(str6);
    }
    for(i=0;i<G_count;i++)//if you meet ghost return its number,else return 100//
      if( ghosts[i].x==packman.x && ghosts[i].y==packman.y){
         if( ghosts[i].temp==FOOD_SYMBOL)
             packman.dot_count++;////
         return i;}
    return 100;  
}

int ghost_move(int i)
{
  int  b,a,a1,a2,a3,a4,thesi=0,min=7000,x_next,y_next,j,k,y_cur=ghosts[i].y,ydc=ghosts[i].y_dir,xdc=ghosts[i].x_dir,x_cur=ghosts[i].x;
  static int l=0,m=0;
  D[y_cur][x_cur]=ghosts[i].temp;
  find(y_cur,x_cur,col,&a,A);
  find(packman.y,packman.x,col,&b,A);
  find(y_cur+1,x_cur,col,&a1,A);   find(y_cur-1,x_cur,col,&a2,A);
  find(y_cur,x_cur+1,col,&a3,A);  find(y_cur,x_cur-1,col,&a4,A);
  if(a1!=-1) if(d[a][b]-d[a1][b]==1){y_next=A[a1]/col; x_next=A[a1]%col;thesi=1; }
  if(a2!=-1) if(d[a][b]-d[a2][b]==1){y_next=A[a2]/col; x_next=A[a2]%col;thesi=2; }
  if(a3!=-1) if(d[a][b]-d[a3][b]==1){y_next=A[a3]/col; x_next=A[a3]%col;thesi=3; }
  if(a4!=-1) if(d[a][b]-d[a4][b]==1){y_next=A[a4]/col; x_next=A[a4]%col;thesi=4; }

  ghosts[i].x_dir=x_next-x_cur;
  ghosts[i].y_dir=y_next-y_cur;

   //after third lever let ghosts change their direction in any case//
  if(ydc + ghosts[i].y_dir==0 && xdc + ghosts[i].x_dir==0 &&level<4){
     if(thesi!=1)  if(a1!=-1) if(d[a][b]-d[a1][b]<min){y_next=A[a1]/col; x_next=A[a1]%col;min=d[a][b]-d[a1][b];}
     if(thesi!=2)  if(a2!=-1) if(d[a][b]-d[a2][b]<min){y_next=A[a2]/col; x_next=A[a2]%col;min=d[a][b]-d[a2][b];}
     if(thesi!=3)  if(a3!=-1) if(d[a][b]-d[a3][b]<min){y_next=A[a3]/col; x_next=A[a3]%col;min=d[a][b]-d[a3][b];}
     if(thesi!=4)  if(a4!=-1) if(d[a][b]-d[a4][b]<min){y_next=A[a4]/col; x_next=A[a4]%col;min=d[a][b]-d[a4][b];}
     ghosts[i].x_dir=x_next-x_cur;
     ghosts[i].y_dir=y_next-y_cur;
   }
   if( D[(ghosts[i].y)+ghosts[i].y_dir][(ghosts[i].x)+ghosts[i].x_dir]==GHOST_SYMBOL ||D[(ghosts[i].y)+ghosts[i].y_dir][(ghosts[i].x)+ghosts[i].x_dir]=='W'){
      ghosts[i].x_dir=-1*ghosts[i].x_dir;
      ghosts[i].y_dir=-1*ghosts[i].y_dir;}


  if( D[(ghosts[i].y)+ghosts[i].y_dir][(ghosts[i].x)+ghosts[i].x_dir]==GHOST_SYMBOL ||D[(ghosts[i].y)+ghosts[i].y_dir][(ghosts[i].x)+ghosts[i].x_dir]=='W'){
     ghosts[i].x_dir=-1*ghosts[i].x_dir;
     ghosts[i].y_dir=-1*ghosts[i].y_dir;}
  else{
     ghosts[i].x += ghosts[i].x_dir;
     ghosts[i].y += ghosts[i].y_dir;}

  if     (ghosts[i].x == col-1 )  ghosts[i].x=0;          //appear to the other //
  else if(ghosts[i].x==0       )  ghosts[i].x=col-1;     // side if it's needed//
  else if(ghosts[i].y==lines-1 )  ghosts[i].y=0;
  else if(ghosts[i].y==0       )  ghosts[i].y=lines-1;


   if(ghosts[i].y==packman.y && ghosts[i].x==packman.x){
      ghosts[i].temp= D[ghosts[i].y][ghosts[i].x];     //temp is used because ghost's don't change//
      standout();                                     //the values of ground while moving        //
      mvaddch(  ghosts[i].y,  ghosts[i].x, GHOST_SYMBOL );
      standend();
      D[ghosts[i].y][ghosts[i].x]=GHOST_SYMBOL;
      mvaddch( y_cur, x_cur,  D[y_cur][x_cur] );
      return 0;
   }
   ghosts[i].temp= D[ghosts[i].y][ghosts[i].x];     //temp is used because ghost's don't change//
   standout();                                     //the values of ground while moving        //
   init_pair(3, COLOR_RED, COLOR_BLACK);
   attron(COLOR_PAIR(3));
   mvaddch(  ghosts[i].y,  ghosts[i].x, GHOST_SYMBOL );
   attroff(COLOR_PAIR(3));
   standend();
   D[ghosts[i].y][ghosts[i].x]=GHOST_SYMBOL;
   mvaddch( y_cur, x_cur,  D[y_cur][x_cur] );
   return 1;
}

void next_level(int user)
{  
   level++;
   initscr();
   int i,j,c;
   ghosts_period-=period_decrease;
   if( ghosts_period<1){
      printf("DEN UPORSTIRIZETAI TOSO MIKRI PERIODOS\n");
      exit(0);}
   if(user)                           //user ask for next level???       //
      packman.points+=5000;          //if not 5000 points for next level//
   for ( i=0 ; i<lines ; i++   ){
      for ( j=0 ; j<col ; j++  ){
          D[i][j]=CONST[i][j];
          mvaddch(i,j,CONST[i][j]);}     //exactly to their ex-positions//
   }
   P_count=4;
   reset(1);
   refresh();
   noecho();
   crmode();
}

void reset(int how)//1-for next lever,2-if pacman is killed//
{  
  int i;
  if(how==1){
     for(i=0;i<G_count;i++){
        mvaddch(ghosts[i].y,ghosts[i].x,CONST[ghosts[i].y][ghosts[i].x]);
        mvaddch(ghosts[i].y=ghosts[i].y_initial,ghosts[i].x=ghosts[i].x_initial,GHOST_SYMBOL);
        D[ghosts[i].y][ghosts[i].x]=CONST[ghosts[i].y][ghosts[i].x];
        ghosts[i].inside=1;
        ghosts[i].temp=BLANK;}
      mvaddch(packman.y,packman.x,CONST[packman.y][packman.x]);
      mvaddch(packman.y_initial,packman.x_initial,PAC_SYMBOL);
      D[packman.y][packman.x]=CONST[packman.y][packman.x];
      packman.x=packman.x_initial;
      packman.y=packman.y_initial;
      packman.x_dir=packman.dot_count=packman.y_dir=0;
      move(LINES-1,COLS-1);}
  else{
     for(i=0;i<G_count;i++){
        mvaddch(ghosts[i].y,ghosts[i].x,ghosts[i].temp);
        mvaddch(ghosts[i].y_initial,ghosts[i].x_initial,GHOST_SYMBOL);
        D[ghosts[i].y][ghosts[i].x]=ghosts[i].temp;
        ghosts[i].x=ghosts[i].x_initial;
        ghosts[i].y=ghosts[i].y_initial;
        ghosts[i].inside=1;}
     D[packman.y][packman.x]=BLANK;
     mvaddch(packman.y,packman.x,BLANK);
     D[packman.y=packman.y_initial][packman.x=packman.x_initial]=PAC_SYMBOL;
     packman.x_dir=packman.y_dir=0;
     mvaddch(packman.y,packman.x,PAC_SYMBOL);
  }
  beggin=0;
}

void Floyd_Warshall(int **W,int dots,int **d)
{
   int k,j,i;
   for(i=0;i<dots;i++)
     for(j=0;j<dots;j++)
       d[i][j]=W[i][j];
   for(k=0;k<dots;k++)
     for(i=0;i<dots;i++)
       for(j=0;j<dots;j++)
         if((d[i][k]+d[k][j])<d[i][j])
            d[i][j]=d[i][k]+d[k][j];
}

void find(int i,int j,int col,int *pos ,int A[])
{
    int p;
      for(p=0;p<dots;p++)
       if (A[p]==i*col+j){ *pos=p;break;}
       else *pos=-1;
}
