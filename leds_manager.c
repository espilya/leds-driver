#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]){
    int len = 1;
    int fid = open("/dev/leds_driver", O_WRONLY);
    int i, ret;
    char* data = "0123";
    for(i = 0; i<10; i++){
        ret = write(fid, data+i%4, len);
        printf("%d, %d, %c, %d\n", fid, ret, *data+i%4, len);
        sleep(1);
    }
    /*

    */
    close(fid);

    return 0;
}