//printf in color! for windows only :(

#include <stdio.h>
#include <stdlib.h>		//for system()

extern FILE *outfile;

//background can be set with \033[fg;bgm
//the set /p suppresses the newline
#define RED_FONT        "echo|set /p= \033[31m"
#define GREEN_FONT      "echo|set /p= \033[32m"
#define YELLOW_FONT     "echo|set /p= \033[33m"
#define BLUE_FONT       "echo|set /p= \033[34m"
#define MAGENTA_FONT    "echo|set /p= \033[35m"
#define CYAN_FONT       "echo|set /p= \033[36m"
#define WHITE_FONT      "echo|set /p= \033[37m"
#define RESET_FONT  	"echo|set /p= \033[0m"

#define set_text_color(color)	system(color);

#define printf(...)	({int cc = printf(__VA_ARGS__); if(outfile) fprintf(outfile, __VA_ARGS__); cc;})
#define puts(s)		({int cc = puts(s); if(outfile) fputs(s, outfile); fputc('\n', outfile); cc;})
#define putchar(c)	({int cc = putchar(c); if(outfile) fputc(c, outfile); cc;})

#define printfcol(color, ...)			\
	({								\
		set_text_color(color);			\
		int dummy = printf(__VA_ARGS__);			\
		set_text_color(RESET_FONT);		\
		dummy;							\
	})

#define putscol(color, str)				\
	do {								\
		set_text_color(color);			\
		puts(str);						\
		set_text_color(RESET_FONT);		\
	} while(0)


