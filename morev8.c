#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<termios.h>
#include <sys/ioctl.h>
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> 
#include<string.h>
int LINELEN=50; 
int PAGELEN=20;
void do_more(FILE *);
int get_input(FILE *,int);
int calc_lines(FILE *);
char * fileName = "long";
struct termios saved_attributes;
void reset_input_mode(void)
{
	tcsetattr(STDIN_FILENO,TCSANOW,&saved_attributes);
}
void set_input_mode(void)
{
	struct termios tattr;
	char * name;
	if(!isatty(STDIN_FILENO))
	{
		fprintf(stderr,"Not a terminal.\n");
		exit(EXIT_FAILURE);
	}
	tcgetattr(STDIN_FILENO,&saved_attributes);
	//atexit(reset_input_mode);
	tattr.c_lflag &= ~(ICANON|ECHO);
	tattr.c_cc[VMIN]=1;
	tattr.c_cc[VTIME]=0;
	tcsetattr(STDIN_FILENO,TCSAFLUSH,&tattr);
}
void sig_handler(int signo)
{	
	struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    if (signo == SIGWINCH){
    	//LINELEN = (int)w.ws_col - 1;
    	PAGELEN = (int)w.ws_row - 1;
    }
}
int search_string(FILE * fp,char * str)
{
	int cur = fseek(fp,0,SEEK_CUR);
	char temp[512];
	while(fgets(temp, 512, fp) != NULL)
	{
		if((strstr(temp, str)) != NULL)
		{
			fseek(fp,-10,SEEK_CUR);
			return 1;
		}
		//line_num++;
	}
	fseek(fp,cur,SEEK_SET);
   	return -1;
}
int main(int argc,char * argv[])
{
	struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    //LINELEN = (int)w.ws_col - 1;
    PAGELEN = (int)w.ws_row - 1;
	if (signal(SIGWINCH, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGWINCH\n");
	if(argc==1)
	{
		do_more(stdin);
	}
	int i=0;
	while(++i<argc){
		FILE * fp;
		fileName = argv[i];
		fp=fopen(argv[i],"r");
		if(fp==NULL)
		{
			perror("Can't open file!\n");
			exit(1);
		}
		do_more(fp);
		fclose(fp);
	}
	//reset_input_mode();
	return 0;
}
int calc_lines(FILE * fp)
{
	int lines = 0;
	char buffer[LINELEN];
	while(fgets(buffer,LINELEN,fp))
	{
		lines++;
	}
	fseek(fp, 0, SEEK_SET);
	return lines;
}

void do_more(FILE * fp)
{
	int num_of_lines = 0;
	int total_lines = 0;
	int rv;
	int file_lines = 10; 
	file_lines = calc_lines(fp);
	FILE * fp_tty = fopen("/dev/tty","r");
	char buffer[LINELEN];
	while(fgets(buffer,LINELEN,fp))
	{
		fputs(buffer,stdout);
		num_of_lines++;
		total_lines++;
		if(num_of_lines == PAGELEN){
			double percent=0;
			percent = (total_lines*100) / file_lines;
			rv = get_input(fp_tty,percent);
		 	//reset_input_mode();
			if(rv==0)
			{
				printf("\n");
				printf("\033[1A \033[2K \033[1G");
				break;
			}
			else if(rv == 1)
			{	
				printf("\n");
				printf("\033[1A \033[2K \033[1G");
				num_of_lines -= PAGELEN;
			}
			
			else if(rv == 2)
			{
				printf("\n");
				printf("\033[1A \033[2K \033[1G");
				num_of_lines -= 1;
			}
			else if(rv==3)
			{
				//total_lines=0;
				pid_t pid=fork();
				if (pid==0) { /* child process */
					const char * fileName2 = fileName;
					char *argv2[]={"vim",fileName2,NULL};
					execv("/bin/vim",argv2);
					exit(127); /* only if execv fails */
				}
				else { /* pid!=0; parent process */
					waitpid(pid,0,0); /* wait for child to exit */
					do_more(fp);
				}		
			}
			else if(rv==5)
			{
				printf("\n");
				printf("\033[1A \033[2K \033[1G");
				break;
			}
			else if(rv == 4){
				int offs = fseek(fp,0,SEEK_CUR);
				printf("\n/");
				char arr[512];
				scanf("%s",arr);
				if(search_string(fp,arr) == 1)
				{
					num_of_lines -= PAGELEN-1;
					printf("\033[7m skipping... \033[m");
					printf("\n");
				}
				else
				{
					//fseek(fp,offs,SEEK_SET);
					num_of_lines -= PAGELEN;
					printf("\033[7m No such pattern \033[m");	
				}
				
			}
		}
	}
}
int get_input(FILE * cmdstream,int percent)
{
	char c;
	printf("\033[7m --more--(%d%%) \033[m",percent);
	set_input_mode();
	c=getc(cmdstream);
	reset_input_mode();
	if(c=='q')
		return 0;
	if(c==' ')
		return 1;
	if(c=='\n')
		return 2;
	if(c=='v')
		return 3;
	if(c=='/')
		return 4;
	return 5;
}

