#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>

#define BUFLEN 512

void err(char *s)
{
    perror(s);
    exit(1);
}

size_t trimwhitespace(char *out, size_t len, const char *str)
{
  if(len == 0)
    return 0;

  const char *end;
  size_t out_size;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
  {
    *out = 0;
    return 1;
  }

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;
  end++;

  // Set output size to minimum of trimmed string length and buffer size minus 1
  out_size = (end - str) < len-1 ? (end - str) : len-1;

  // Copy trimmed string and add null terminator
  memcpy(out, str, out_size);
  out[out_size] = 0;

  return out_size;
}

void echoserver(int sockfd)
{
  struct sockaddr_in rem_addr;
  socklen_t addrlen = sizeof(rem_addr);
  char buf[BUFLEN];
  int i, recvlen;
  for (i=0; i<BUFLEN;i++)
    buf[i] = 0;
  printf("receiving nau\n");
  while(1)
  {
    recvlen = recvfrom(sockfd, buf, 512, 0, (struct sockaddr*)&rem_addr, &addrlen);
    if (recvlen > 0) {
      //if (buf[0] == 'q') break;
      printf("received %d bytes from %s: \"%s\"\n",recvlen, inet_ntoa(rem_addr.sin_addr), buf);
      if (sendto(sockfd, buf, recvlen, 0, (struct sockaddr*)&rem_addr, addrlen)==-1)
        err("sendto() failed");
    }
  }

  close(sockfd);
}

void interactive(int sockfd, char* remaddr, int port)
{
  // Initializing remote address!?
  struct sockaddr_in serv_addr;
  int slen=sizeof(serv_addr);
  struct sockaddr_in rem_addr;
  socklen_t addrlen = sizeof(rem_addr);
  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  if (inet_aton(remaddr, &serv_addr.sin_addr)==0) //just converting ip to bin
    err("invalid address failed\n");

  int recvlen;
  char buf[BUFLEN];
  int i;
  for (i=0; i<BUFLEN;i++)
    buf[i] = ' ';
  while(1)
  {
    printf(">");
    scanf("%[^\n]",buf); // here we block :<, need to use threads or something
    getchar();
    if(strcmp(buf,"exit") == 0)
      exit(0);

    char msg[BUFLEN];
    int msglen = trimwhitespace(&msg, BUFLEN, buf);
    printf("trying to send \"%s\", %d bytes\n", msg,msglen);
    //send our message
    if (sendto(sockfd, msg, msglen, 0, (struct sockaddr*)&serv_addr, slen)==-1)
      err("sendto() failed");
    //listen to what server send us... is this blocking!? lets expect that its not
    recvlen = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *)&rem_addr, &addrlen);
    if (recvlen > 0) {
      buf[recvlen] = 0;
      printf("server sent %d bytes: \"%s\"\n",recvlen, buf);
    }
  }

  close(sockfd);
}

int main(int argc, char** argv)
{
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
      err("error initializing socket");
  //Test if the socket is in non-blocking mode:
  if(fcntl(sockfd, F_GETFL) & O_NONBLOCK) {
    // socket is non-blocking
  }

  // Put the socket in non-blocking mode:
  if(fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK) < 0) {
    // handle error
    err("couln't put socket to non-blocking mode\n");
  }
  struct sockaddr_in myaddr;
  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(atoi(argv[1]));

  if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
    err("bind failed");

  char buf[BUFLEN];
  int i;
  for (i=0; i<BUFLEN;i++)
  buf[i] = ' ';
  printf("[i]nteractive or [e]cho\ni/e?");
  scanf("%[^\n]",buf);
  getchar();
  //lets initialize the socket first.
  printf("got: %s", buf);
  if (buf[0] == 'e') {
    if(argc < 2)
    {
      printf("Usage : %s <Port>\n",argv[0]);
      exit(0);
    }
    printf("wat\n");
    echoserver(sockfd);
  } else if(buf[0] == 'i') {
    if(argc < 4)
    {
      printf("Usage : %s <Port> <Server-IP> <Server-Port>\n",argv[0]);
      exit(0);
    }
    interactive(sockfd, argv[2], atoi(argv[3]));
  }

  return 0;
}
