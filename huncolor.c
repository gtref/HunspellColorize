#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include <hunspell.h>

const char *aff_path = "/usr/share/hunspell/en_US.aff";
const char *dic_path = "/usr/share/hunspell/en_US.dic";

#define START "\033[1;31m" // RED ANSI CODE
#define STOP "\033[0m"
#define UNDER_ON "\033[4m" // ENABLE UNDERLINING ANSI CODE
#define BOLD_ON "\033[1;36m" // ENABLE BOLD & CYAN ANSI CODE

#define MAX_IGNORE 128
#define SMALLBUF 80

struct state {
	Hunhandle *hunhandle;
	enum {
		Newline,
		Noise,
		Word,
		NotAWord,
		Escape,
	} state;
	int esclen;
	char escape[SMALLBUF];
	int wordlen;
	char word[SMALLBUF];
	int resetlen;
	char reset[SMALLBUF];
	int underline;
	int bold;
	int ignore_count;
	const char *ignore_words[MAX_IGNORE];
	int line_count;
    int column_count;
	int count_lines;
	int count_columns;
	int max_columns;
};

static void print_counts(struct state *st)
{
    if (st->count_lines)
        fprintf(stderr, "Lines: %d\n", st->line_count);

    if (st->count_columns) {
        int columns = st->max_columns;

        /*
         * Handle a file that doesn't end with a newline.
         */
        if (st->column_count - 1 > columns)
            columns = st->column_count - 1;

        fprintf(stderr, "Columns: %d\n", columns);
    }
}

static void count_position(struct state *st, unsigned char c)
{
    if (c == '\n') {
        st->line_count++;

        if (st->column_count - 1 > st->max_columns)
            st->max_columns = st->column_count - 1;

        st->column_count = 1;
    } else {
        st->column_count++;
    }
}

static int is_ignored(struct state *st) {
	for (int i = 0; i < st->ignore_count; i++) {
		if (strcmp(st->word, st->ignore_words[i]) == 0) {
			return 1;
		}
	}
	return 0;
}

static int check_word(struct state *st)
{
    /* We're not doing German or Finnish... */
    if (st->wordlen == SMALLBUF)
        return 1;

    st->word[st->wordlen] = '\0';

    if (is_ignored(st))
        return 1;

    return Hunspell_spell(st->hunhandle, st->word);
}

static void check_and_print(struct state *st)
{
	if (check_word(st)) {
		write(1, st->word, st->wordlen);
		return;
	}

	// write(1, START, strlen(START));
	// write(1, st->word, st->wordlen);
	// write(1, STOP, strlen(STOP));
	// write(1, st->reset, st->resetlen);

	if (st->underline) {
		write(1, UNDER_ON, strlen(UNDER_ON));
		write(1, st->word, st->wordlen);
		write(1, STOP, strlen(STOP));
		write(1, st->reset, st->resetlen);
	} else if (st->bold) {
		write(1, BOLD_ON, strlen(BOLD_ON));
		write(1, st->word, st->wordlen);
		write(1, STOP, strlen(STOP));
		write(1, st->reset, st->resetlen);
	} else {
		write(1, START, strlen(START));
		write(1, st->word, st->wordlen);
		write(1, STOP, strlen(STOP));
		write(1, st->reset, st->resetlen);
	}
}

static void handle_escape(struct state *st, char c)
{
	if (st->esclen < SMALLBUF)
		st->escape[st->esclen++] = c;
}

static void remember_escape(struct state *st)
{
	st->resetlen = st->esclen;
	memcpy(st->reset, st->escape, st->esclen);
}

// Print a single line, colorizing unrecognized words
//
// This is incredibly stupid, and only handles plain
// US-ASCII text.
static void process(struct state *st, const char *buf, size_t len)
{
	const char *last = buf;

	for ( ; len > 0 ; len--, buf++) {
		unsigned char c = *buf;

		if (st->count_lines || st->count_columns)
        	count_position(st, c);


		switch (c) {
		case 128 ... 255:
		case 'A' ... 'Z':
		case 'a' ... 'z':
			switch (st->state) {
			case Word:
				if (st->wordlen == SMALLBUF) {
					write(1, st->word, SMALLBUF);
					st->state = NotAWord;
					continue;
				}
				st->word[st->wordlen++] = c;
				continue;
			case NotAWord:
				continue;
			case Escape:
				handle_escape(st, c);
				if (c == 'm')
					remember_escape(st);
				st->state = Noise;
				continue;
			default:
				if (last < buf)
					write(1, last, buf-last);
				st->state = Word;
				st->wordlen = 1;
				st->word[0] = c;
				continue;
			}

		// Mixed letters and numbers / underscores are C identifiers
		case '0' ... '9':
		case '_':
			switch (st->state) {
			case Escape:
				handle_escape(st, c);
				continue;
			case Word:
				write(1, st->word, st->wordlen);
				last = buf;
				st->state = NotAWord;
				continue;
			default:
				st->state = NotAWord;
				continue;
			}

		// Special case
		case '\'':
			if (st->state == Word && len > 1 && isalpha(buf[1]) && st->wordlen < SMALLBUF) {
				st->word[st->wordlen++] = c;
				continue;
			}
			/* fallthrough */
		default:
			switch (st->state) {
			case Escape:
				handle_escape(st, c);
				continue;
			case Word:
				check_and_print(st);
				last = buf;
				break;
			default:
				break;
			}
			switch (c) {
			case '\n':
				st->state = Newline;
				continue;
			case '\033':
				st->state = Escape;
				st->esclen = 1;
				st->escape[0] = '\033';
				continue;
			default:
				st->state = Noise;
				continue;
			}
		}
	}

	// We always flush the buffer at at the end
	if (st->state != Word && last < buf)
		write(1, last, buf-last);
}

static void exec_less(void)
{
	char *less_args[] = { "less", NULL };
	execvp("less", less_args);
	perror("Couldn't exec 'less'");
	exit(1);
}

static void local_dictionary(Hunhandle *handle, const char *filename)
{
	struct stat st;
	if (!stat(filename, &st) && S_ISREG(st.st_mode))
		Hunspell_add_dic(handle, filename);
}

#define BUFSIZE 1024

int main(int argc, char **argv)
{
	int fd[2];
	char buf[BUFSIZE];
	const char *custom_dic = NULL;
	const char *input_file = NULL;
	int underline = 0;
	int bold = 0;
	int count_lines_flag = 0;
	int count_columns_flag = 0;
	const char *ignore_words[MAX_IGNORE];
	int ignore_count = 0;
	

	// Parse command line options
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			printf("Usage: %s [-d custom.dic] [-u] [-b] [-i WORD] [FILE]\n", argv[0]);
			return 0;
		}
		if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dict") == 0) && i + 1 < argc) {
			custom_dic = argv[++i];
		} else if (argv[i][0] != '-') {
			input_file = argv[i];
		} else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--underline") == 0) {
			underline = 1;
		} else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bold") == 0) {
			underline = 0;
			bold = 1;
			// Set a flag to enable bold formatting
			// This will be handled in the check_and_print function
			// where we can add a condition to check for this flag
			// and apply bold formatting instead of underline.
			// For now, we just set underline to 0 to disable it.
		} else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--count-lines") == 0) {
			count_lines_flag = 1;
		} else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--count-columns") == 0) {
			count_columns_flag = 1;
		} else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--ignore") == 0) {
    		if (i + 1 >= argc) {
            	fprintf(stderr, "%s requires a word\n", argv[i]);
            	return 1;
        	}

        	if (ignore_count >= MAX_IGNORE) {
            	fprintf(stderr, "Too many ignored words (max %d)\n", MAX_IGNORE);
            	return 1;
        	}

        	ignore_words[ignore_count++] = argv[++i];

		} else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			return 1;
		}
	}

	setenv("LESS", "-FRX", 0);

	if (input_file) {
		if (!freopen(input_file, "r", stdin)) {
			perror(input_file);
			return 1;
		}
	}

	if (isatty(0))
		exec_less();

	Hunhandle *hunhandle = Hunspell_create(aff_path, dic_path);

	if (!hunhandle || pipe(fd))
		exec_less();

	// Add local dictionaries from cwd and $HOME
	local_dictionary(hunhandle, ".dictionary");
	char *home = getenv("HOME");
	if (home) {
		snprintf(buf, BUFSIZE, "%s/.dictionary", home);
		local_dictionary(hunhandle, buf);
	}
	// Load custom dictionary passed via -d
	if (custom_dic) {
		local_dictionary(hunhandle, custom_dic);
	}

	if (fork()) {
		dup2(fd[0], 0);
		close(fd[0]);
		close(fd[1]);
		exec_less();
	}
	dup2(fd[1], 1);
	close(fd[0]);
	close(fd[1]);

	struct state state = {
		.hunhandle = hunhandle,
		.state = Newline,
		.resetlen = strlen(STOP),
		.reset = STOP,
		.underline = underline,
		.bold = bold,
		.ignore_count = ignore_count,
		.line_count = 0,
    	.column_count = 1,

    	.count_lines = count_lines_flag,
    	.count_columns = count_columns_flag,
		.max_columns = 0,
	};

	for (int i = 0; i < ignore_count; i++) {
    	state.ignore_words[i] = ignore_words[i];
	}
	

	for (;;) {
		ssize_t len = read(0, buf, sizeof(buf));
		if (len <= 0)
			break;

		process(&state, buf, len);
	}

	// process() flushes the input buffer at the
	// end, but will leave any partial words for
	// the next iteration. Deal with that here.
	if (state.state == Word)
		check_and_print(&state);

	print_counts(&state);

	Hunspell_destroy(hunhandle);

	return 0;
}
