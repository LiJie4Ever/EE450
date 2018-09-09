//
// Created by Li Jie on 4/6/18.
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
#include <ctype.h>

#define AWSPORT "25981"   //aws TCP port
#define HOST "localhost"
#define MAXLENGTH 1024

//from Beej
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, char* argv[]) {

    char function[100];
    char word[100];
    char searchResult[MAXLENGTH];

    //get input from command line
    if (argc < 3) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    strcpy(function, argv[1]);
    strcpy(word, argv[2]);
    if (argv[3] != NULL) {
        strcat(word, " ");
        strcat(word, argv[3]);
    }


    //set up TCP --from Beej
    int sockfd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(HOST, AWSPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
            == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect. \n");
        exit(0);
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure
    printf("The client is up and running. \n");

    if ((numbytes = send(sockfd, function, sizeof function, 0)) == -1) {
        perror("send");
        exit(1);
    }

    if ((numbytes = send(sockfd, word, sizeof word, 0)) == -1) {
        perror("send");
        exit(1);
    }

    printf("The client sent <%s> and <%s> to AWS.\n", word, function);

    //search function
    if (strcmp(function, "search") == 0) {
        int flag;
        if ((numbytes = recv(sockfd, (char *)&flag, sizeof(flag), 0)) == -1) {
            perror("talker: recvfrom");
            exit(1);
        }

        if (flag) {
            if ((numbytes = recv(sockfd, searchResult, sizeof searchResult, 0)) == -1) {
                perror("send");
                exit(1);
            }

            char astr[MAXLENGTH];
            size_t length = (int) strlen(searchResult) - strlen(strstr(searchResult, " :: "));
            strncpy(astr, searchResult, length);
            printf("Found a match for < %s >:\n",astr);
            strcpy(astr, strstr(searchResult, ": "));
            char buf[MAXLENGTH];
            strcpy(buf, strstr(astr, " "));
            buf[strlen(buf) - 1] ='\0';
            printf("<%s>\n", buf);
        } else {
            printf("No match for <%s>\n", word);
        }
    }

    //prrefix function
    if (strcmp(function, "prefix") == 0) {
        int sum = 0;
        char result[MAXLENGTH];
        if ((numbytes = recv(sockfd, (char *)&sum, sizeof(sum), 0)) == -1) {
            perror("talker: recvfrom");
            exit(1);
        }
        printf("Found <%d> matches for <%s>:\n", sum, word);

        for (int i = 0; i < sum; ++i) {
            if ((numbytes = recv(sockfd, result, sizeof result, 0 )) == -1) {
                perror("talker: recvfrom");
                exit(1);
            }
            printf("%s\n", result);
        }
    }

    //suffix function
    if (strcmp(function, "suffix") == 0) {
        int sum = 0;
        char result[MAXLENGTH];
        if ((numbytes = recv(sockfd, (char *)&sum, sizeof(sum), 0)) == -1) {
            perror("talker: recvfrom");
            exit(1);
        }
        printf("Found <%d> matches for <%s>:\n", sum, word);

        for (int i = 0; i < sum; ++i) {
            if ((numbytes = recv(sockfd, result, sizeof result, 0 )) == -1) {
                perror("talker: recvfrom");
                exit(1);
            }
            printf("%s\n", result);
        }
    }
}