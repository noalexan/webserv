#include <iostream>
#include <curl/curl.h>

class Client
{
    public:
        Client(const std::string& serverUrl);
        ~Client();

        std::string makeRequest(const std::string& endpoint);

    private:
        std::string serverUrl;
};
