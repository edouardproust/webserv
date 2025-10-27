// (Este é o seu novo main.cpp)

#include "config/Config.hpp"     // Para criar o Config
#include "network/Network.hpp"   // Para criar o Network
#include "network/Colors.hpp"    // Para os logs de erro (FT_ERROR)
#include <iostream>

int main(int argc, char** argv) 
{
    // 1. Validar os argumentos de entrada
    if (argc != 2) 
    {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    try 
    {
        // 2. Criar o objeto Config
        // O construtor do Config(argv[1]) já faz o parse do ficheiro de configuração.
        Config config(argv[1]);

        // 3. Criar o objeto Network
        // Passamos 'config' por referência constante para o construtor do Network.
        // O construtor do Network já cria e faz bind/listen dos sockets.
        Network webserver(config);

        // 4. Iniciar o loop principal do servidor (epoll_wait, recv, send)
        // Esta função só retorna quando 'keep()' for falso (Ctrl+C).
        webserver.start_servers();

    } 
    catch (const std::exception& e) 
    {
        // Captura qualquer exceção lançada durante a inicialização ou o loop
        // (ex: falha no bind, epoll, new, etc.)
        std::cerr << FT_ERROR << e.what() << std::endl;
        return 1;
    }

    return 0;
}