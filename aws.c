//
// Created by Li Jie on 4/2/18.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define UDPPORT_A "21981"       // UDP port of A
#define UDPPORT_B "22981"      // UDP port of B
#define UDPPORT_C "23981"    // UDP port of C
#define UDPPORT "24981"     // UDP port of aws
#define TCPPORT_CLIENT "25981"      // TCP with client
#define TCPPORT_MONITOR "26981"     // TCP with monitor
#define HOST "localhost"   // use local IP
#define BACKLOG 10      // how many pending connections queue will hold
#define MAXBUFLEN 1024

//search results from backend server
struct search_result {
    char perfect[MAXBUFLEN];
    char edit[MAXBUFLEN];
    int num;
};

//serch Info from client
struct search_Info {
    char function[100];
    char word[100];
};

struct search_result sr;
struct search_Info si;

char* prefix_result[MAXBUFLEN];
char* suffix_result[MAXBUFLEN];

int total_num_pre;
int total_num_suf;

//from Beej
void sigchld_handler(int s) {
    // from Beej
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

//from Beej
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// using UDP to get the info from the backend server
int UDPConnection(char function[], char world[], char ch) {
    //set up UDP from Beej
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char* UDPPort;
    int numbytes;
    char result[4 * MAXBUFLEN];


    if (ch == 'A') {
        UDPPort = UDPPORT_A;
    }
    else if (ch == 'B') {
        UDPPort = UDPPORT_B;
    }
    else if (ch == 'C') {
        UDPPort = UDPPORT_C;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(HOST, UDPPort, &hints, &servinfo))
        != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
            == -1) {
            perror("talker: socket");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
    }


    if ((numbytes = sendto(sockfd, world, MAXBUFLEN, 0, p->ai_addr,p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    if ((numbytes = sendto(sockfd, function, MAXBUFLEN, 0, p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    memset(result,0,sizeof(result));

    //function is search
    if (strcmp(function, "search") == 0) {
        //get a struct contains all the infomation
        if ((numbytes = recvfrom(sockfd, result, sizeof result, 0 , NULL, NULL)) == -1) {
            perror("talker: recvfrom");
            exit(1);
        }
        memset(sr.perfect, '\0', sizeof(sr.perfect));
        memset(sr.edit, '\0', sizeof(sr.perfect));
        sr.num = 0;
        memcpy(&sr, result, sizeof(sr));
        printf("The AWS received <%d> similar words from Backend-Server <%c> using UDP over port <%s>\n", sr.num, ch, UDPPort);
    }

    //function is prefix
    if (strcmp(function, "prefix") == 0) {
        int num = 0;
        //get the number of the similar words
        if ((numbytes = recvfrom(sockfd, (char *)&num, sizeof num, 0 , NULL, NULL)) == -1) {
            perror("talker: recvfrom");
            exit(1);
        }
        //get the silmilar wrods in a loop
        for (int i = total_num_pre; i < num + total_num_pre; ++i) {
            if ((numbytes = recvfrom(sockfd, result, sizeof result, 0 , NULL, NULL)) == -1) {
                perror("talker: recvfrom");
                exit(1);
            }
            prefix_result[i] = strdup(result);
        }
        total_num_pre = total_num_pre + num;
        printf("The AWS received <%d> matches from Backend-Server <%c> using UDP over port <%s>\n", num, ch, UDPPort);

    }

    //function is suffix
    if (strcmp(function, "suffix") == 0) {
        int num = 0;
        //get the number of the similar words
        if ((numbytes = recvfrom(sockfd, (char *)&num, sizeof num, 0 , NULL, NULL)) == -1) {
            perror("talker: recvfrom");
            exit(1);
        }

        //get the words in a loop
        for (int i = total_num_suf; i < num + total_num_suf; ++i) {
            if ((numbytes = recvfrom(sockfd, result, sizeof result, 0 , NULL, NULL)) == -1) {
                perror("talker: recvfrom");
                exit(1);
            }
            suffix_result[i] = strdup(result);
        }
        total_num_suf = total_num_suf + num;
        printf("The AWS received <%d> matches from Backend-Server <%c> using UDP over port <%s>\n", num, ch, UDPPort);

    }
    return 0;
}

//Create TCP with client and monitor and transfer data
int run() {

    //set up TCP --from Beej
    int sockfd_C, new_fd_C, sockfd_M, new_fd_M;  // listen on sock_fd, new connection on new_fd, client and monitor
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    int yes=1;
    int rv;
    int numbytes;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP


    //create a parent socket with Clientn
    if ((rv = getaddrinfo(HOST, TCPPORT_CLIENT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd_C = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
            == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd_C, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
            == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd_C, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd_C);
            perror("server: bind");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }
    // all done with this structure
    freeaddrinfo(servinfo);

    if (listen(sockfd_C, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    //Create a parent socket with monitor
    if ((rv = getaddrinfo(HOST, TCPPORT_MONITOR, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd_M = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
            == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd_M, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
            == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd_M, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd_M);
            perror("server: bind");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }
    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd_M, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf( "The AWS is up and running.\n");

    //keep accepting monitor requiring
    sin_size = sizeof their_addr;
    new_fd_M = accept(sockfd_M, (struct sockaddr *) &their_addr, &sin_size);
    if (new_fd_M == -1) {
                perror("accept");
                exit(1);
    }

    //keep handling client query
        while (1) {
            //Client link to the aws and aws accepts it
            sin_size = sizeof their_addr;
            new_fd_C = accept(sockfd_C, (struct sockaddr *) &their_addr, &sin_size);
            if (new_fd_C == -1) {
                perror("accept");
                exit(1);
            }


            memset(&si, '0', sizeof(si));

            //get the name of function from the client
            if ((numbytes = recv(new_fd_C, si.function, sizeof(si.function), 0)) == -1) {
                perror("recv");
                exit(1);
            }

            //send the function name to the monitor
            if ((numbytes = send(new_fd_M, si.function, sizeof(si.function), 0)) == -1) {
                perror("recv");
                exit(1);
            }

            //get the word from the client
            if ((numbytes = recv(new_fd_C, si.word, sizeof(si.word), 0)) == -1) {
                perror("recv");
                exit(1);
            }

            //send the word to the monitor
            if ((numbytes = send(new_fd_M, si.word, sizeof(si.word), 0)) == -1) {
                perror("recv");
                exit(1);
            }

            printf("The AWS received input=<%s> and function=<%s> from the client using TCP over port <%s>\n", si.word, si.function, TCPPORT_CLIENT);

            //search function
            if (strcmp(si.function, "search") == 0) {
                //perform the UDP Connection
                printf("The AWS sent <%s> and <%s> to Backend-Server <%c> using UDP over port <%s>\n", si.word, si.function, 'A', UDPPORT);
                printf("The AWS sent <%s> and <%s> to Backend-Server <%c> using UDP over port <%s>\n", si.word, si.function, 'B', UDPPORT);
                printf("The AWS sent <%s> and <%s> to Backend-Server <%c> using UDP over port <%s>\n", si.word, si.function, 'C', UDPPORT);
                int flag = 0;
                UDPConnection(si.function, si.word, 'A');

                char temp_pre[MAXBUFLEN];
                char temp_suf[MAXBUFLEN];
                int tempNum = 0;

                if (sr.perfect[0] != '\0') {
                    strncpy(temp_pre, sr.perfect, MAXBUFLEN);
                    flag = 1;
                }
                if (sr.num != 0) {
                    strncpy(temp_suf, sr.edit, MAXBUFLEN);
                    tempNum = sr.num;
                }
                UDPConnection(si.function, si.word, 'B');

                if (sr.perfect[0] != '\0') {
                    strncpy(temp_pre, sr.perfect, MAXBUFLEN);
                    flag = 1;
                }
                if (sr.num != 0) {
                    strncpy(temp_suf, sr.edit, MAXBUFLEN);
                    tempNum = sr.num;
                }

                UDPConnection(si.function, si.word, 'C');
                if (sr.perfect[0] != '\0') {
                    strncpy(temp_pre, sr.perfect, MAXBUFLEN);
                    flag = 1;
                }
                if (sr.num != 0) {
                    strncpy(temp_suf, sr.edit, MAXBUFLEN);
                    tempNum = sr.num;
                }

                // to remind that there may be no match
                if ((numbytes = send(new_fd_C, (char *)&flag, sizeof(flag), 0)) == -1) {
                    perror("recv");
                    exit(1);
                }

                if ((numbytes = send(new_fd_M, (char *)&flag, sizeof(flag), 0)) == -1) {
                    perror("recv");
                    exit(1);
                }

                //If the result
                if (flag) {
                    if ((numbytes = send(new_fd_C, temp_pre, sizeof(temp_pre), 0)) == -1) {
                        perror("recv");
                        exit(1);
                    }
                    printf("The AWS sent < 1 > matches to client.\n");

                    if ((numbytes = send(new_fd_M, temp_pre, sizeof(temp_pre), 0)) == -1) {
                        perror("recv");
                        exit(1);
                    }

                    if ((numbytes = send(new_fd_M, (char *)&tempNum, sizeof(tempNum), 0)) == -1) {
                        perror("recv");
                        exit(1);
                    }

                    if (temp_suf[0] != '\0') {
                        if ((numbytes = send(new_fd_M, temp_suf, sizeof(temp_suf), 0)) == -1) {
                            perror("recv");
                            exit(1);
                        }
                        char pstr[MAXBUFLEN];
                        memset(pstr, '\0', MAXBUFLEN);
                        size_t length1 = (int) strlen(temp_pre) - strlen(strstr(temp_pre, " :: "));
                        strncpy(pstr, temp_pre, length1);
                        char estr[MAXBUFLEN];
                        memset(estr, '\0', MAXBUFLEN);
                        size_t length2 = (int) strlen(temp_suf) - strlen(strstr(temp_suf, " :: "));
                        strncpy(estr, temp_suf, length2);
                        printf("The AWS sent <%s> and <%s> to the monitor via TCP port <%s>.\n", pstr, estr, TCPPORT_MONITOR);
                        memset(temp_suf, '\0', MAXBUFLEN);
                    }

                } else {
                    printf("The AWS sent < 0 > matches to client.\n");
                    printf("The AWS sent < 0 > matches to monitor.\n");
                }


            }

            //prefix function
            if (strcmp(si.function, "prefix") == 0) {
                total_num_pre = 0;
                printf("The AWS sent <%s> and <%s> to Backend-Server <%c> using UDP over port <%s>\n\n", si.word, si.function, 'A', UDPPORT);
                printf("The AWS sent <%s> and <%s> to Backend-Server <%c> using UDP over port <%s>\n\n", si.word, si.function, 'B', UDPPORT);
                printf("The AWS sent <%s> and <%s> to Backend-Server <%c> using UDP over port <%s>\n\n", si.word, si.function, 'C', UDPPORT);

                //perform the UDP connection
                UDPConnection(si.function, si.word, 'A');
                UDPConnection(si.function, si.word, 'B');
                UDPConnection(si.function, si.word, 'C');

                if ((numbytes = send(new_fd_C, (const char*)&total_num_pre, sizeof(total_num_pre), 0)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }

                for (int j = 0; j < total_num_pre; ++j) {
                    if ((numbytes = send(new_fd_C, prefix_result[j], MAXBUFLEN, 0)) == -1) {
                        perror("talker: sendto");
                        exit(1);
                    }
                }
                printf("The AWS sent < %d > matches to client.\n", total_num_pre);

                if ((numbytes = send(new_fd_M, (const char*)&total_num_pre, sizeof(total_num_pre), 0)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }

                for (int j = 0; j < total_num_pre; ++j) {
                    if ((numbytes = send(new_fd_M, prefix_result[j], MAXBUFLEN, 0)) == -1) {
                        perror("talker: sendto");
                        exit(1);
                    }
                }

            }

            //suffix function
            if (strcmp(si.function, "suffix") == 0) {
                total_num_suf = 0;
                printf("The AWS sent <%s> and <%s> to Backend-Server <%c> using UDP over port <%s>\n\n", si.word, si.function, 'A', UDPPORT);
                printf("The AWS sent <%s> and <%s> to Backend-Server <%c> using UDP over port <%s>\n\n", si.word, si.function, 'A', UDPPORT);
                printf("The AWS sent <%s> and <%s> to Backend-Server <%c> using UDP over port <%s>\n\n", si.word, si.function, 'A', UDPPORT);

                //perform the UDP connection
                UDPConnection(si.function, si.word, 'A');
                UDPConnection(si.function, si.word, 'B');
                UDPConnection(si.function, si.word, 'C');

                if ((numbytes = send(new_fd_C, (const char*)&total_num_suf, sizeof(total_num_suf), 0)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }

                for (int j = 0; j < total_num_suf; ++j) {
                    if ((numbytes = send(new_fd_C, suffix_result[j], MAXBUFLEN, 0)) == -1) {
                        perror("talker: sendto");
                        exit(1);
                    }
                }
                printf("The AWS sent < %d > matches to client.\n", total_num_suf);

                if ((numbytes = send(new_fd_M, (const char*)&total_num_suf, sizeof(total_num_suf), 0)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }

                for (int j = 0; j < total_num_suf; ++j) {
                    if ((numbytes = send(new_fd_M, suffix_result[j], MAXBUFLEN, 0)) == -1) {
                        perror("talker: sendto");
                        exit(1);
                    }
                }
            }
    }

}

int main() {

    //run the programmes
    run();
    return 0;
}





