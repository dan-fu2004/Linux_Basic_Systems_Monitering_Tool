#include <stdio.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <sys/types.h>
 #include <utmp.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>

// The goal of this function is to return an arr of strings which contains info from uname
char** get_kernal_info(){
    struct utsname kernal_info;
    uname(&kernal_info); //utsname and usname are from the documententation
    char** info = (char**)malloc(6*sizeof(char*)); //we allocate mem for 4 strings

    for(int i = 0; i<5; i++){
        info[i] = (char*)(malloc(50));
    }

    strcpy(info[0], kernal_info.sysname);
    strcpy(info[1],kernal_info.nodename);
    strcpy(info[2],kernal_info.release);
    strcpy(info[3],kernal_info.version);
    strcpy(info[4],kernal_info.machine);


    return info;
}

long* get_system_info(){
    struct sysinfo system_info;
    sysinfo(&system_info); 
    long* info = (long*)malloc(6*sizeof(long)); 

    info[0] = system_info.uptime;
    info[1] = system_info.totalram * system_info.mem_unit;
    // We multipy by mem_unit as the documentation states the totalram is given as a multiple of mem_unit
    info[2] = (info[1] - ((system_info.freeram + system_info.bufferram + system_info.sharedram)  * system_info.mem_unit));

    info[3] = info[1] + (system_info.totalswap * system_info.mem_unit);
    info[4] = (info[3] - ((system_info.freeram  + system_info.freeswap) * system_info.mem_unit));

    // info[1] and info[2] are total ram vs used ram
    // info[3] and info[4] is total mem vs used mem
    
    info[1] = info[1]/(1048576*1024);
    info[2] = info[2]/(1048576*1024);
    info[3] = info[3]/(1048576*1024);
    info[4] = info[4]/(1048576*1024);
    return info;
}


int get_core_count() {
    //Function to get number of cores 
    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
    char line[255];
    int cores = 0;

    if (cpuinfo == NULL) {
        perror("Could not open /proc/cpuinfo");
        return -1; // Return -1 to indicate error
    }

    while(fgets(line, 255, cpuinfo) ) {
        if (strncmp(line, "cpu cores", 9) == 0) {
            char* colon = strchr(line, ':'); 
            // Based on the /proc/cpuinfo file what we're looking for looks something like
            //cpu cores: int
            // We find this line then we create a pointer to the colon and increment it by 2 so we can get the INTEGER

            colon+=2;
            cores = atoi(colon);
            break;
            
        }
    }
    fclose(cpuinfo);
    return cores; 
}

void get_users(int* num_users, char names[][50], char ports[][40]){
    // Function to get users connected to the machine
    FILE *ufp = fopen(_PATH_UTMP, "r");
    int numberOfUsers = 0;
    struct utmp usr;
    //From Documentation, this File is a sequence of UTMP structs not strings.
    //So like how we read strings, we instead read and then store utmp structures inside our usr var
    while (fread(&usr, sizeof(usr),1, ufp) == 1)  {
    if (*usr.ut_name && *usr.ut_line && *usr.ut_line != '~') {

        if (numberOfUsers >=49) {
            break;
        }

        strcpy(names[numberOfUsers], usr.ut_name);
        names[numberOfUsers][49] = '\0'; // Ensure null-termination
        strcpy(ports[numberOfUsers], usr.ut_line);
        ports[numberOfUsers][39] = '\0'; // Ensure null-termination
        numberOfUsers++;
        }
    }

    fclose(ufp);
    *num_users = numberOfUsers;
 }


void read_cpu_times(long long *idle_time, long long *total_time) {
    //This function reads the proc/stat file and calculates the total cpu time for cpu usage
    FILE *fp = fopen("/proc/stat", "r");
    long long user, nice, system, idle, iowait, irq, softirq, steal;
    fscanf(fp, "cpu %lld %lld %lld %lld %lld %lld %lld %lld", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal); 
    //Since we know the file format beforehand, we can use fscanf to match the file with our vars

    *idle_time = idle + iowait;
    *total_time = user + nice + system + idle + iowait + irq + softirq + steal;

    fclose(fp);
}

double get_cpu_usage() {
    //This function reads the cpu_times twice in 1 second time intervals to calculate the cpu usage
    long long idle_time1, total_time1;
    long long idle_time2, total_time2;

    read_cpu_times(&idle_time1, &total_time1);
    sleep(1); // We sample over 1 secs
    read_cpu_times(&idle_time2, &total_time2);

    long long idle_diff = idle_time2 - idle_time1;
    long long total_diff = total_time2 - total_time1;
    //We just calculate the difference between the two times
    return (1.0 - (double)idle_diff / total_diff) * 100.0; 
}


void print_info(bool system, bool user, bool sequential, int samples, int tdelay){
    //Function to print info

    long current_ram[samples]; 
    long current_swap[samples];
    long* info;
    printf("\033[2J"); //clear the screen first

    for(int i =0; i<samples; i++){
        if(sequential == false){
            printf("\033[2J"); //Clear the Screen if sequential is false
            //If sequential true then we print everything in order
        }
        printf("Nbr of samples: %d -- every %d seconds\n", samples, tdelay);
        printf("Iteration Number: %d\n", i+1);

        if(user!= true){
            printf("------------------------------------------\n");
            printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
            info = get_system_info();
            current_ram[i] =info[2];
            current_swap[i] = info[4];

            for(int j = 0; j<i+1; j++){
                printf("%ld GB / %ld GB --  %ld GB / %ld GB\n", current_ram[j], info[1], current_swap[j], info[3]);
            }

            free(info); // Since we allocate memory when we get_system_info() we free at the end
        }

        if(system!= true){
            printf("-----------------------------------------\n");
            printf("### Sessions/users ###\n");
            char names[50][50];
            char ports[50][40];
            int num_of_users = 0;
            get_users(&num_of_users, names, ports); // we just load user data into names and ports

            for(int j=0; j<num_of_users; j++){
                printf("%s           %s\n", names[j], ports[j]);
            }
        }
        if(user!= true){
            printf("-----------------------------------------\n");
            int cores = get_core_count();
            double cpu_usage = get_cpu_usage();  // Load data into cores and cpu_usage
            printf("Number of cores: %d\n", cores);
            printf("total cpu use: %f%%\n", cpu_usage);
        }

        if(user == false  && system == false){
            char** kernal_info = get_kernal_info();
            printf("-----------------------------------------\n");
            printf("### System Information ###\n");
            printf("System_name = %s\n", kernal_info[0]);
            printf("Machine_name = %s\n", kernal_info[1]);
            printf("Version = %s\n", kernal_info[3]);
            printf("Release = %s\n", kernal_info[2]);
            printf("Architercture = %s\n", kernal_info[4]);
            free(kernal_info); // same with kernal_info
        }
        printf("-----------------------------------------\n");
        printf("\n \n \n \n \n"); // We move the cursor 5 down at the end. Useful for sequential output
        //I couldn't get the escape codes working for moving the cursor down in my vscode terminal so I just use 5 \n 
        sleep(tdelay);



   }
 



}

int main(int argc, char** argv){
    int opt;
    int option_index = 0;

    bool system_flag = false;
    bool user_flag = false;
    bool sequential_flag = false;

    int samples = 10; // Default number of samples
    int tdelay = 1;   // Default time

    static struct option long_options[] = { // These are the flags we can accept
        {"system", no_argument, 0, 's'},
        {"user", no_argument, 0, 'u'},
        {"sequential", no_argument, 0, 'q'},
        {"samples", optional_argument, 0, 'n'},
        {"tdelay", optional_argument, 0, 't'},
        {0, 0, 0, 0}  // End marker
    };

    // Process options
    while ((opt = getopt_long(argc, argv, "suqn::t::", long_options, &option_index))  !=-1) {
         // We loop through all the flags then we have logic for handling everything
        switch (opt) {
            case 's':
                system_flag = true;
                break;
            case 'u':
                user_flag = true;
                break;
            case 'q':
                sequential_flag = true;
                break;
            case 'n':
                if (optarg) {
                    samples = atoi(optarg);
                }
                break;
            case 't':
                if (optarg) {
                    tdelay = atoi(optarg);
                }
                break;

            default:
        }


    }

    print_info(system_flag, user_flag, sequential_flag, samples, tdelay);

}