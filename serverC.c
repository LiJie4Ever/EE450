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


#define MYPORT "23981"
#define HOST "localhost"
#define MAXBUFLEN 1024

//Comments are almost same as the serverA
char strLine[MAXBUFLEN];
char strLine_distance[MAXBUFLEN];
char* wordSet_Pre[MAXBUFLEN];
char* wordSet_Suf[MAXBUFLEN];
int num_similar = 0;

struct search_result {
    char perfect[MAXBUFLEN];
    char edit[MAXBUFLEN];
    int num;
};

int isEditDistanceOne(char *str1, char *str2) {
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    int distance = 0;
    int i = 0, j = 0;
    if (len1 != len2) {
        return -1;
    }

    while (i < len1 && j < len2) {
        if (str1[i] != str2[j]) {
            if (distance == 1) {
                return -1;
            }
            distance ++;
            i++;
            j++;
        } else {
            i++;
            j++;
        }
    }
    if (distance == 1) {
        return 1;
    } else {
        return -1;
    }
}


char* search_exact(char *pstr) {
    FILE *fp = NULL;
    char result[MAXBUFLEN];
    fp = fopen("backendC.txt","r");
    if(fp == NULL){
        exit(0);
    }
    while(!feof(fp)) {
        memset(result, '\0', sizeof(result));
        fgets(strLine, MAXBUFLEN, fp);
        size_t length = (int)strlen(strLine)- strlen(strstr(strLine, " :: "));
        strncpy(result, strLine, length);
        if (strcmp(pstr, result) == 0) {
            return strLine;
        }
    }
    memset(strLine, '\0', sizeof(strLine));
    fclose(fp);
    return "0";
}


char* search_editDistance(char *pstr) {
    FILE *fp = NULL;
    char result[MAXBUFLEN];
    fp = fopen("backendC.txt", "r");
    if (fp == NULL) {
        exit(0);
    }
    while (!feof(fp)) {
        memset(result, '\0', sizeof(result));
        fgets(strLine_distance, MAXBUFLEN, fp);
        size_t length = (int) strlen(strLine_distance) - strlen(strstr(strLine_distance, " :: "));
        strncpy(result, strLine_distance, length);
        if (isEditDistanceOne(pstr, result) == 1) {
            num_similar++;
        }
    }
    fclose(fp);
    if (num_similar == 0) {
//        memset(strLine_distance, '\0', sizeof(strLine_distance));
        return NULL;
    } else {
        fp = fopen("backendC.txt", "r");
        if (fp == NULL) {
            exit(0);
        }
        while (!feof(fp)) {
            memset(result, '\0', sizeof(result));
            fgets(strLine_distance, MAXBUFLEN, fp);
            size_t length = (int) strlen(strLine_distance) - strlen(strstr(strLine_distance, " :: "));
            strncpy(result, strLine_distance, length);
            if (isEditDistanceOne(pstr, result) == 1) {
                return strLine_distance;
            }
        }
    }
    return NULL;
}


void prefix(char *pstr) {
    FILE *fp = NULL;
    int i = 0;
    char result[MAXBUFLEN];
    char prefix[MAXBUFLEN];
    fp = fopen("backendC.txt", "r");
    if (fp == NULL) {
        exit(0);
    }
    while (!feof(fp)) {
        memset(result, '\0', sizeof(result));
        memset(prefix, '\0', sizeof(prefix));
        fgets(strLine, MAXBUFLEN, fp);
        size_t length = (int) strlen(strLine) - strlen(strstr(strLine, " :: "));
        strncpy(result, strLine, length);
        strncpy(prefix, result, strlen(pstr));
        if (strcmp(prefix, pstr) == 0) {
            wordSet_Pre[i] = strdup(result);
            i++;
        }
    }
    fclose(fp);
}


void suffix(char *pstr) {
    FILE *fp = NULL;
    int i = 0;
    char result[MAXBUFLEN];
    char suffix[MAXBUFLEN];
    fp = fopen("backendC.txt","r");
    if(fp == NULL){
        exit(0);
    }
    while(!feof(fp)) {
        memset(result, '\0', sizeof(result));
        memset(suffix, '\0', sizeof(suffix));
        fgets(strLine, MAXBUFLEN, fp);
        size_t length = (int) strlen(strLine) - strlen(strstr(strLine, " :: "));
        strncpy(result, strLine, length);
        for(size_t j = strlen(result) - strlen(pstr), k = 0; k < strlen(pstr); j++, k++) {
            suffix[k] = result[j];
        }
        if (strcmp(suffix, pstr) == 0) {
            wordSet_Suf[i] = strdup(result);
            i++;
        }
    }
    fclose(fp);
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(void) {
    // Set up UDP -- From Beej
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    int rv;
    socklen_t addr_len;
    int num_pre;
    int num_suf;


    char finalResult[4 * MAXBUFLEN];

    struct search_result sr;

    memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
    hints.ai_addr = AF_UNSPEC; // don't care IP4 or IP6
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    hints.ai_flags = AI_PASSIVE; // Use my IP


    if ((rv = getaddrinfo(HOST, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 0;
    }


    // loop through all the results and bind to the first we can -- Beej
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
            == -1) {
            perror("ServerC: socket");
            continue;
        }

        if (bind(sockfd, p -> ai_addr, p -> ai_addrlen) == -1) {
            close(sockfd);
            perror("ServerC: bind");
            continue;
        }

        break;
    }


    if (p == NULL) {
        fprintf(stderr, "ServerC: failed to bind socket\n");
        return 2;
    }


    freeaddrinfo(servinfo);
    printf( "The ServerC is up and running using UDP on port <%s>.\n", MYPORT);


    while(1) {

        // receive data frowm aws -- Beej
        addr_len = sizeof(their_addr);
        char function[7];
        char buf[MAXBUFLEN];
        ssize_t numbytes;


        if((numbytes=recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                              (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        if ((numbytes = recvfrom(sockfd, function, sizeof(function) , 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }


        printf("The ServerC received input <%s> and operation <%s>\n", buf, function);


        if (strcmp(function, "search") == 0) {
            num_similar = 0;
            search_exact(buf);
            search_editDistance(buf);

            memset(finalResult,0,sizeof(finalResult));

            if (strlen(strLine) == 0) {
                printf("The serverC has found <%d> match and <%d> similar words\n", 0, num_similar);
            } else {
                printf("The serverC has found <%d> match and <%d> similar words\n", 1, num_similar);
            }

            strcpy(sr.perfect, strLine);
            strcpy(sr.edit, strLine_distance);
            sr.num = num_similar;
            memcpy(finalResult,&sr,sizeof(sr));

            if ((numbytes = sendto(sockfd, finalResult, sizeof(finalResult), 0, (struct sockaddr *) &their_addr, addr_len)) == -1) {
                perror("talker: sendto");
                exit(1);
            }
            
            printf("The ServerC finished sending the output to AWS\n");

        }

        if (strcmp(function, "prefix") == 0) {
            memset(wordSet_Pre, 0, MAXBUFLEN);
            prefix(buf);
            num_pre = 0;
            for (int i = 0; i < MAXBUFLEN; ++i) {
                if (wordSet_Pre [i] != NULL) {
                    num_pre ++ ;
                }
            }
            printf("testsssss");
            printf("The ServerC has found <%d> matches\n", num_pre);
            if ((numbytes = sendto(sockfd, (const char*)&num_pre, sizeof(num_pre), 0, (struct sockaddr *) &their_addr, addr_len)) == -1) {
                perror("talker: sendto");
                exit(1);
            }

            for (int j = 0; j < num_pre; ++j) {
                if ((numbytes = sendto(sockfd, wordSet_Pre[j], MAXBUFLEN, 0, (struct sockaddr *) &their_addr, addr_len)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }
            }

            if (numbytes > 0) {
                printf("The ServerC finished sending the output to AWS\n");
            }
        }


        if (strcmp(function, "suffix") == 0) {
            memset(wordSet_Suf, 0, MAXBUFLEN);
            suffix(buf);
            num_suf = 0;
            for (int i = 0; i < MAXBUFLEN; ++i) {
                if (wordSet_Suf[i] != NULL) {
                    num_suf++;
                }
            }
            printf("The ServerA has found <%d> matches\n", num_suf);

            if ((numbytes = sendto(sockfd, (const char*)&num_suf, sizeof(num_suf), 0, (struct sockaddr *) &their_addr, addr_len)) == -1) {
                perror("talker: sendto");
                exit(1);
            }

            for (int j = 0; j < num_suf; ++j) {
                if ((numbytes = sendto(sockfd, wordSet_Suf[j], MAXBUFLEN, 0, (struct sockaddr *) &their_addr, addr_len)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }
            }

            printf("The ServerA finished sending the output to AWS\n");
        }
    }
}
