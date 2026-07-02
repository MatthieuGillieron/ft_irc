class Client 
{
    public:
        Client(int fd) : _fd(fd) {};

        int getFd() const { return _fd; }
        
    private:
        int _fd;
};