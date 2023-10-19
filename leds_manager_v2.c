#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

/*
Programa que enciende los leds capsLock y scrollLock cada vez que hay una lectura o escritura en el disco definido mas abajo. 
Por otra parte enciende el led numLock cada vez que se realiza la comprobacion de nuevas lecturas/escrituras

Se accede al fichero "/proc/diskstats" para comprobar los accesos al disco definido lecturas/escrituras

*/

// sda, sda1, ...
#define DISK "sda" 
#define FILENAME "/proc/diskstats"
#define DRIVER "/dev/leds_driver"
#define	power_on_interval 40000
#define	power_off_interval 10000

bool led_state = 0;
char stat_dev[20];
char *off = "0", *on = "1";
unsigned long init_read_count, stat_read_count, stored_read_count;
unsigned long init_write_count, stat_write_count, stored_write_count;
#define STATS_FORMAT "%*d %*d %s %lu %*u %*u %*u %lu %*u %*u %*u %*u %*u %*u"

int stats_file;

// lectura de linea del fichero
unsigned int read_line(int file, char *line){
	int length = 0, read_count = 0;
	char character[2];

	while (true) {
		read_count = read(file, character, 1);
		if (read_count == 0)
			break;
		if (character[0] == '\0' || character[0] == '\n')
			break;
		line[length] = character[0];
		length += read_count;
	}
	line[length] = '\0';
	return length;
}

// apertura del fichero stats
bool open_stats_file(){
	bool result = true;
	stats_file = open(FILENAME, O_RDONLY);
	if (stats_file < 0) {
		perror(FILENAME);
		result = false;
	}
	return result;
}

// cierre del fichero stats
void close_stats_file(){
	if (stats_file)
		close(stats_file);
}

// parsear el fichero stats
void read_stats(char *device, unsigned long *read_count, unsigned long *write_count){
	char buff[200] = {0};
	unsigned int count = read_line(stats_file, buff);

	if (count == 0) {
		lseek(stats_file, 0, SEEK_SET);

		strcpy(device, "null");
		read_count = 0;
		write_count = 0;
		return;
	}
	sscanf(buff, STATS_FORMAT, device, read_count, write_count);
}

// funcion principal encargada de comprobar si hay nuevos accesos
void disk_check(bool *read, bool *write){
    // buscamos el disco que se corresponda con el DISK definido 
    do {
        read_stats(stat_dev, &stat_read_count, &stat_write_count);
    } while(strcmp(stat_dev, DISK) != 0);

    // comparamos las escrituras actuales con las anteriores
    if(stat_read_count > stored_read_count){
        *read = true;
        stored_read_count = stat_read_count;
    }
    if(stat_write_count > stored_write_count){
        *write = true;
        stored_write_count = stat_write_count;
    }
}

// escritura al driver que controla los leds
int led_write(int fid, char* data, int len){
    return write(fid, data, len);
}

// funcion de manejo de los leds
void led(int fid, bool r, bool w){
    int len = 1;
    int ret;
    char* data;

    if(r && w)
        data = "23";
    else if(r)
        data = "2";
    else
        data = "3";
    
    ret = led_write(fid, data, len);
    usleep(power_on_interval);
    ret = led_write(fid, off, len);
    usleep(power_off_interval);
    
    printf("dev: %s, read_count: %lu, write_count: %lu\n",stat_dev, stat_read_count-init_read_count, stat_write_count-init_write_count);
}

// funcion que invierte el estado del led
void invert_state(int fid){
    if(led_state)
        led_write(fid, on, 1);
    else 
        led_write(fid, off, 1);
    led_state = !led_state;
}

// inicializacion del contador de R y W inicial
void init(){
    bool r, w;
    disk_check(&r, &w);
    //printf("dev: %s, read_count: %lu, write_count: %lu\n",stat_device, stat_read_count-init_read_count, stat_write_count-init_write_count);
    init_read_count = stat_read_count;
    init_write_count = stat_write_count;
}

int main(int argc, char *argv[]){
    bool r, w;
    int fid;
    open_stats_file();
    init();
    fid = open(DRIVER, O_WRONLY);
    
    // loop 
    while(1){
        r = false, w = false;
        disk_check(&r, &w);
        if (r || w)
            led(fid, r, w);
        else
			usleep(1000);

        // cada vez que se hace check se enciende el led SCROLL
        invert_state(fid);
    }
    close(fid);
    close_stats_file();

    return 0;
}