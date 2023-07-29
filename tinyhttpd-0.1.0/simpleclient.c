#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <chrono>

int main(int argc, char *argv[])
{
 int sockfd;
 int len;
 struct sockaddr_in address;
 int result;
 char ch = 'A';

 sockfd = socket(AF_INET, SOCK_STREAM, 0);
 address.sin_family = AF_INET;
 address.sin_addr.s_addr = inet_addr("127.0.0.1");
 address.sin_port = htons(9734);
 len = sizeof(address);

 std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
 result = connect(sockfd, (struct sockaddr *)&address, len);
 std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
 std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
 std::cout << data << std::endl;

 if (result == -1)
 {
  perror("oops: client1");
  exit(1);
 }
 write(sockfd, &ch, 1);
 read(sockfd, &ch, 1);
 printf("char from server = %c\n", ch);
 close(sockfd);
 exit(0);
}
