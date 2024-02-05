# Systems Monitoring Tool

This is a tool used for getting information about the system such as seeing memory and cpu utilization
This will only work for linux OS

## How I created the tool
My plan to create the tool was to create individual functions to gather information then format and display that information
For example, to get the Kernal information, I use the function get_kernal_information which returns an array of strings containing the information
In this particular case I used the  struct utsname from #include <sys/utsname.h> to collect the information

After collecting the information, I then parsed the command line for any arguments and I created this struct:

static struct option long_options[] = { 
    {"system", no_argument, 0, 's'},
    {"user", no_argument, 0, 'u'},
    {"sequential", no_argument, 0, 'q'},
    {"samples", optional_argument, 0, 'n'},
    {"tdelay", optional_argument, 0, 't'},
    {0, 0, 0, 0}  // End marker
}; 

with the help of https://man7.org/linux/man-pages/man3/getopt.3.html

After parsing the command line for all the flags, I use bools to represent whether the flag is present and have seperate logic for formatting
and outputting the information using print_info()

## How to use
Compile the file with gcc Systems_Monitoring_Tool.c 

There are 5 flags: system, user, sequential, samples, tdelay.

the system flag prints only system information
the user flag prints only user information
the sequential flag prints information sequentially
the samples flag chooses how many times we sample the stats with default 10
the tdelay flag chooses how long we wait in seconds between every sample with defualt 1

By defualt, system, user and sequential flags are false, meaning if you do not specify, we will print all information non sequentially.
For example :
executing the compiled file with no flags: ./executable_file 
means that we are printing all the information

If we wanted to only see system information sequentially 20 times with 2 second delay we would execute with:
./executable_file --system --sequential --samples=20 --tdelay=2.


## Documentation

### char** get_kernal_info()

This function returns an array of strings containing the kernal information
We use the utsname struct from its corrosponding header file which contains the information.
We collect the system name, machine name, release, version and architecture and return it

### long* get_system_info()

This function returns and array of longs containing system information
We use the sysinfo struct from its corrosponding header file 
We calculate total physical mem vs physcal mem in use and total virtual mem and virtual mem in use

To calulate physical memory we take freeram * memory unit
To calulate free physical memory we subtract from the total with the free ram, buffered ram and shared ram

... Same process for total virtual memory and total virtual memory in use
Note we divide all our results by 1024^3 since it's orginal unit is in kilobyte so we divide by 1024^3 to get the corrosponding gigabyte representation

### void get_users(int* num_users, char names[][50], char ports[][40])

This function counts the amount of users currently using the system their names and their ports.
We change the pointer num_users directly to have the total amount of users while names and ports are 2d arrays

note: We can only hold 50 users and ports and the names of them must be less than 50 and 40 characters respectively

We use the _PATH_UTMP file which is /var/run/utmp 
which is a file containg the UTMP Struct 
https://man7.org/linux/man-pages/man5/utmp.5.html

As we read the file, we read each UTMP struct one by one and we get information such as the name and port

credit: https://stackoverflow.com/questions/4219695/how-to-programmatically-get-the-number-of-users-logged-in-on-a-linux-machine
this stack overflow thread help enlighten things

###  read_cpu_times(long long *idle_time, long long *total_time) 

This function reads the CPU information from the /proc/stat file.
We calculate the idle time with : idle+iowait 
and the total time with everything else

we then change the pointer idle_time and total_time respectively

### double get_cpu_usage() 

This function returns a percentage of cpu utilization.
We just take the total idle time / total time

We sample the CPU information over 1 second. 


### print_info(bool system, bool user, bool sequential, int samples, int tdelay)

This function collects all the information using the functions above, formats then outputs

We have flags: system, user, sequential... which change the behaviour of the function

This function works by collecting all the information, formatting then outputting for each iteration
after every iteration, given that sequential is false, we clear the screen and get ready to output the next iteration
At the end of every iteration, we wait tdelay seconds meaning we only sample every tdelay seconds

We can seperate each output into their different sections:

System out and user output and we can change which one we output depending on system and user.
for example: if system is false, then we only print user info and vice versa if user is false

note: if system and user are both false then we don't print anything.

### Main function, parsing arugments

Mainly used for parsing command line arugments

we have this code:

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


This code defines our arugments into our static struct.
We then loop through and parse the arguments. Everytime we encounter a valid argument, we set it's respective flag to True or we change a 
default value as with samples and tdelay.

We can then pass the correct flags to print_info.