#include "../header/Server.hpp"

// volatile : le handler la modifie a tout instant, le compilateur ne doit pas
// en garder une copie en registre dans la boucle principale.
volatile sig_atomic_t g_shutdown = 0;

// Un handler de signal ne peut rien faire d'autre sans risque : il s'execute a
// un moment arbitraire, y compris au milieu d'une allocation.
void signalHandler(int signal)
{
	(void)signal;
	g_shutdown = 1;
}

// Le try/catch garantit ce qu'exige le sujet : le programme ne quitte jamais de
// facon inattendue, meme a court de memoire ou un new echouerait.
int main(int ac, char **av)
{
	if (ac != 3)
	{
		std::cout << C_ERR << "Usage: ./ircserv <port> <password>" << RESET << std::endl;
		return 1;
	}

	unsigned int port = std::atoi(av[1]);
	if (port < 1024 || port > 65535)
	{
		std::cout << C_ERR << "Error: port must be between 1024 and 65535" << RESET << std::endl;
		return 1;
	}

	std::string password(av[2]);
	if (password.empty())
	{
		std::cout << C_ERR << "Error: password cannot be empty" << RESET << std::endl;
		return 1;
	}

	std::cout << C_INFO << BOLD << "[*] ircserv listening on port " << port
			  << RESET << std::endl;

	signal(SIGINT, signalHandler);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGQUIT, signalHandler);

	try
	{
		Server server(port, password);
		if (!server.run())
			return 1;
	}
	catch (const std::exception &e)
	{
		std::cerr << C_ERR << "[!] Fatal: " << e.what() << RESET << std::endl;
		return 1;
	}

	return 0;
}
