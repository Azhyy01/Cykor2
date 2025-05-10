#include<stdio.h>
#include<unistd.h>
#include<pwd.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<signal.h>



#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define HOST_NAME_MAX 64
#define PATH_MAX 1024
#define TOKEN_CNT 64
int job_num = 0; // &명령어 실행시 출력될 job_number
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
	static char prev_cwd[PATH_MAX] = "";
	char curr_path[PATH_MAX];
	// 현재 경로 저장
	if (getcwd(curr_path, sizeof(curr_path)) == NULL) {
		perror("getcwd");
		return -1;
	}

	
	if (path == NULL || strcmp(path, "") == 0 || strcmp(path, "~") == 0) {  //cd, cd ~ -> 홈 디렉토리로 이동하는 명령어 구현
		path = getenv("HOME");
		if (path == NULL) {
			fprintf(stderr, "cd: HOME not set\n");
			return -1;
		}
	}
	//cd - 구현
	else if (strcmp(path, "-") == 0)
	{

		if (strlen(prev_cwd) == 0)
		{
			fprintf(stderr, "cd: OLDPWD not set\n");
			return -1;
		}
		path = prev_cwd;
		printf("%s\n", path);
	}



	if (chdir(path) != 0) {
		perror("cd");
		return -1;
	}

	strcpy(prev_cwd, curr_path); //다음 cd호출 때는 curr_path가 이전 경로가 되기 때문에 갱신
	return 0;
}

void multi1 (char* line, char** token, int token_count) { // 다중 명령어 ; 처리
	char** node_token[MAX_TOKEN_SIZE];
	int node = 0; 
	int mi_token = 0;
	int pid = 0;  //fork() 사용 위함
	node_token[0] = malloc(sizeof(char*) * MAX_TOKEN_SIZE);
	for (int i = 0; token[i] != NULL; i++) {
		if (strcmp(token[i], ";") == 0) {
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
	for (int i = 0; i <= node; i++)
	{
		pid = fork();   //부모 프로세스를 자식 프로세스로 대체해야 반복문 계속 진행
		if (pid == 0) {
			execvp(node_token[i][0], node_token[i]);
			perror("execvp fail");
			exit(1);
		}
		else {
			wait(NULL);
		}

	}

	for (int i = 0; i <= node; i++) {
		free(node_token[i]);
	}
	return;
}

void multi2(char* line, char** token, int token_count) {  //다중명령어 && 처리
	char** node_token[MAX_TOKEN_SIZE];
	int node = 0, mi_token = 0;
	node_token[0] = malloc(sizeof(char*) * MAX_TOKEN_SIZE);

	for (int i = 0; token[i] != NULL; i++) {
		if (strcmp(token[i], "&&") == 0) {
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

	for (int i = 0; i <= node; i++) {
		int pid = fork();
		if (pid == 0) {
			execvp(node_token[i][0], node_token[i]);
			perror("execvp fail");
			exit(1);
		}
		else {
			int status;
			waitpid(pid, &status, 0);
			// 앞 명령이 실패하면 다음 명령어는 실행 안 함
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) //부모가 자식의 종료상태 체크
				break;
		}
	}

	for (int i = 0; i <= node; i++)
		free(node_token[i]);
}

void multi3(char* line, char** token, int token_count) { //다중 명령어 || 처리
	char** node_token[MAX_TOKEN_SIZE];
	int node = 0, mi_token = 0;
	node_token[0] = malloc(sizeof(char*) * MAX_TOKEN_SIZE);

	for (int i = 0; token[i] != NULL; i++) {
		if (strcmp(token[i], "||") == 0) {
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

	for (int i = 0; i <= node; i++) {
		int pid = fork();
		if (pid == 0) {
			execvp(node_token[i][0], node_token[i]);
			perror("execvp fail");
			exit(1);
		}
		else {
			int status;
			waitpid(pid, &status, 0);
			// 앞 명령이 성공하면 다음 명령어는 실행 안 함
			if (!WIFEXITED(status) || WEXITSTATUS(status) == 0)
				break;
		}
	}

	for (int i = 0; i <= node; i++)
		free(node_token[i]);
}

void multi4(char* line, char** token, int token_count) {  //다중명령어 & 처리
	char** node_token[MAX_TOKEN_SIZE];
	
	int node = 0;
	int mi_token = 0;
	node_token[0] = malloc(sizeof(char*) * MAX_TOKEN_SIZE);
	for (int i = 0; token[i] != NULL; i++) {
		if (strcmp(token[i], "&") == 0) {
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
	for (int i = 0; i <= node; i++)
	{
		int pid = fork();
		if (pid == 0)
		{
			

			execvp(node_token[i][0], node_token[i]);
			perror("excvp fail");
			exit(1);
		}

	}
	for (int i = 0; i <= node; i++) {
		free(node_token[i]);
	}
	return;
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
	//다중 명령어 기호 탐색
	for (int i = 0; line[i] != '\0'; i++)
	{
		if (line[i] == '&' && line[i + 1] == '&') {
			multi2(line, tokens, token_count);
			return;
		}
		else if (line[i] == '|' && line[i + 1] == '|') {
			multi3(line, tokens, token_count);
			return;
		}
		else if (line[i] == ';')
		{
			multi1(line, tokens, token_count);
			return;
		}
		else if (line[i] == '&')
		{
			multi4(line, tokens, token_count);
			
			
			return;
		}
	}
	
	

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
				perror("fork fail");
				return;
			}
			else if (pid == 0) {
				execvp(tokens[0], tokens);
				perror("execvp fail");
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
		if (strcmp(line, "exit") == 0) break; //exit 기능 구현
		execc(line);
		

	}
	


}
