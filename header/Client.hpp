
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>






class Client
{
    public:
        Client(int fd, bool pass = false, bool registred = false) : _fd(fd), _pass(pass), _registred(registred) {};
		~Client() {};



		// GETTER - SETTER
        int getFd() const { return _fd; }
		bool getPass() const { return _pass; }
		bool getRegistred() const { return _registred; }
		std::string getNickName() const { return _nickName; }
		std::string getUsrName() const { return _userName; }

		void setPass(bool pass) { _pass = pass; }
		void setRegistred(bool registred) { _registred = registred; }
		void setNickName(std::string nickName) { _nickName = nickName; }
		void setUserName(std::string userName) { _userName = userName; }


    private:
        int _fd;
		std::string _nickName;
		std::string _userName;
		bool _pass;
		bool _registred;
};





#endif