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

#define AWSPORT "26981"
#define HOST "localhost"
#define MAXLENGTH 1024

//from Beej
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, const char * argv[]) {
    char searchResult_Perfect[MAXLENGTH];
    char searchResult_Edit[MAXLENGTH];
    char function[100];
    char word[100];

    //set up TCP --from Beej
    int sockfd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

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
    printf("The Monitor is up and running. \n");

    while (1) {
        if ((numbytes = recv(sockfd, function, sizeof function, 0)) == -1) {
            perror("talker: recvfrom");
            exit(1);
        }

        if ((numbytes = recv(sockfd, word, sizeof word, 0)) == -1) {
            perror("talker: recvfrom");
            exit(1);
        }

        //search function
        if (strcmp(function, "search") == 0) {
            memset(searchResult_Edit, '\0', MAXLENGTH);
            memset(searchResult_Perfect, '\0', MAXLENGTH);
            int total_num = 0;

            int flag;
            if ((numbytes = recv(sockfd, (char *) &flag, sizeof(flag), 0)) == -1) {
                perror("talker: recvfrom");
                exit(1);
            }

            if (flag) {
                if ((numbytes = recv(sockfd, searchResult_Perfect, sizeof searchResult_Perfect, 0)) == -1) {
                    perror("send");
                    exit(1);
                }

                if ((numbytes = recv(sockfd, (char *) &total_num, sizeof(total_num), 0)) == -1) {
                    perror("talker: recvfrom");
                    exit(1);
                }
                if (total_num != 0) {
                    if ((numbytes = recv(sockfd, searchResult_Edit, sizeof(searchResult_Edit), 0)) == -1) {
                        perror("send");
                        exit(0);
                    }
                } else {
                    printf("No match for one edit distance of <%s>\n", word);
                }
                char astr[MAXLENGTH];
                size_t length = (int) strlen(searchResult_Perfect) - strlen(strstr(searchResult_Perfect, " :: "));
                strncpy(astr, searchResult_Perfect, length);
                printf("Found a match for < %s >:\n", astr);
                strcpy(astr, strstr(searchResult_Perfect, ": "));
                char buf[MAXLENGTH];
                strcpy(buf, strstr(astr, " "));
                buf[strlen(buf) - 1] = '\0';
                printf("<%s>\n", buf);

                if (total_num != 0) {
                    memset(astr, '\0', MAXLENGTH);
                    length = (int) strlen(searchResult_Edit) - strlen(strstr(searchResult_Edit, " :: "));
                    strncpy(astr, searchResult_Edit, length);
                    printf("One edit distance match is <%s>:\n", astr);
                    strcpy(astr, strstr(searchResult_Edit, ": "));
                    memset(buf, '\0', MAXLENGTH);
                    strcpy(buf, strstr(astr, " "));
                    buf[strlen(buf) - 1] = '\0';
                    printf("<%s>\n", buf);
                }
                memset(astr, '\0', MAXLENGTH);
            } else {
                printf("No match for <%s>\n", word);
            }
        }

        //prefix function
            if (strcmp(function, "prefix") == 0) {
                int sum = 0;
                char result[MAXLENGTH];
                if ((numbytes = recv(sockfd, (char *) &sum, sizeof(sum), 0)) == -1) {
                    perror("talker: recvfrom");
                    exit(1);
                }
                printf("Found <%d> matches for <%s>:\n", sum, word);

                for (int i = 0; i < sum; ++i) {
                    if ((numbytes = recv(sockfd, result, sizeof result, 0)) == -1) {
                        perror("talker: recvfrom");
                        exit(1);
                    }
                    printf("%s\n", result);
                }
            }

            //sufix ffuction
            if (strcmp(function, "suffix") == 0) {
                int sum = 0;
                char result[MAXLENGTH];
                if ((numbytes = recv(sockfd, (char *) &sum, sizeof(sum), 0)) == -1) {
                    perror("talker: recvfrom");
                    exit(1);
                }
                printf("Found <%d> matches for <%s>:\n", sum, word);

                for (int i = 0; i < sum; ++i) {
                    if ((numbytes = recv(sockfd, result, sizeof result, 0)) == -1) {
                        perror("talker: recvfrom");
                        exit(1);
                    }
                    printf("%s\n", result);
                }
            }
        }
    }