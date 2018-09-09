================================================================================
                         EE450 Socket Project README
================================================================================

                           a. Name:    Jie Li
                              Session: #1
                           b. USC ID:  1335216981

================================================================================

c.  Work Have Done

    In this socket project, I have successfully implemented all the required
    functionality with one client file, one monitor file and four individual server files.

    At the beginning, I used local OS environment with IDE called Clion to code and test
    my programs. After I finished debug testing, I moved all the files to the
    Ubuntu 16.04 and tested my program as well. It worked as expected.

    I also successfully finished the extra part, AKA "Suffix" function.

================================================================================

d.  Code Files Information

    aws.c:
            This file is the main source code for the functionality of aws. It contains creating UDP
            connection to three backend servers and creating TCP connection to monitor and client.
            All information concerned with "search", "prefix" and "suffix" is manipulated in this file.

    serverA.c:
    serverB.c:
    serverC.c:
            These three files are main source code for the functionality of backend servers. They contain
            "search", "prefix" and "suffix" method to manipulate dictionaries respectively. They also connect
            to the aws separately.

    monitor.c:
            This file is the main source code for the functionality of monitor. It uses TCP to connect to the
            aws and show the results received form the aws.

    client.c:
            This file is the main source code for the functionality of client. It contains funcition that
            receive input by command line, and transfer them to the aws with TCP connection and redceive
            results from aws.

================================================================================

e.  The format of all the messages exchanged.

    All the messages are following what projects required.
    Extra:
        If one word is not found, the client and monitor windows will show "No match for <word>"
        And aws will show "The AWS sent < 0 > matches to client."
                          "The AWS sent < 0 > matches to monitor."

================================================================================

g.  Any idiosyncrasy of your project. It should say under what conditions the project fails, if any.

    1. My programs will work fine all the time except one case:
        I use             {char astr[MAXLENGTH];
                            size_t length = (int) strlen(searchResult) - strlen(strstr(searchResult, " :: "));
                            strncpy(astr, searchResult, length);
                            printf("Found a match for < %s >:\n",astr);
                          }
        to slice the sentece and get the words. It will work well on a IDE called CLion, but when I run
        the programs on Linux, seldom (not always), it will append a wired symbol at the end of the word.
        I think it is not the problem of source codes.
    2. My programs is case sensitive that means if you want to use "search", "prefix" and "suffix" function, you should
        type exact words, otherwise, it will return "Not Found".

================================================================================

h:  Reused Code:

        According to the instruction, I used some blocks of code concerned with TCP and UDP connection form
        Beej's tutorial books and I write them in comment. Except that, all source codes are written by myself.

================================================================================
