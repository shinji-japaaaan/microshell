#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

int	ft_strlen(char *s)
{
	int len = 0;
	while (s && s[len])
		len++;
	return (len);
}

void	print_error(char *msg)
{
	write(2, msg, ft_strlen(msg));
}

char	**extract_args(char **av, int start, int end)
{
	int		count = end - start;
	char	**args = malloc(sizeof(char *) * (count + 1));
	int		i = 0;

	if (!args)
	{
		print_error("error: fatal\n");
		exit(1);
	}
	while (start < end)
		args[i++] = av[start++];
	args[i] = NULL;
	return (args);
}

void	exec_cmd(char **args, char **env, int in_fd, int *pipe_fd)
{
	if (dup2(in_fd, 0) == -1)
	{
		print_error("error: fatal\n");
		exit(1);
	}
	if (pipe_fd != NULL)
	{
		if (dup2(pipe_fd[1], 1) == -1)
		{
			print_error("error: fatal\n");
			exit(1);
		}
		close(pipe_fd[0]);
		close(pipe_fd[1]);
	}
	execve(args[0], args, env);
	print_error("error: cannot execute ");
	print_error(args[0]);
	print_error("\n");
	exit(1);
}

int	exec_pipeline(char **args, char **env, int in_fd, int need_pipe)
{
	int pid;
	int pipe_fd[2];

	if (need_pipe)
	{
		if (pipe(pipe_fd) == -1)
		{
			print_error("error: fatal\n");
			return (1);
		}
	}
	pid = fork();
	if (pid == -1)
	{
		print_error("error: fatal\n");
		return (1);
	}
	if (pid == 0)
	{
		if (need_pipe)
			exec_cmd(args, env, in_fd, pipe_fd);
		else
			exec_cmd(args, env, in_fd, NULL);
	}
	waitpid(pid, NULL, 0);
	close(in_fd);
	if (need_pipe)
	{
		close(pipe_fd[1]);
		return (pipe_fd[0]);
	}
	return (dup(0));
}

int	exec_cd(char **args)
{
	if (args[1] == NULL || args[2] != NULL)
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

int	ft_strcmp(const char *str1, const char *str2)
{
	while (*str1 || *str2)
	{
		if ((unsigned char)*str1 != (unsigned char)*str2)
			return ((unsigned char)*str1 - (unsigned char)*str2);
		str1++;
		str2++;
	}
	return (0);
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
	if (s == NULL)
		return (1);
	if (ft_strcmp(s, ";") == 0)
		return (1);
	if (is_pipe(s))
		return (1);
	return (0);
}

int	main(int ac, char **av, char **env)
{
	int i = 1;
	int in_fd = dup(0);
	int end, pipe_flag;
	char **args;

	while (i < ac)
	{
		end = i;
		while (!is_break(av[end]))
			end++;

		pipe_flag = is_pipe(av[end]);
		args = extract_args(av, i, end);

		if (args[0] && ft_strcmp(args[0], "cd") == 0)
		{
			if (exec_cd(args) != 0)
			{
				free(args);
				return (1);
			}
		}
		else if (args[0])
		{
			in_fd = exec_pipeline(args, env, in_fd, pipe_flag);
		}
		free(args);
		i = end + 1;
	}
	close(in_fd);
	return (0);
}
