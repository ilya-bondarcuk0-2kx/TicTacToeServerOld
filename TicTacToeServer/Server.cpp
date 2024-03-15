#include "Server.h"





std::mutex mtx;

Server::Server(const char* host, int port)
    :listening_socket(INVALID_SOCKET), host(host), port(port), conn_count(0), turn(1),active(false), hint(sockaddr_in()), current_side("Cross")
{


    for (int i = 0; i < 2; ++i) {
        clients.push_back(Client());
    }

}



bool Server::init_ws()
{

    WSAData wsData;

    if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0)
        return false;

    return true;
}

void Server::close_ws()
{
    closesocket(listening_socket);
    WSACleanup();
}


int Server::init()
{

    if ((listening_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
        return WSAGetLastError();

    // Make a struct than contains prefernces of connection, port, ip e.t.c.

    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, host, &hint.sin_addr);
    //bind listen socket
    if (bind(listening_socket, reinterpret_cast<sockaddr*>(&hint), sizeof(hint)) == SOCKET_ERROR)
        return WSAGetLastError();
    //set socket in listening mode
    if (listen(listening_socket, SOMAXCONN) == SOCKET_ERROR)
        return WSAGetLastError();

    active = true;
    return 0;
}


void Server::run()
{
    
    if (!init_ws())
        return;

    std::cout << init() << std::endl;


    admin_panel();
    connections_accept();
}


int Server::send_to_client(SOCKET& socket, std::string buffer)
{
    return send(socket, const_cast<char*>(buffer.c_str()), buffer.size() + 1, NULL);
}

int Server::send_to_client(SOCKET& socket, json& json)
{
    return send(socket, json.dump().c_str(), json.size(), NULL);
}

void Server::broadcast_to_clients(std::string buffer)
{

    mtx.lock();
    for (int i = 0; i < connections.size(); ++i) {
        send_to_client(connections[i], buffer + "\n");
    }
    mtx.unlock();

}

void Server::broadcast_to_clients(SOCKET& socket, std::string buffer)
{
    mtx.lock();
    for (int i = 0; i < connections.size(); ++i) {
        if (connections[i] == socket) {
            continue;
        }

        send_to_client(connections[i], buffer + "\n");
    }
    mtx.unlock();
}

int Server::receive_from_client(SOCKET& socket, std::string& buffer)
{
    return recv(socket, const_cast<char*>(buffer.c_str()), buffer.size() + 1, NULL);;
}

void Server::admin_panel()
{
    std::thread admin_thread([=]()
        {
            int a;
            std::cout << "Type anything for stop the server" << std::endl;
            std::cin >> a;
            active = false;
            std::this_thread::sleep_for(std::chrono::seconds(15));
            close_ws();

        });
    admin_thread.detach();
}

void Server::connections_accept()
{
    while (true)
    {
        SOCKET client_socket = INVALID_SOCKET;

        int size_addr = sizeof(hint);

        client_socket = accept(listening_socket, reinterpret_cast<sockaddr*>(&hint), &size_addr);

        

        

        if (!active.load())
            return;

        if (client_socket == INVALID_SOCKET) {
            closesocket(client_socket);
            continue;
        }
        connections.push_back(client_socket);

        std::cout << "Connected" << std::endl;

        

        std::string buf;
        buf.resize(20);
        if (receive_from_client(client_socket, buf) == 0)
            continue;
        bool is_side_different = false;
        Client tmp;
        buf.find("Cross") != std::string::npos ? tmp = Client("Cross") : tmp = Client("Zero");
        for (auto& client : clients)
        {
            if (client.get_side() == tmp.get_side()) {
                std::cout << "This side picked" << std::endl;
                break;
            }
            else
            {
                client.set_side(tmp.get_side());
                is_side_different = true;
                break;
            }
        }
        if (!is_side_different)
        {
            if (send_to_client(client_socket, "ERROR\n") == 0)  std::cout << "Choose side send error" << std::endl;
            continue;
        }
        if (send_to_client(client_socket, "OK\n") == 0) continue;


        ++conn_count;
        broadcast_to_clients(client_socket, "connected\n");

        auto client_func = [=]() mutable
            {

                while (true) {

                    

                    if (!active.load())
                        break;

                    std::string buffer;
                    buffer.resize(300);

                    if (receive_from_client(client_socket, buffer) == 0)
                        break;
                    
                    if (buffer.find("quit") != std::string::npos) {
                        broadcast_to_clients(client_socket, "disconnect");
                        std::cout << "quit" << std::endl;
                        break;
                    }

                    if (conn_count != 2)
                    {
                        send_to_client(client_socket, "wait\n");
                        std::cout << "Room is not full" << std::endl;
                        continue;
                    }


                    json response = json::parse(buffer);


                    if (response["side"] != current_side)
                    {
                        send_to_client(client_socket, "null\n");
                        continue;
                    }

                    response["turn"] = ++turn;

                    broadcast_to_clients(response.dump());


                    std::cout << response.dump() << std::endl;

                    mtx.lock();

                    if (current_side == "Cross")
                        current_side = "Zero";
                    else
                        current_side = "Cross";

                    mtx.unlock();

                }

   
                turn = 1;
                current_side = "Cross";
                --conn_count;
                refresh_clients();
                closesocket(client_socket);


            };

        std::thread client_thread([client_func]() mutable
            {
                client_func();
            });

        client_thread.detach();

    }
}

void Server::refresh_clients()
{
    mtx.lock();
   
    clients.clear();
    for (int i = 0; i < 2; ++i) {
        clients.push_back(Client());
    }
    std::cout << "Clients refresh" << std::endl;

    mtx.unlock();
}
