#include "application.hpp"


int main(int argc, char *argv[])
{
    Erp::Client::Application a(argc, argv);
    
    return a.run(argc, argv);
}


