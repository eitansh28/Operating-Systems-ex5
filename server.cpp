#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>


  
typedef struct stack{
    char str[1024];
    int pointer;
    // stack* next;
}stack, *pst;

#define PORT "3490"  // the port users will be connecting to
#define MAXDATASIZE 100
#define BACKLOG 10   // how many pending connections queue will hold

stack* head;
int file_fd=open("fileLock.txt", O_WRONLY | O_CREAT | O_TRUNC);
struct flock my_lock;

//I took the code for 'malloc' and 'free' from here: https://stackoverflow.com/questions/8475609/implementing-your-own-malloc-free-with-mmap-and-munmap
void * my_malloc ( size_t size )
{
    int *plen;
    int len = size + sizeof( size ); // Add sizeof( size ) for holding length.

    plen = (int*)mmap( 0, len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0 );

    *plen = len;                     // First 4 bytes contain length.
    return (void*)(&plen[1]);        // Memory that is after length variable.
}

void my_free ( void * ptr )
{
    int *plen = (int*)ptr;
    int len;

    plen--;                          // Reach top of memory
    len = *plen;                     // Read length

    munmap( (void*)plen, len );
}

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *myThreadFun(void* new_fd){
    memset(&my_lock, 0, sizeof(my_lock));
    char buf[1024]={0};
    int sock=*(int*)new_fd;
    int numbytes;
    while (1) {
        bzero(buf,1024);
        if((numbytes = recv(sock, buf, MAXDATASIZE-1, 0)) ==-1) {
            printf("error\n");
            exit(1);
        }buf[numbytes]='\0';

        if(strstr(buf, "exit")){
            close(sock);
            break;
        }
        else if(strncmp(buf, "PUSH", 4)==0){  
            my_lock.l_type = F_WRLCK;
            fcntl(file_fd, F_SETLKW, &my_lock);
            int i = head->pointer+1;
            for (size_t j = 5; j < strlen(buf);i++, j++) {
                head->str[i] = buf[j];
                head->pointer++;
            }
            head->str[i] = '\0';
            head->pointer++;
            send(sock, "thank you for pushing", 25, 0);
            my_lock.l_type = F_UNLCK;
            fcntl(file_fd, F_SETLKW, &my_lock);
        }

        else if(strstr(buf, "POP")){         
            my_lock.l_type = F_WRLCK;
            fcntl(file_fd, F_SETLKW, &my_lock);
            if(head->pointer==0) {
                send(sock, "the stack is empty", 25, 0);
            }else{
                int stapop=head->pointer-1;
                while(head->str[stapop]!='\0'){
                    stapop--;
                    head->pointer--;
                } head->pointer--;
                send(sock, "thank you for popping", 25, 0);
                my_lock.l_type = F_UNLCK;
                fcntl(file_fd, F_SETLKW, &my_lock);
            }
        }
        else if(strstr(buf, "TOP")){ 
            my_lock.l_type = F_WRLCK;
            fcntl(file_fd, F_SETLKW, &my_lock);
            if (head->pointer==0){
                send(sock, "the stack is empty", 25, 0);
            }else {
                char ans[1024]="OUTPUT: ";
                int stast=head->pointer-1;
                while(head->str[stast]!='\0'){
                    stast--;
                }
                for(int k=8;head->str[stast+1]!='\0';k++){
                    ans[k]=head->str[stast+1];
                    stast++;
                }

                send(sock, ans, strlen(ans), 0);
            }
            my_lock.l_type = F_UNLCK;
            fcntl(file_fd, F_SETLKW, &my_lock);
        }  
    }
    return NULL;     
}
    
int main(void)
{
    head = (stack*)mmap( 0, 10000, PROT_READ | PROT_WRITE |PROT_EXEC, MAP_SHARED | MAP_ANON, -1, 0);
    head->str[0]='\0';
    head->pointer=0;
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        // if(pthread_create(&tid, NULL, &myThreadFun, &new_fd)!=0){
        //     printf("thread failed");
        // }
        if(!fork()){
            close(sockfd); // child doesn't need the listener
            myThreadFun(&new_fd);
            close(new_fd);
            exit(0);
        }
    }

    return 0;
}