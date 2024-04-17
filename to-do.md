# ClientConnection.cpp:
    - cpnnect_TCP()
    - WaitForRequest()
        - comando port
        - comando pasv
        - comando stor
        - comando retr
        - comando list

# FTPServer.cpp:
    - define_socket_TCP()

int define_socket_TCP(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Failed to create socket");
        return -1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Failed to bind socket");
        close(sockfd);
        return -1;
    }
}

# ftp_server.cpp == main