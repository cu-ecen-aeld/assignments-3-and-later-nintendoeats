#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define ERROR(test, message...) if(test) {syslog (LOG_ERR, message); return 1;}

int main(int argc, char* argv[])
{
    openlog("writer",LOG_PERROR,LOG_USER);
    
    ERROR(argc != 3, "Incorrect number of arguments.")
    
    
    
    const char* const writeFile = argv[1];
    const char* const writeStr= argv[2];
    const size_t writeLength = strlen(writeStr);

    const int fd = open(writeFile, O_RDWR |O_CREAT| O_TRUNC , 0666);
    ERROR(fd == -1, "Target file %s could not be opened or created.", writeFile)
    
        
    syslog (LOG_DEBUG, "Writing %s to %s", writeStr, writeFile);

    write(fd, writeStr, writeLength);
    ERROR(errno != 0, "Error writing to file.")
    
    close(fd);
    ERROR(errno != 0, "Error closing file.")
    
    closelog();
}
