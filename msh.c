/*
 * Kim Huynh
 * Wilder
 * CS240 
 * 10/4/20, 10/25/20 
 *  
 * NOTES:  
 * My shell (msh) supports basic Unix commands supported by execvp, cd.
 * Custom commands supported: 
 *    *exit             -Exit terminal. 
 *    *colorme (color)  -Colors terminal text. Typing it incorrectly will show color options.
 *    *mshrc            -Executes commands from mshrc
 *    *history          -Shows history and corresponding !___, !! commands.
 *    *alias/ unalias   -Create/ delete alias, or show alias list. Doesn't support pipes or ; currently. 
 *    *export           -To change path location. 
 * 
 * Recursive ; and piping (|) is supported.
 * It does not, at the moment, support shortcuts like (tab) or word expansions (*).  
 *
 * The shell does not also have a custom support help page. Sorry. :( 
 * ASCII art is from https://www.asciiart.eu/computers/computers.
 * Linked list code modified from https://www.tutorialspoint.com/data_structures_algorithms/linked_list_program_in_c.htm.
 */
 
 
//Includes.
#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/wait.h> 

//Defines. 
#define MAX 100  //Max input size.

//Global var.
FILE *hist;           //History text pointer.
int historyCounter=0; //Keeps track of current increment for history. 

//For alias usage. Linked list.
struct node {
   char command[MAX];
   char name[MAX];
   struct node *next;
};
struct node *head = NULL;
struct node *current = NULL;

//Prototypes.
void initMessage();                                              //Initialize shell with a nice greeting! Like the fancy Linux interfaces.  
void checkExecute(char input[], char* inputArg[]);               //Checks commands, parses, and executes accordingly. 
void readLine(char str[]);                                       //Reads input command line from user.
int checkLine(char str[]);                                       //Check if just spaces or no input. Prevents seg fault. Returns 1 if empty.  
void parseLine(char str[], char* strArg[]);                      //Parse input into array of strings (strArg) using strtok. 
int checkBuiltIn(char str[]);                                    //Check if command is built in. 
void execute(char* strArg[]);                                    //Execute command, aka make child and execvp.
void executePipe(char strPipe[]);                                //Execute command, aka make child and execvp with pipes.
void executeColon(char str[]);                                   //Special execution for ;.
void executeBuiltIn(char* strArg[]);                             //Execute special commands. 
void executeAlias(char nameA[]);                                 //Execute alias.
void changeFontColor(char* strArg[]);                            //Change text color.
void updateHistory(char str[], FILE **fp);                       //Increment history text.
void displayHistory(FILE **fp);                                  //Display last 20 commands of history text.
void goToHistory(FILE **fp, char str[]);                         //Execute history command from text.
void createAlias(char *strArg[]);                                //Create alias from user input.
void displayAlias();                                             //Displays all created aliases.
struct node* deleteAlias(char nameA[]);                          //Delete selected alias from linked list.
struct node* findAlias(char nameA[]);                            //Find selected alias in linked list.  


int main(){
  char input[MAX];              //Initial input.
  char *inputArg[MAX];          //Parsed input.

  hist = fopen ("bash_history", "w+" ); //File containing history of commands. 
  
  initMessage();
  
  while(1){
  
    readLine(input);
    if (checkLine(input)==1){ //Check if no command given or just a bunch of spaces. 
      continue; //Go on.
    }
    updateHistory(input, &hist);   //Add to hsitory file.
    
    checkExecute(input, inputArg); //Checks conditions (;,|), parses, executes.
  }
  return 0;  
}

//-------------------------------------------------------------------------------------------------------------End of main.

void initMessage(){ //Was experimenting with cool ASCII art. 
  printf(".--.                        \n");
  printf("|__| .-------.              \n");
  printf("|=.| |.-----.| Type exit    \n");
  printf("|--| || MSH || to terminate \n");
  printf("|  | |'-----'| shell.       \n");
  printf("|__|~')_____('              \n");
  printf("\nTo execute commands from mshrc, type mshrc\n");
  
}

void checkExecute(char input[], char* inputArg[]){
  if (checkLine(input)==2){ //Check if has colons (;). It will be executed differently than normal.
    executeColon(input);
    return;
  }
  if (checkLine(input)==3){ //Check if has pipes (|). It will be executed differently than normal.
    executePipe(input);
    return;
  }
  if (checkLine(input)==4){ //Check if has !! or !.
    goToHistory(&hist, input);
    return;
  }

  parseLine(input, inputArg); //Parse input and put into inputArg.
  
  if (findAlias(inputArg[0])!=NULL){              //If alias, execute alias. 
    executeAlias(inputArg[0]);
  }
  else if (checkBuiltIn(inputArg[0])){            //If built-in, execute built-in command.
    executeBuiltIn(inputArg);
  }
  else{                                           //Execute command, since it is not the above conditions. 
    execute(inputArg);                             
  }
}

void readLine(char str[]){
  char ch; //Character.
  int i=0; //Index. Where array starts. 
  
  printf("\n>: ");
  while (( ch = getchar()) != '\n' || i==(MAX-1)){
    str[i++] = ch; //Put char in string while end is not reached. 
  }
  str[i] = '\0';
} 

int checkLine(char str[]){
  int i=0; //Index
  int emptyCounter=0;  //How much of it is empty?
  int colonCounter=0;  //Do semicolons (;) exist?
  int pipeCounter=0;  //Do semicolons (;) exist?
  int histCounter=0;  //Is someone doing !___ or !!?
  
  int len=strlen(str); //Length of string. 
  
  for (i=0; i<len; i++){
    if (str[i]==' '|| str[i]=='\0'){
      emptyCounter++;
    }
    if (str[i]==';'){
      colonCounter++;
    }
    if (str[i]=='|'){
      pipeCounter++;
    }
    if (str[i]=='!' && i<2){
      histCounter++;
    }
  }
  if (emptyCounter==len){ //If it is fully empty/ equal to string length.
    return 1;
  }
  else if (colonCounter>0){ //Has ;
    return 2;
  }
  else if (pipeCounter>0){  //Has |
    return 3; 
  }
  else if (histCounter>0){  //Has !, !!
    return 4; 
  }
  else{
    return 0;
  }
}

void parseLine(char str[], char* strArg[]){ 
  char *token; 
  int i=0; 
  token = strtok(str," "); //Use strtok method to split string at spaces. 
  while(token!=NULL){
      strArg[i]=token;     
      i++;
      token = strtok(NULL," ");
  }
  strArg[i]=NULL; //End string.   
}

int checkBuiltIn(char str[]){
  if (strcasecmp(str, "exit")==0                     //Exit shell.
      || strcasecmp(str, "cd")==0                    //Change directory.
      || strcasecmp(str, "ColorMe")==0               //Color terminal text.
      || strcasecmp(str, "mshrc")==0                 //Execute mshrc file.
      || strcasecmp(str, "history")==0               //Show history.
      || strcasecmp(str, "alias")==0                 //Show alias list or create one.
      || strcasecmp(str, "unalias")==0               //Delete alias
      || (strcasecmp(str, "echo")==0 && str[5]=='$') //For showing PATH value mainly.
      || strcasecmp(str, "export")==0                //Export a path or something.
      ){    
    return 1;   
  }
  return 0;
}

void execute(char* strArg[]){

  pid_t pid = fork();  //Fork the child! 
  int exe;             //Dummy variable to hold value.

  if (pid == 0){ 
    exe=execvp(strArg[0], strArg); //Execute!
    
    if (exe < 0){ //If it failed...
      printf("\nOops, something failed! Is this an invalid command?\n"); 
      exit(0); //Exit out of function. 
    } 
  } 
  else{ //Parent process.
    wait(NULL); //Wait for the child to die.   
  } 

}

void executePipe(char strPipe[]){ 
  int i=0; //Just counters.
  int i2=0;
  int i3=0;
  int pipeNum=0;
  char *strArg[MAX];//Parsed input.
  char *token;
  
  for(i; i<strlen(strPipe); i++){
    if(strPipe[i]=='|'){
      pipeNum++; //Count number of pipes.
    }
  }
  //printf("%d pipes\n", pipeNum);
  
  int pipefds[2*pipeNum]; //Create needed pipes.
  for(i=0; i<pipeNum; i++){
    pipe(pipefds + i*2);
  }  
  
  //First use strtok to separate at |.
  i=0;
  token = strtok(strPipe,"|"); 
  while(token!=NULL){
    strArg[i]=token;     
    i++;
    token = strtok(NULL,"|");
    strArg[i]=NULL; //End string.   
  }  
  
  //Recursion version for pipes.
  pid_t pid;
  int current=0;
  while((strArg[current]!=NULL)||(strArg[current]!='\0')){
    char *strArg2[MAX];
    char *token2;
    i2=0;
    token2 = strtok(strArg[current]," "); //Remove spaces. Get current command. 
    while(token2!=NULL){
      strArg2[i2]=token2;     
      i2++;
      token2 = strtok(NULL," ");
      strArg2[i2]=NULL; //End string.   
    }
    pid = fork(); //Fork it.
    if(pid == 0) {
      if(strArg[current+1]!=NULL || strArg[current+1]!='\0'){ //Next command isn't null.
        dup2(pipefds[current*2+1], 1);
      }
      if(current!=0){ //Not the first command. Add && (current!=2*pipeNum)?
        dup2(pipefds[(current-1)*2], 0);
      }
      for(i = 0; i < (2*pipeNum); i++){
        close(pipefds[i]);
      }
      if(execvp(strArg2[0], strArg2) < 0 ){
        printf("Is there an incorrect command somewhere? \n");
      }
    } 
    else if(pid < 0){ //Didn't fork succesfully. 
      printf("Hmmm...I see a bug somewhere.");
    }
    current++;
  }

  for(i = 0; i < (2 * pipeNum); i++){ //Close all when done.
      close(pipefds[i]);
  }
  for(i = 0; i < (pipeNum + 1); i++){ //Wait warmly. 
      wait(NULL);
  }
  
  
  //A working fork. but limited to 2.
  /*char *strArg2[MAX];
  char *token2;
  char *strArg3[MAX];
  char *token3;
  i2=0;
  i3=0;
  token2 = strtok(strArg[0]," "); 
  while(token2!=NULL){
    strArg2[i2]=token2;     
    i2++;
    token2 = strtok(NULL," ");
    strArg2[i2]=NULL; //End string.   
  }
  token3 = strtok(strArg[1]," "); 
  while(token3!=NULL){
    strArg3[i3]=token3;     
    i3++;
    token3 = strtok(NULL," ");
    strArg3[i3]=NULL; //End string.   
  }
  
  
  int pipefds[2];
  if(pipe(pipefds) == -1) {
    perror("Pipe failed");
    exit(1);
  }

  if(fork() == 0)            
  {
      close(STDOUT_FILENO);  
      dup(pipefds[1]);         
      close(pipefds[0]);       
      close(pipefds[1]);

      execvp(strArg2[0], strArg2);
      perror("execvp of ls failed");
      //exit(1);
  }

  if(fork() == 0)            
  {
      close(STDIN_FILENO);   
      dup(pipefds[0]);         
      close(pipefds[1]);      
      close(pipefds[0]);
      
      execvp(strArg3[0], strArg3);
      perror("execvp of wc failed");
      //exit(1);
  }

  close(pipefds[0]);
  close(pipefds[1]);
  wait(0);
  wait(0);*/
  
  
}

void executeBuiltIn(char* strArg[]){
  if (strcasecmp(strArg[0], "exit")==0){           //Exit shell.
    printf("\nBye! Thanks for using my shell.\n\n");
    strArg[1]="reset";
    changeFontColor(strArg);
    fclose(hist);
    exit(0);
  }
  else if (strcasecmp(strArg[0], "cd")==0){        //cd will not work when forking, so do it here. 
    chdir(strArg[1]);
  }
  else if (strcasecmp(strArg[0], "ColorMe")==0){  //Check if want to change color. 
    changeFontColor(strArg);
  }
  else if (strcasecmp(strArg[0], "mshrc")==0){    //Check if want to execute mshrc
    FILE *fp = fopen ("mshrc", "r" );
    if (fp != NULL) {
      char input[MAX];
      char *inputArg[MAX];          //Parsed input.
      
      while(fgets(input,sizeof(input),fp)!= NULL){ 
        input[strcspn(input, "\n")] = 0;
        if (checkLine(input)==1){ //Check if no command given or just a bunch of spaces. 
          continue; //Go on.
        }
        checkExecute(input, inputArg);
      }
      fclose(fp);
    }
    else {
      printf("Please fill mshrc with commands.\n"); 
    }
  }
  else if (strcasecmp(strArg[0], "history")==0){
    displayHistory(&hist);
  }
  else if (strcasecmp(strArg[0], "alias")==0){
    if (strArg[1]==NULL || strArg[1]=='\0'){
      displayAlias();
    }
    else{
      createAlias(strArg);
    }
  }
  else if (strcasecmp(strArg[0], "unalias")==0){
    if (strArg[1]==NULL || strArg[1]=='\0'){
      printf("Please specify which alias you would like to delete. \n");
    }
    else{
      deleteAlias(strArg[1]);
    }
  }
  else if (strcasecmp(strArg[0], "echo")==0 && strArg[1][0]=='$'){
    if (strcasecmp(strArg[1], "$PATH")==0){
      printf (getenv("PATH"));
    }
    else if (strArg[1]!=NULL && strArg[1][0]=='$'){ //Execute variable. 
      char com[MAX];
      sscanf(strArg[1],"$%s", com);
      printf (getenv(com));
    }
    /*else if (strArg[1]!=NULL && strArg[1][0]=='\"'){ //Probably a string of text. Work in progress.
      char com[MAX];
      sscanf(strArg[1],"\"%[^\"]", com);
      printf("%s \n", com);
    }*/
    else{
      printf("That command is not currently supported. \n");
    }
  }
  else if (strcasecmp(strArg[0], "export")==0){
    char newP[MAX];
    char oldP[MAX];
    char value[MAX];
    sscanf(strArg[1], "%[^=]=%[^:]%s", newP, oldP, value); 
    sscanf(oldP,"$%s", oldP); //Get rid of $.
    char *expandP=getenv(oldP);
    strcat(expandP, value);
    setenv(newP, expandP, 1);
  }
}

void executeColon(char str[]){
  char *token; 
  int i=0; 
  int z=0;
  int i2=0;
  char *strArg[MAX];          //Parsed input.

  //First use strtok to separate at ;.
  token = strtok(str,";"); 
  while(token!=NULL){
    strArg[i]=token;     
    i++;
    token = strtok(NULL,";");
    strArg[i]=NULL; //End string.   
  }  
  
  //Then use strtok to separate at spaces for each command split by ;. 
  while((strArg[z]!=NULL)||(strArg[z]!='\0')){
    char *strArg2[MAX];
    char *token2;
    i2=0;
    token2 = strtok(strArg[z]," "); 
    while(token2!=NULL){
      strArg2[i2]=token2;     
      i2++;
      token2 = strtok(NULL," ");
      strArg2[i2]=NULL; //End string.   
    }
    
    //checkExecute(strArg[z], strArg2);
    if (findAlias(strArg2[0])!=NULL){              //If alias, execute alias. 
      executeAlias(strArg2[0]);
    }
    else if (checkBuiltIn(strArg2[0])){            //If built-in, execute built-in command.
      executeBuiltIn(strArg2);
    }
    else{                                           //Execute command, since it is not the above conditions. 
      execute(strArg2);                             
    }
    z++; //Go on to next command.
  }
}

void changeFontColor(char *strArg[]){

  int i;
  int changed=0;
  char *colors[]={"RED", "YELLOW", "GREEN", "BLUE", "CYAN", "MAGENTA", "WHITE", "RESET"};
  char *colorsLabel[]={"\x1B[31m", "\x1B[33m", "\x1B[32m", "\x1B[34m", "\x1B[36m", "\x1B[35m", "\x1B[37m", "\x1B[0m"};
  
  if (strArg[1]!=NULL || strArg[1]!='\0'){ //Error checking. Make sure color is specified.  
    for (i=0; i<8; i++){
      if (strcasecmp(strArg[1], colors[i])==0){
        changed=1;
        printf(colorsLabel[i]); //Use according command.
      }
    }
  }
  if (changed==0){
    printf("\nThat color is not supported by this shell, or no color was specified."); 
    printf("\nSupported colors include: red, yellow, green, blue, cyan, magenta, white.");
    printf("\nTo reset font color, type in reset as the color option.\n");
  }
}

void updateHistory(char str[], FILE **fp){
  historyCounter++;
  fprintf(*fp, "%s \n", str); //Add to history file.
  fseek ( *fp, 0, SEEK_CUR);  //Switch directions.
}

void displayHistory(FILE **fp){
  char input[MAX];
  int size=historyCounter-20;
  int i=0;
  
  FILE *fp2 = fopen ("bash_history", "r" );
  
  if (size<0){
    size=0;
  }
  for (i=0; i<size; i++){ //Read past sections until get to 20 most recent. 
    fgets(input,sizeof(input),fp2);
  }
  while(fgets(input,sizeof(input),fp2)!= NULL){ 
    input[strcspn(input, "\n")] = 0;
    printf("%d %s \n", size++, input);
  }
  fclose(fp2);
}

void goToHistory(FILE **fp, char str[]){
  char temp[6];
  char input[MAX];
  char *inputArg[MAX];
  int excl=0;
  int i=0;
  int x=0;
  int lineNum;
  FILE *fp3 = fopen ("bash_history", "r" );
   
  while (str[i]!='\0'){
    if (str[i]=='!'){ //Check if !! or !___
      excl++;
    }
    else{
      temp[x]=str[i];
      x++;
    }
    i++;
  }
  temp[x]='\0';
  lineNum=atoi(temp);
  
  if ((lineNum>=(historyCounter-20) && lineNum<historyCounter && excl!=2) || (excl==2 && lineNum==0)){ //Check options to see if valid. 
  
    if (lineNum<historyCounter && excl!=2){
      for (i=0; i<=lineNum; i++){
        fgets(input,sizeof(input),fp3);
      }
      input[strcspn(input, "\n")] = 0;
      printf("Now executing history: %d %s \n", lineNum, input);
    }
    else if (excl==2 && lineNum==0){
      for (i=0; i<historyCounter-1; i++){
          fgets(input,sizeof(input),fp3);
      }
      input[strcspn(input, "\n")] = 0;
      if((historyCounter-2)==-1){
        printf("There is no history to repeat but this one...try again later. \n");
        return;
      }
      else{
        printf("Now executing last history: %d %s \n", historyCounter-2, input);
      }
    }
    checkExecute(input, inputArg);
  }
  else{
    printf("That history number is not stored currently. \n");
  }
  fclose(fp3);
}

void createAlias(char *strArg[]){
  char nameA[MAX];      //For name.
  char comA[MAX]="\0";  //For command.     
  int i=2;
  
  sscanf(strArg[1], "%[^=]=\'%[^']", nameA, comA);
  while (strArg[i]!=NULL){
    char temp[MAX];
    sscanf(strArg[i], "%s", temp);
    char *rem; //Remove delimiter '
    rem = strchr(temp,'\'');   //Get the pointer to char token
    *rem = '\0'; 
    strcat(comA, " ");
    strcat(comA, temp);
    i++;
  } 
  /*if (comA[strlen(comA)-1]=='\''){
    comA[strlen(comA)-1]=='\0';
  }*/
  
  if(findAlias(nameA)==NULL && comA[0]!='\0'){ //Double-checking to see if collects junk values
    struct node *temp = (struct node*) malloc(sizeof(struct node));	
    strcpy(temp->name, nameA);
    strcpy(temp->command, comA);
  	
    temp->next=head; 
    head=temp; //Make front of linked list. 
    printf("Alias created. \n");
  }
  else{
    printf("Alias command not recognized or already exists. \n");
    printf("Try getting rid of the spaces between the alias name, =, and 'command'. \n");
    printf("Example: alias name='command' \n"); 
  }
}

void displayAlias() {
   struct node *ptr = head;
   printf("Alias list: \n");
   while(ptr != NULL) {
      printf("%s = %s \n",ptr->name,ptr->command);
      ptr = ptr->next; //Move through linked list.
   }
}

struct node* deleteAlias(char nameA[]){
  struct node* current = head;
  struct node* previous = NULL;
	
  if(head == NULL) {
    printf("There are no aliases to delete. \n");
    return NULL;
  }

  while(strcmp(current->name, nameA)!=0) {
    if(current->next == NULL) { //It's the last node...
       return NULL;
    } 
    else {
       previous = current;
       current = current->next;
    }
  }
  if(current == head) {
    head = head->next;
    printf("Alias deleted. \n");
  } 
  else {
    previous->next = current->next;
  }    
  return current;
}

struct node* findAlias(char nameA[]){
   struct node* current = head;

   if(head == NULL) {
     return NULL; //Doesn't exist, like my brain.
   }

   while(strcmp(current->name, nameA)!=0) {
      if(current->next == NULL) {
         return NULL;
      } 
      else {
         current = current->next;
      }
   }      
   return current; //Return alias if found.
}

void executeAlias(char nameA[]){
  char *inputArg[MAX];
  struct node *findA =findAlias(nameA);  
  
  checkExecute(findA->command, inputArg);  
  /*parseLine(findA->command, inputArg);
  
  if (checkBuiltIn(inputArg[0])){
    executeBuiltIn(inputArg);
  }
  else{
    execute(inputArg);                          
  }*/
}