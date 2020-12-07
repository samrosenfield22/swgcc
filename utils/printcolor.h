//printf in color! for windows only :(

#include <stdio.h>
#include <stdlib.h>		//for system()

extern FILE *outfile;
extern const char *outname;

//redefine print functions for logging
#define printf(...)	({int cc = printf(__VA_ARGS__); print_to_log_file(__VA_ARGS__); cc;})
#define puts(s)		({int cc = puts(s); print_to_log_file("%s\n", s); cc;})
#define putchar(c)	({int cc = putchar(c); print_to_log_file("%c", c); cc;})

#define print_to_log_file(...) ({		\
	if(outfile) {						\
		fprintf(outfile, __VA_ARGS__);	\
		fclose(outfile);				\
		outfile = fopen(outname, "a");	\
		assert(outfile);				\
	}									\
})

#ifdef __linux__
	#define PRINTTERM	"echo "
#else		//windows
	#define PRINTTERM	"echo|set /p= "	//set /p suppresses the newline
#endif

//background can be set with \033[fg;bgm
//the set /p suppresses the newline
#define RED_FONT        PRINTTERM "\033[31m"
#define GREEN_FONT      PRINTTERM "\033[32m"
#define YELLOW_FONT     PRINTTERM "\033[33m"
#define BLUE_FONT       PRINTTERM "\033[34m"
#define MAGENTA_FONT    PRINTTERM "\033[35m"
#define CYAN_FONT       PRINTTERM "\033[36m"
#define WHITE_FONT      PRINTTERM "\033[37m"
#define RESET_FONT  	PRINTTERM "\033[0m"

#define set_text_color(color)	system(color);


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


