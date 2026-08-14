#ifndef COLORS_HPP
#define COLORS_HPP

# define RESET "\033[0m"
# define BLACK "\033[0;30m"
# define RED "\033[0;31m"
# define GREEN "\033[0;32m"
# define YELLOW "\033[0;33m"
# define BLUE "\033[0;34m"
# define PURPLE "\033[0;35m"
# define CYAN "\033[0;36m"
# define WHITE "\033[0;37m"
# define BOLD "\033[1m"
# define DIM "\033[2m"

# define C_UP     GREEN    // arrivee d'un client
# define C_DOWN   YELLOW   // depart d'un client
# define C_ERR    RED      // erreur
# define C_INFO   CYAN     // cycle de vie du serveur
# define C_DETAIL DIM      // details secondaires

#endif
