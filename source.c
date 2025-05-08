#include<stdio.h>
#include<unistd.h>
#include<pwd.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>




#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define HOST_NAME_MAX 64
#define PATH_MAX 1024
#define TOKEN_CNT 64

void print_prompt() {    //인터페이스 구현
	char hostname[HOST_NAME_MAX];
	char cwd[PATH_MAX];
	char* username;

	username = getpwuid(getuid())->pw_name;
	gethostname(hostname, sizeof(hostname));
	getcwd(cwd, sizeof(cwd));
	printf("%s@%s:%s$ ", username, hostname, cwd);



}
char** tokenize(char* line , int* token_count) //입력한 문자열을 공백단위로 나눠 토큰화 한 후 return하는 함수 -> 이러면 입력을 배열로 받아야 함.
{
	
	char** tokens = malloc(sizeof(char*) * MAX_TOKEN_SIZE);
	char buffer[MAX_TOKEN_SIZE];
	int tokenindex = 0, charindex = 0;

	for (int i = 0; ; i++) {
		char c = line[i];
		if (c == ' ' || c == '\n' || c == '\t' || c == '\0') {
			if (charindex > 0) {
					buffer[charindex] = '\0';
					tokens[tokenindex] = strdup(buffer);
					tokenindex++;
					charindex = 0;
				}
			if (c == '\0') break;
			}
		else {
				buffer[charindex++] = c;
			}
		}
	tokens[tokenindex] = NULL;
	*token_count = tokenindex;
	return tokens;
}






void pwd(void)
{


	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) != 0) {
		printf("%s\n", cwd);
	}
	else {
		printf("Error getting current directory\n");

	}
}
int cd(char* path) {
	if (path == NULL || strcmp(path, "") == 0) {
		path = getenv("HOME");
		if (path == NULL) {
			fprintf(stderr, "cd: HOME not set\n");
			return -1;
		}
	}

	if (chdir(path) != 0) {
		perror("cd");
		return -1;
	}

	return 0;
}



void pipee(char** token, int token_count, char* line) {
	int pipes[TOKEN_CNT][2] = { 0, };
	int pid = 0;
	int status;

	char** node_token[MAX_TOKEN_SIZE];
	int node = 0;
	int mi_token = 0;

	node_token[0] = malloc(sizeof(char*) * MAX_TOKEN_SIZE);
	for (int i = 0; token[i] != NULL; i++) {
		if (strcmp(token[i], "|") == 0) {
			node_token[node][mi_token] = NULL;
			node++;
			mi_token = 0;
			node_token[node] = malloc(sizeof(char*) * MAX_TOKEN_SIZE);
		}
		else {
			node_token[node][mi_token++] = token[i];
		}
	}
	node_token[node][mi_token] = NULL;

	// 첫 번째 명령어
	if (strcmp(node_token[0][0], "cd") == 0 || strcmp(node_token[0][0], "pwd") == 0) {
		if (strcmp(node_token[0][0], "cd") == 0) {
			if (node_token[0][1] == NULL) {
				cd(NULL);
			}
			else if (node_token[0][2] != NULL) {
				fprintf(stderr, "cd: too many arguments\n");
			}
			else {
				cd(node_token[0][1]);
			}
		}
		else {
			pwd();
		}
	}
	else {
		pipe(pipes[0]);
		pid = fork();

		if (pid < 0) exit(1);
		else if (pid == 0) {
			dup2(pipes[0][1], STDOUT_FILENO);
			close(pipes[0][0]);
			close(pipes[0][1]);
			execvp(node_token[0][0], node_token[0]);
			perror("execvp");
			exit(1);
		}
		close(pipes[0][1]);
		wait(&status);

		// 중간 명령어 처리
		for (int i = 0; i < node - 1; i++) {
			if (strcmp(node_token[i + 1][0], "cd") == 0 || strcmp(node_token[i + 1][0], "pwd") == 0) {
				if (strcmp(node_token[i + 1][0], "cd") == 0) {
					if (node_token[i + 1][1] == NULL) {
						cd(NULL);
					}
					else if (node_token[i + 1][2] != NULL) {
						fprintf(stderr, "cd: too many arguments\n");
					}
					else {
						cd(node_token[i + 1][1]);
					}
				}
				else {
					pwd();
				}
				continue;
			}

			pipe(pipes[i + 1]);
			pid = fork();
			if (pid < 0) exit(1);
			else if (pid == 0) {
				dup2(pipes[i][0], STDIN_FILENO);
				dup2(pipes[i + 1][1], STDOUT_FILENO);
				close(pipes[i][0]);
				close(pipes[i + 1][1]);
				execvp(node_token[i + 1][0], node_token[i + 1]);
				perror("execvp");
				exit(1);
			}
			close(pipes[i][0]);
			close(pipes[i + 1][1]);
			wait(&status);
		}

		// 마지막 명령어 처리
		if (strcmp(node_token[node][0], "cd") == 0 || strcmp(node_token[node][0], "pwd") == 0) {
			if (strcmp(node_token[node][0], "cd") == 0) {
				if (node_token[node][1] == NULL) {
					cd(NULL);
				}
				else if (node_token[node][2] != NULL) {
					fprintf(stderr, "cd: too many arguments\n");
				}
				else {
					cd(node_token[node][1]); 
				}
			}
			else {
				pwd();
			}
			return;
		}

		pid = fork();
		if (pid < 0) exit(1);
		else if (pid == 0) {
			dup2(pipes[node - 1][0], STDIN_FILENO);
			close(pipes[node - 1][0]);
			execvp(node_token[node][0], node_token[node]);
			perror("execvp");
			exit(1);
		}
		close(pipes[node - 1][0]);
		wait(&status);
	}

	// 메모리 해제
	for (int i = 0; i <= node; i++) {
		free(node_token[i]);
	}
	
}



void execc(char* line) {
	int token_count = 0;
	int pipp = 0;
	char** tokens = tokenize(line, &token_count);
	int pid;

	// 파이프 기호 탐색
	for (int i = 0; line[i] != '\0'; i++) {
		if (line[i] == '|') {
			pipp = 1;
			break;
		}
	}

	if (pipp) {
		pipee(tokens, token_count, line);
	}
	else {
		// 내부 명령어 처리
		if (strcmp(tokens[0], "cd") == 0 || strcmp(tokens[0], "pwd") == 0) {
			if (strcmp(tokens[0], "cd") == 0) {
				if (token_count == 1) {
					cd(NULL);
				}
				else if (token_count == 2) {
					cd(tokens[1]);
				}
				else {
					fprintf(stderr, "cd: too many arguments\n");
				}
			}
			else {
				pwd();
			}
		}
		else {
			pid = fork();
			if (pid < 0) {
				perror("fork 실패");
				return;
			}
			else if (pid == 0) {
				execvp(tokens[0], tokens);
				perror("execvp 실패");
				exit(1);
			}
			else {
				int status;
				waitpid(pid, &status, 0);
			}
		}
	}
	for (int i = 0; tokens[i] != NULL; i++) {
		free(tokens[i]);
	}
	free(tokens);

}





int main(int argc, char* argv[])
{
	char line[MAX_INPUT_SIZE];

	while (1) {
		print_prompt();
	
		char* result = fgets(line, sizeof(line), stdin);  
		if (result == NULL) {
			break;
		}
		line[strcspn(line, "\n")] = '\0';
		if (strlen(line) == 0) continue;
		if (strcmp(line, "exit") == 0) break;
		execc(line);
		printf("\n");

	}
	


}
