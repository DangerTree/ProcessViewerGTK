// file tests proc data access

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gtkProcStruct.h"

#define MAX_FILE 1024
#define MAX_PID 32768
#define MAX_WORD 64

//static void update (GtkListStore *);

// establish a stucture in which to hold process information
typedef struct pidNode{
    gchar * proc_name;
    gchar * user_name;
    //gchar cpu_percent [8];
    gfloat cpu_percent;
    gint pid;
    gchar * mem_using;
    int oldProcTicks;
    int oldCpuTicks;
} pidNode;

int updateCntr = 0;

// establish a static array with a slot for every possible pid
static pidNode * pidArray [MAX_PID];

static GtkListStore * store;


/* getCPUticks calculate the total CPU time spent     
 it does this by adding the following info from /proc/stat:
 user, nice, system, irq, softirq
*/ 
int getCPUticks (){
    
/*    int userTime;
    int niceTime;
    int systemTime;
    int irqTime;
    int softirqTime;
*/    int totalCPUtime = 0;
    char userTime[MAX_WORD];
    char niceTime[MAX_WORD];
    char systemTime[MAX_WORD];
    char irqTime[MAX_WORD];
    char softirqTime [MAX_WORD];

    // open a file stream to /proc/stat
    FILE * cpuProc = fopen ("/proc/stat", "r");
    char cpuTickLine [MAX_FILE];

    // read all lines from /proc/stat into cpuTickLine
    while (fgets (cpuTickLine, MAX_FILE, cpuProc) != NULL){
        if (cpuTickLine [0] == 'c'){
            /* tokenize the line string into Time categories:
                user(1), nice(2), systemTime(3), irq(6), and softirq(7)*/
            strtok (cpuTickLine, " ");
            /*userTime = atoi (strtok (NULL, " "));
            niceTime = atoi (strtok (NULL, " "));
            systemTime = atoi (strtok (NULL, " "));
            strtok (NULL, " ");
            strtok (NULL, " ");
            irqTime = atoi (strtok (NULL, " "));
            softirqTime = atoi (strtok (NULL, " "));
            */
            strcpy (userTime, strtok (NULL, " "));
            strcpy (niceTime, strtok (NULL, " "));
            strcpy (systemTime, strtok (NULL, " "));
            strtok (NULL, " ");
            strtok (NULL, " ");
            strcpy (irqTime, strtok (NULL, " "));
            strcpy (softirqTime, strtok (NULL, " "));
            

            //printf ("%d\t%d\t%d\t%d\t%d\n", userTime, niceTime, systemTime, irqTime, softirqTime);
        }
        else { break; }
    }
    fclose (cpuProc);
    
    // return the total number of CPU ticks
    //totalCPUtime = userTime + niceTime + systemTime + irqTime + softirqTime;
    totalCPUtime = atoi (userTime) + atoi (niceTime) + atoi (systemTime) + atoi (irqTime) + atoi (softirqTime);
    return totalCPUtime;
}


// calcProcTicks returns the sum of utime and stime of one proc
int calcProcTicks(char * pidPath){
    
    // open stat file
    char statPath [MAX_WORD];
    strncat (strncpy (statPath, pidPath, strlen(pidPath)+1), "/stat", 6);
    char statLine [MAX_FILE]; // this is way too big
    int newProcTicks = 0;

    // get sum of utime and stime of process from /proc/[pid]/stat
    FILE * pidStat = fopen (statPath, "r");
    if (pidStat == NULL){
        return 0;
    }       
    
    if (fgets (statLine, MAX_FILE, pidStat) != NULL){
        // go to 14th string in stat, utime
        strtok (statLine, " ");
        for (int i = 0; i < 12; i++){
            strtok (NULL, " ");
        }
        newProcTicks += atoi (strtok (NULL, " "));
        newProcTicks += atoi (strtok (NULL, " "));
    }
    fclose (pidStat);
    return newProcTicks;
}


// calcCPU calculates the %CPU and puts the correct value into the pidNode
// int isNew is 1 if this is a new process, and 0 is if is old
void calcCPU (pidNode * pNode, char * pidPath, int isNew){

    // calculate current number of ticks for the proc and CPU 
    int newProcTicks = calcProcTicks (pidPath);
    int newCpuTicks = getCPUticks();
    
    // if this is a new process, fill possible fields and return
    // OR if process has suddenly died
    if (isNew == 1 || newProcTicks == 0){
        pNode->oldProcTicks = newProcTicks;
        pNode->oldCpuTicks =  newCpuTicks;
        pNode->cpu_percent = 0.0;
        /*char * zeroCpuPercent = malloc (8);
        zeroCpuPercent = "0.00%\0";
        int zLen = strlen (zeroCpuPercent);
        memset (pNode->cpu_percent, 0, 8);
        strncpy (pNode->cpu_percent, zeroCpuPercent, strlen(zeroCpuPercent));
        int pLen = strlen (pNode->cpu_percent);
        */
        return;
    }
    
    // otherwise, this process is already in pidArray.
    // calculate CPU%: (newProcTicks - oldProcTicks)/(newCPUtime - oldCPUtime)
    float cpuPrcnt = ((float) newProcTicks - (float) pNode->oldProcTicks)/((float) newCpuTicks - (float) pNode->oldCpuTicks);
    
    /* check if cpuPrcnt is valid (will be negative if proc is suddenly dead)
    if (cpuPrcnt < 0){
        
    }*/

    // put the cpu percent into readable string format
    /*char cpuStr [8];
    memset (cpuStr, 0, 8);
    sprintf (cpuStr, "%.2f", cpuPrcnt*100);
    strncat (cpuStr, "\%\0", 2);

    memset (pNode->cpu_percent, 0, 8);
    // assign cpu_percent
    //cpuStr[8] = '\0';
    strncpy (pNode->cpu_percent, cpuStr, 8);
    //pNode->cpu_percent = malloc (8);
    //pNode->cpu_percent = cpuStr;
    printf ("%s\n", cpuStr);
    */
    char retStr [20];
    snprintf (retStr, 20, "%.2f", cpuPrcnt);
    gfloat toRet = (gfloat) atof (retStr);
    pNode->cpu_percent = toRet;
    // update pNode tick vals
    pNode->oldCpuTicks = newCpuTicks;
    pNode->oldProcTicks = newProcTicks;
}


// getProcName returns the Process Name of a pid
//char * getProcName (char * pidPath){
gchar * getProcName (pidNode * pNode, char * pidPath){
    
    int pathLen = strlen (pidPath) + 6;
    char * procName = malloc (MAX_WORD);
    char procCommPath [pathLen];
    strncat (strncpy (procCommPath, pidPath, strlen(pidPath)+1), "/comm", 5);

    FILE * pid = fopen (procCommPath, "r");
    if (pid == NULL){
        return (gchar *) "Dead";
    }
    fgets (procName, pathLen, pid);
    fclose (pid);
    printf("\t%s", procName);
    
    int pNameSize = strlen(procName) + 1;
    char * pNameStr = malloc (pNameSize);
    strncpy (pNameStr, procName, pNameSize);
    return (gchar *) pNameStr;
    //strncpy (pNode->proc_name, (gchar *) procName, strlen(procName)+1);
    //pNode->proc_name = (gchar *) procName;
}


// getMem returns the size of the program from a pid's statm, and converts to MB
gchar * getMem (char * pidPath){
    
    char procStatm [MAX_WORD];
    char memStrRaw [MAX_WORD];
    // assign procStatm to /proc/[pid]/statm
    strncat (strncpy (procStatm, pidPath, strlen(pidPath)+1), "/statm", 7);
    
    FILE * memFile = fopen (procStatm, "r");

    // in the unlikely chance that the proc ended since getMem was called
    if (memFile == NULL){
        gchar * toRet = "N/A\0";
        return toRet;
    }

    fgets (memStrRaw, MAX_WORD, memFile);
    gfloat memUsed = atoi (strtok (memStrRaw, " "));
    fclose (memFile);
    
    printf ("\t%f", memUsed);           

    memUsed = memUsed * 4; // muliply by page size
    // charMemUsed is char rep of int with two decimal places
    char * memStrDone = malloc (16);
    char * charMemUsed = malloc (16);
    //snprintf (charMemUsed, strlen(atoi(memUsed))+1, "%.2f", memUsed);
    sprintf (charMemUsed, "%.2f", memUsed);
    
    if (memUsed != 0){
        // put proper units in
        if (strlen(charMemUsed) < 7){ // KB
            //memUsed = memUsed;
            sprintf (memStrDone, "%.2f", memUsed);
            strncat (memStrDone, " KB\0", 4);
        }
        else {
            memUsed = memUsed / 1024;
            sprintf (memStrDone, "%.2f", memUsed);
            strncat (memStrDone, " MB\0", 4);
        
        }
        // put it into Megabyte units
    }
    else {
        memStrDone = "N/A\0";
    }

    printf ("\t%.2f\t", memUsed);
    
    // put memUsed in a string with proper mem unit appended to it
    //char * memStrDone = malloc (sizeof (memUsed) + 5);
    //sprintf (memStrDone, "%.2f", memUsed);
    //strncat (memStrDone, " MB\0", 4);

    return (gchar *) memStrDone;
}


// getUserName returns a string rep of the User / Owner of the process from pidPath
gchar * getUserName (char * pidPath){

    struct stat * fileStat = malloc (sizeof (struct stat));
    struct passwd * pwd;
    
    // get stat info on the file
    stat (pidPath, fileStat);            
    // and print out the user owning the process
    pwd = getpwuid(fileStat->st_uid);
    
    int usrNameSize = strlen (pwd->pw_name);
    char * user = malloc (usrNameSize + 1);    
    char holder [usrNameSize+1];
    strncpy (holder, pwd->pw_name, usrNameSize);
    holder [usrNameSize] = '\0';
    strncpy (user, holder, usrNameSize+1);
    printf("\t%s", user);
    
    free (fileStat);
    return (gchar *) user;
}


/* addNewLSRow creates a new row for data in a list store,
and populates it with data from a pidNode
*/
void addNewLSRow (pidNode * newNode, GtkTreeIter * iter, GtkListStore * store){

    // create a new row in listStore
    gtk_list_store_append (store, iter);

    // add the pidNode to the listStore
    gtk_list_store_set (store, iter, PROC_NAME, newNode->proc_name, 
                                    USER_NAME, newNode->user_name,
                                    CPU_PERCENT, newNode->cpu_percent,
                                    PID, newNode->pid,
                                    MEM_USING, newNode->mem_using, -1);
}


// free pidArray of dead processes and updates LS iterator
void refreshListStore (GtkListStore * store, GtkTreeIter * iter, int pidNum){ 

    gboolean valid = TRUE;    
    gint LSpid;

    // while the iter is not to the end of the LS
    //while (valid){
    while (iter != FALSE || iter != 0){
        // and the listStore iter-> pid != this pid
//        gint LSpid = 0;
        gtk_tree_model_get (GTK_TREE_MODEL (store), iter, PID, &LSpid, -1);
        LSpid = (int) LSpid;
        // delete the row from the listStore
        if (LSpid < pidNum){
            // delete the that pid from the pidList
            pidArray [LSpid] = NULL;
            gtk_list_store_remove (GTK_LIST_STORE (store), iter);
            // if iter is not pointing to 
        }
        else { break; }
        // increment the iterator, try again
        //valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
    }
}


/* update: goes through all processes and puts them + info in liststore
*/
int update (GtkListStore * store){
    
    gchar * pid;

    // establish listStore iter to start of listStore
    GtkTreeIter iter;
    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);

    // establish a DIR and stru
    DIR * procDir;
    struct dirent * procDirent;
    char path [MAX_WORD] = "/proc/";
    char pidPath [MAX_WORD] = "/proc/";

    procDir = opendir (path);


    // for all entries in the directory that start with a number
    while ((procDirent = readdir (procDir)) != NULL){
        pid = procDirent->d_name;
        if (pid[0] >= 48 && pid[0] <= 57){
            printf("%s\t", pid);            
            int pidNum = atoi (pid); // store pid

            updateCntr++;

            // store /proc/[pid] path
            strncat (pidPath, pid, strlen(pid));
            printf("%s\t", pidPath);
            
            // UPDATE PIDARRAY AND LIST STORE
            // if pid is in the pidArray, update memUsed and cpu%
            if (pidArray [pidNum] != NULL){
                pidArray [pidNum] -> mem_using = getMem (pidPath);//memUsed;
                calcCPU (pidArray [pidNum], pidPath, 0);
                
                // update liststore: rm old processes, set iter-> correct row
                refreshListStore (store, &iter, pidNum);

                // update the listStore with pidList vals
                gtk_list_store_set (store, &iter, MEM_USING, pidArray[pidNum]->mem_using, CPU_PERCENT, pidArray[pidNum]->cpu_percent, -1);
                gboolean nextIter = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
                // if iter is at end of LS, append a row in case there is more pids to store
/*                if (nextIter == FALSE){
                    gtk_list_store_append (store, &iter);
                }
*/                printf("\n");
            }

            // if pid is not in the pidArray
            else {
                // add all vals to the node
                pidNode * newNode = malloc (sizeof (pidNode));
                newNode->mem_using = getMem (pidPath);
                newNode->user_name = getUserName (pidPath);
                newNode->pid = (gint) pidNum;
                newNode->proc_name = getProcName (newNode, pidPath);
                calcCPU (newNode, pidPath, 1);

                // add the pid to the pidList
                pidArray [newNode->pid] = newNode;
                // add the process info to the liststore
                // create a new row in listStore
                gtk_list_store_append (store, &iter);

                // add the pidNode to the listStore
                gtk_list_store_set (store, &iter,
                                    PROC_NAME, newNode->proc_name, 
                                    USER_NAME, newNode->user_name,
                                    CPU_PERCENT, newNode->cpu_percent,
                                    PID, newNode->pid,
                                    MEM_USING, newNode->mem_using, -1);


//                addNewLSRow (newNode, &iter, store);
            }

            // reset path to "/proc/" by appending a null char
            pidPath[6] = '\0';}
    }
    //int ticks = getCPUticks();
    closedir(procDir);
    return 1;
}



int main (){

    gtk_init (0, NULL);

    store = gtk_list_store_new (COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_INT, G_TYPE_STRING);

    // initiate sorting
    // ---------------------

    update (store);
    
    // create treeview (columns)
    GtkWidget *treeview = gtk_tree_view_new ();
    build_treeview (treeview);

    // create timeout refresh
    g_timeout_add_seconds (1, (GSourceFunc) update, (gpointer) store);
    
    // add the model to the tree view
    gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

    // unreference the model so it will be destroyed when window closes
    g_object_unref (store);

    // display the tree
    display (treeview);

    gtk_main ();

    return 1;
}


