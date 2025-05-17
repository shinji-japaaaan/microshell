#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int ft_strlen(char *msg)
{
    int i = 0;

    while (msg && msg[i])
        i++;
    return (i);
}
void print_error(char *msg)
{
    write(2, msg, ft_strlen(msg));
}

int	ft_strcmp(const char *s1, const char *s2)
{
	while (*s1 || *s2)//*s1とせずにs1としてしまっていた
	{
		if ((unsigned)*s1 != (unsigned)*s2)//(unsigned *)sとしてしまっていた
			return ((unsigned)*s1 - (unsigned)*s2);
		s1++;
		s2++;
	}
	return (0);
}

char	**extract_args(int start, int end, char **av)
{
	int		count;
	char	**str;
	int		i = 0;//初期化漏れ

	count = end - start;
	str = malloc(sizeof(char *) * (count + 1));
	if (str == NULL)
	{
		print_error("error:fatal\n");//faral spellミス
		return (NULL);
	}
	while (start < end)
		str[i++] = av[start++];
	str[i] = NULL;
	return (str);
}

int	is_pipe(char *s)
{
	if (s == NULL)
		return (0);
	if (ft_strcmp(s, "|") == 0)
		return (1);
	return (0);
}

int	is_break(char *s)
{
	if (s == NULL)//s=としてしまっていた箇所が複数
		return (1);
	if (ft_strcmp(s, ";") == 0)//== 0をつけていなかった
		return (1);
	if (is_pipe(s))
		return (1);
	return (0);
}

int	exec_cd(char **args)
{
	if (args[1] == NULL || args[2] != NULL)//==ではなく=としてしまっていた
	{
		print_error("error: cd: bad arguments\n");
		return (1);
	}
	if (chdir(args[1]) == -1)
	{
		print_error("error: cd: cannot change directory to ");
		print_error(args[1]);
		print_error("\n");
		return (1);
	}
	return (0);
}

void exec_cmd(char **args, char **env, int fd, int *pipe_fd)
{
    if (dup2(fd, 0) == -1)
    {
        print_error("error:fatal\n");
        if (fd != 0)
        	close(fd);//ここのclose
        if (pipe_fd)
        {
            close(pipe_fd[0]);//ここのclose
            close(pipe_fd[1]);//ここのclose
        }
        exit(1);
    }
    if (fd != 0)
        close(fd);//ここのclose
    if (pipe_fd)
    {
        if (dup2(pipe_fd[1], 1) == -1)
        {
            print_error("error:fatal\n");
            close(pipe_fd[0]);//ここのclose
            close(pipe_fd[1]);//ここのclose
            exit(1);
        }
        close(pipe_fd[0]);//ここのclose
        close(pipe_fd[1]);//ここのclose
    }
    execve(args[0], args, env);
    // free(args);//executeが失敗したとき、freeをする
    print_error("error: cannot execute ");//エラー表示がスペースは言っていないとかでもＮＧとなってしまうので注意
    print_error(args[0]);
    print_error("\n");
    exit(1);    
}

int	exec_pipeline(int fd, int pipe_flag, char **args, char **env)
{
	int	pipe_fd[2];
	int	pid;

	if (pipe_flag)
	{
		if (pipe(pipe_fd) == -1)
		{
			print_error("error:fatal\n");
			return (-1);
		}
	}
	pid = fork();
	if (pid == -1)
	{
		print_error("error:fatal\n");
		if (pipe_flag) 
		{
			close(pipe_fd[0]);
			close(pipe_fd[1]);//ここはかなり見逃しやすい
		}
		return (-1);
	}
	if (pid == 0)
	{
		if (pipe_flag)
			exec_cmd(args, env, fd, pipe_fd);
		else
			exec_cmd(args, env, fd, NULL);
	}
	waitpid(pid, NULL, 0);
	if (pipe_flag)
    {
        close(pipe_fd[1]);
        return (pipe_fd[0]);
    }
    else
    {
        return (0);
    }
}

int	main(int ac, char **av, char **env)
{
	int i = 1;// i = 0としてしまっていた
	int end = 0;
	int fd = 0;
	int pipe_flag = 0;
	char **args;

	while (i < ac)//i < endとしてしまっていた
	{
		end = i; 
        while (!is_break(av[end]))
			end++;
		pipe_flag = is_pipe(av[end]);
		args = extract_args(i, end, av);
		if (!args)
        {
            if (fd != 0)
                close(fd);
            exit(1);
        }
		if (args[0] && ft_strcmp(args[0], "cd") == 0)
			exec_cd(args);
		else if (args[0])
		{
			int new_fd = exec_pipeline(fd, pipe_flag, args, env);
			if (new_fd == -1)
			{
				// fdは閉じるが終了はしない
				if (fd != 0)
        			close(fd);
				fd = 0;  // 入力は標準入力に戻す
			}
			else
			{
				if (fd != 0)
            		close(fd);
				fd = new_fd;
			}
		}
		free(args);
		i = end + 1;
	}
	close(fd);//ここも気を付ける
	return (0);
}
