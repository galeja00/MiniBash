#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


#define BUFFER_SIZE 500

// zjistuje jestli se radek vlezl do bufferu
int is_full_line(char *buffer, int size) {
	int i = 0;
	while (i < size) {
		if (buffer[i] == '\0') {
			return 1;
		}
		i++;
	}
	return 0;
}

// zvetsuje buffer
// funkce nevyuzita v tomto programu -> nestihl jsem implementovat
char* create_bigger_buffer(char *buffer, int *size) {
	*size *= 2;
	char* res = realloc(sizeof(char) * *size);
	for (int i = 0; i < *size; i++) {
		res[i] = buffer[i];
	}
	return res;
} 


// zjisti pocet funkci
int count_foos(char *str) {
	int i = 0;
	int count = 0;
	while (str[i] != '\0') {
		if (str[i] == ';' || str[i] == '\n') {
			count++;
		}
		if (str[i] == '&' && str[i+1] == '&') {
			count++;
		}
		i++;	
	}
	return count;
}

// zjisti pocet znaku slova/funkce
int count_word(char *str) {
	int i = 0;
	int count = 0;
	while (str[i] != ' ' && str[i] != '\0') {
		count++;
		i++;
	}
	return count;
}

// dostane odkaz na prvni pismeno slova a vrati slovo
char* get_word(char *str) {
	int i = 0;
	int count = count_word(str);
	char *word = malloc(sizeof(char) * count + 1);
	while (str[i] != ' ' && str[i] != '\0' && str[i] != '\n') {
		word[i] = str[i];
		i++;
	}
	word[i] = '\0';
	return word;
}

// najde funkci na dane pozici a navrati index prvního pismene dane funkce
int find_foo_index(char *str, int num) {
	int i = 0;
	int fun = 1;
	
	while(fun != num) {
		if (str[i] == ';' || str[i] == '\n') {
			fun++;
		}
		if (str[i] == '&' && str[i+1] == '&') {
			fun++;
			i++;
		}
		i++;
	}
	// preskoci mezery
	if (str[i] == ' ') {
		i++;
	}
	return i;
}

// ziská funkci ze stringu na dane pozici index (index je cislovane od 1)
char* parse_foo(char *str, int num) {
	int i = find_foo_index(str, num);
	char* foo = get_word(&str[i]);
	return foo;
}

// zjisti pocet argumentu funkce
int count_args(char *str, int num) {
	int i = find_foo_index(str, num);
	int count = 0;
	while (str[i] != '\0') {
		if (str[i] == ';' || str[i] == '&') {
			return count;
		}
		if (str[i] == ' ') {
			count++;
		}
		i++;
	}
	count++;
	return count;
}

// ziská argumenty funkce na dane pozici index (index je cislovane od 1)
char** parse_args(char *str, int num) {
	int i = find_foo_index(str, num);
	int count = count_args(str, num);
	char **args = malloc((sizeof(char*) * count) + 1);
	for (int j = 0; j < count; j++) {
		int chars = count_word(&str[i]);
		char* arg = get_word(&str[i]);
		args[j] = arg;
		i += chars + 1;
	}
	args[count] = NULL;
	return args;
}
// spusti linux bash program
int run_program(char *foo, char **args) {
	pid_t pid = fork();
	if (pid == -1) {
		return -1;
	}
	if (pid == 0) {
		int pr = execvp(foo, args);
		if (pr == -1) {
			return -1;
		}

	} else {
		int status;
		wait(&status);
		if (WIFEXITED(status)) {
			return WEXITSTATUS(status);
		} else {
			return -1;
		}
	}
}

// zjistí oddělovač mezí funkcemi
char get_seperator(char *str, int num) {
	char res;
	if (num == 1) {
		res = '\n';
		return res; 
	}
	int i = find_foo_index(str, num) - 1;
	while (str[i] == ' ') {
		i--;
	}
	if (str[i] == ';') {
		res = ';';
		return res;
	} else {
		res = '&';
		return res;
	}

}

int main(int argc, char* args[]) {
	if (args[1] == NULL) {
		printf("Error: Argument for file wasnt found\n");
		return 1;
	}

	// otevreni souboru
	FILE *fr = fopen(args[1], "r");
	if (fr == NULL) {
		printf("Error: File wasnt found\n");
		return 1;
	}
	int bsize = BUFFER_SIZE;
	char buffer[BUFFER_SIZE];
	// nacteni dat do bufferu
	while (fgets(buffer, BUFFER_SIZE, fr) != NULL) {
		if (!is_full_line(buffer, bsize)) {
			// mela byt realokace bufferu -> neimplementovano
			printf("Error: Line is too long\n");
			return -1;
		}
		// zjisti pocet funkci na radku
		int end = getc(fr);
		int count = count_foos(buffer);
		if (end == EOF) {
			printf("%d\n", count);
			count++;
		} else {
			ungetc(end, fr);
		}
		int pr = 0;

		// spustí bash funkce v novém procesu
		for (int i = 1; i <= count; i++) {
			char* foo = parse_foo(buffer, i);
			char** foo_args = parse_args(buffer, i);
			char sep = get_seperator(buffer, i);
			if (sep == '&') {
				if (pr == 0) pr = run_program(foo, foo_args);
				if (pr == -1) {
					printf("Error: Bash program failed"\n);
					return -2;
				}
			} else {
				pr = run_program(foo, foo_args);
				if (pr == -1) {
					printf("Error: Bash program failed\n");
					return -2;
				}
			}
			free(foo);
			free(foo_args);
		}
	} 
	
	if (fclose(fr)) {
		printf("Error: File cant be closed");
		return -1;
	}
	return 0;
}





