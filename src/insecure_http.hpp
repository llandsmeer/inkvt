/* inkvt - VT100 terminal for E-ink devices
 * Copyright (C) 2020 Lennart Landsmeer <lennart@landsmeer.email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h> 
#include <stdio.h> 
#include <errno.h>
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 

#include <vector>
#include <sstream>
#include <deque>

#define PORT 8080 

#define server_try(x) (err = (x), (err < 0? \
        (printf("ERROR: " #x " = %ld (errno = %s)\n", err, strerror(errno)), exit(1)) : (void)0), err)

struct Server {
    int fd;
    struct sockaddr_in address;

    struct pollfd get_pollfd() {
        pollfd ret;
        ret.fd = fd;
        ret.events = POLLIN;
        ret.revents = 0;
        return ret;
    }

    void setup(int port) {
        long err;
        int opt = 1;
        fd = server_try(socket(AF_INET, SOCK_STREAM, 0));
        server_try(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        server_try(bind(fd, (struct sockaddr *)&address, sizeof(address)));
        server_try(listen(fd, 3));
        int flags = server_try(fcntl(fd, F_GETFL, 0));
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    void accept(std::deque<char> & output) {
        // blocking read
        char response[] = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nok.\n";
        char buffer[1024] = {0};
        int clientfd;
        int addrlen = sizeof(address);
        clientfd = ::accept(fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (clientfd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            perror("accept");
            exit(1);
        }
        int nread = read(clientfd, buffer, sizeof(buffer)-1);
        if (nread <= 0) {
            close(clientfd);
            return;
        }
        std::stringstream ss;
        ss.write(buffer, nread);
        std::string header;
        buffer[nread] = 0;
        while (std::getline(ss, header)) {
            if (header.length() == 1) break;
        }
        char c;
        while (ss >> c) {
            output.push_back(c);
        }
        send(clientfd, response , sizeof(response)-1 , 0 );
        close(clientfd);
    }

};
