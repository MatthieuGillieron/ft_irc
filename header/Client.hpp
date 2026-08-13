
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>


class Client
{
    public:
        Client(int fd, bool pass = false, bool registred = false) : _fd(fd), _pass(pass), _registred(registred), _quitting(false) {};
		~Client() {};


        int getFd() const { return _fd; }
		bool getPass() const { return _pass; }
		bool getRegistred() const { return _registred; }
		// le client doit partir des que son outBuffer est vide (mauvais mot de passe, QUIT...)
		bool isQuitting() const { return _quitting; }
		std::string getNickName() const { return _nickName; }
		std::string getUsrName() const { return _userName; }
        std::string getInBuffer() const { return _inBuffer; }
		std::string getOutBuffer() const { return _outBuffer; }

		void setPass(bool pass) { _pass = pass; }
		void setRegistred(bool registred) { _registred = registred; }
		void setQuitting(bool quitting) { _quitting = quitting; }
		void setNickName(std::string nickName) { _nickName = nickName; }
		void setUserName(std::string userName) { _userName = userName; }

        void setInBuffer(std::string inBuffer) { _inBuffer = inBuffer; }
        void appendToBuffer(std::string data) { _inBuffer += data; }
        void eraseBuffer(size_t pos, size_t len) { _inBuffer.erase(pos, len); }
		
		void setOutBuffer(std::string outBuffer) { _outBuffer = outBuffer; }
		void appendToOutBuffer(std::string data) { _outBuffer += data; }
		void eraseOutBuffer(size_t pos, size_t len) { _outBuffer.erase(pos, len); }

    private:
        int _fd;
		std::string _nickName;
		std::string _userName;
		bool _pass;
		bool _registred;
        std::string _inBuffer;
		std::string _outBuffer;
		bool _quitting;
};


#endif
